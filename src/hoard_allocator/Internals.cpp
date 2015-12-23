#include <cstring>

#include "HoardState.h"
#include "Internals.h"

namespace hoard {
namespace {

__thread bool thread_inited = false;
std::atomic<bool> state_is_inited;
hoard::lock_t init_mutex;


char state_data[sizeof(HoardState)];
HoardState & state = *reinterpret_cast<HoardState *>(state_data); //ugly hack. State will initialized manually


void InitOnce() {
	if (!thread_inited) {
		if(!state_is_inited.load()) {
			hoard::lock_guard guard(init_mutex);
      if(!state_is_inited.load()) {
        trace("INIT ONCE");
        ResetState();
				state_is_inited.store(true); //after construct!
			}
		}
    thread_inited = true;
	}
}

} // namespace

void ResetState() {
  new(&state) HoardState();
}

void *InternalAlloc(size_t size, size_t alignment) {
  trace("InternalAlloc ", "size: ", size, " alignment: ", alignment);

  InitOnce();
  check_fatal(IsValidAlignment(alignment), "Invalid alignment");

//  if (size < kMinBlockSize) {
//    trace("InternalAlloc , change size to ", kMinBlockSize);
//    size = kMinBlockSize;
//  }

//  if (alignment < kDefaultAlignment) {   // not needed
//    trace("InternalAlloc , change alignment to ", kMinBlockSize);
//    alignment = kDefaultAlignment;
//  }
  trace("InternalAlloc ", "size became: ", size);

	void *result;
	if (size > kMaxBlockSize) {
		result = BigAlloc(size, alignment);
	} else {
		result = SmallAlloc(size, alignment);
	}
	return result;
}


void *SmallAlloc(size_t size, size_t alignment) {
  trace("SmallAlloc ", "size: ", size, " alignment: ", alignment);
  size = std::max(size, alignment);
  LocalHeap &heap = state.GetLocalHeap(size);
  return heap.Alloc();
}

void InternalFree(void *ptr) {
  trace("InternalFree: ", ptr);
	InitOnce();
	if (ptr == nullptr)
		return;

	//big allocations always have page size alignment
	if (((size_t) ptr % kPageSize) || !BigFree(ptr)) {
		SmallFree(ptr);
	}
}

void SmallFree(void *ptr) {
  trace("SmallFree: ", ptr);
  Superblock *const superblock = Superblock::Get(ptr);
  SuperblockHeader &header = superblock->header();
  auto owner_lock_guard = header.GetOwnerLock();
  assert(header.valid());
  BaseHeap * const owner = header.owner();
  owner->Free(superblock, ptr);
}


void *BigAlloc(size_t size, size_t alignment) {
  trace("BigAlloc ", "size: ", size, " alignment: ", alignment);
	assert(IsValidAlignment(alignment));
  size = RoundUp(size, kPageSize);// without it, can be memory leak
	void * const result_ptr = mmapAligned(size, alignment);
	if (!result_ptr) {
		return nullptr;
	}

	hoard::lock_guard guard(state.big_alloc_mutex);
	state.big_allocates_map.Add(result_ptr, size);
	return result_ptr;
}


//returns true if ptr was big allocation
bool BigFree(void *ptr) {
  trace("BigFree: ", ptr);
	hoard::lock_guard guard(state.big_alloc_mutex);
	const size_t size = state.big_allocates_map.Get(ptr);
	if (size == AllocFreeHashMap::kNoSuchKey) {
		return false;
	}
  check_fatal(size % kPageSize == 0, "invalid size freed"); // TODO make debug check
	check_fatal(munmap(ptr, size) == 0, "BigFree unmap failed");
  state.big_allocates_map.Remove(ptr);
  return true;
}

void *InternalRealloc(void *ptr, size_t size) {
  trace("InternalRealloc: ", ptr, size);
	if (ptr == nullptr) {
		return InternalAlloc(size);
	}

	if (size == 0) {
		InternalFree(ptr);
		return nullptr;
	}

	size_t allocation_size = AllocFreeHashMap::kNoSuchKey;
  if(CanBeBigAllocation(ptr)) {
    hoard::lock_guard lock(state.big_alloc_mutex);
    allocation_size = state.big_allocates_map.Get(ptr);
  }
  const bool is_small_allocation = allocation_size == AllocFreeHashMap::kNoSuchKey;

  if (is_small_allocation) {
    SuperblockHeader &header = Superblock::Get(ptr)->header();
    //block size is valid, because can't be changed, while sb not empty
    allocation_size = header.owner()->one_block_size();
  }

  void * const result = InternalAlloc(size);
  if (result == nullptr) {
    return nullptr;
  }

  std::memcpy(result, ptr, std::min(allocation_size, size));
  InternalFree(ptr);
  return result;



//  //TODO: use this optimised version, when other things will be tested
//  if (is_small_allocation) {
//    SuperblockHeader &header = Superblock::Get(ptr)->header();
//    const size_t real_size = header.owner()->one_block_size();
//    const bool fits_in_old_block = real_size / 2 < size && size <= real_size;
//    if (fits_in_old_block) {
//      return ptr;
//    }
//    void *result = InternalAlloc(size);
//    if (result == nullptr) {
//      return nullptr;
//    }
//    std::memcpy(result, ptr, std::min(real_size, size));
//    SmallFree(ptr);
//    return result;
//  } else {
//    const bool will_be_big_allocation = size > kMaxBlockSize;
//    if (will_be_big_allocation) {
//      const size_t rounded_size = RoundUp(size, kPageSize);
//      const size_t rounded_big_alloc_size = RoundUp(big_alloc_size, kPageSize);
//
//      if (rounded_size == rounded_big_alloc_size) {
//        return ptr;
//      } else if (rounded_size < rounded_big_alloc_size) {
//        const int unmap_res = munmap(static_cast<char *>(ptr) + rounded_size, rounded_big_alloc_size - rounded_size);
//        if (unmap_res != 0) {
//          hoard::fatal_error("Realloc unmap faulure");
//        }
//        lock_guard lock(state.big_alloc_mutex);
//        state.big_allocates_map.Set(ptr, size);
//        return ptr;
//      } else { //> rounded_big_alloc_size
//        void *result = mmapAligned(ptr, rounded_size);
//        if (result == nullptr) {
//          //can't get more memory
//          return nullptr;
//        }
//        if(result == ptr) { // allocation was enlarged
//          lock_guard lock(state.big_alloc_mutex);
//          state.big_allocates_map.Set(ptr, size);
//          return result;
//        }
//        std::memcpy(result, ptr, big_alloc_size);
//        assert(BigFree(ptr));
//        lock_guard lock(state.big_alloc_mutex);
//        state.big_allocates_map.Add(result, size);
//        return result;
//      }
//    } else { //will be small
//      void *result = SmallAlloc(size);
//      if (result == nullptr) {
//        return nullptr;
//      }
//      std::memcpy(result, ptr, size);
//      assert(BigFree(ptr));
//      return result;
//    };
//  }




}

}// hoard
