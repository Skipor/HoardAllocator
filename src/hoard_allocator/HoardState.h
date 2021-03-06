#ifndef HOARD_STATE_H
#define HOARD_STATE_H

#include <mutex>
#include <thread>
#include <iostream>

#include "AllocFreeHashMap.h"
#include "utils.h"
#include "FreeSuperblockManager.h"
#include "GlobalHeap.h"
#include "LocalHeap.h"

namespace hoard {

inline bool CanBeBigAllocation(void * ptr) {
  return reinterpret_cast<size_t>(ptr) % kPageSize == 0;
}

class HoardState {

public:
  hoard::lock_t big_alloc_mutex;
  hoard::AllocFreeHashMap big_allocates_map;

  HoardState();

  LocalHeap & GetLocalHeap(size_t block_size);

  ~HoardState();

private:
  constexpr static size_t kDifferntBlockSizes = FloorLog2(kMaxBlockSize / kMinBlockSize) + 1;

  static size_t GetHeapIndexByBlockSize(size_t block_size) {
//    assert(IsPowerOf2(block_size));
    return FloorLog2((block_size * 2 - 1) / kMinBlockSize);
  }

  class GlobalHeapGroup {

  public:
    GlobalHeapGroup(FreeSuperblockManager &superblock_manager) : heaps_(reinterpret_cast<GlobalHeap*>(data_)) {
      size_t i_heap = 0;
      for (size_t current_block_size = kMinBlockSize; current_block_size <= kMaxBlockSize; current_block_size *= 2, ++i_heap) {
        new(&(heaps_[i_heap])) GlobalHeap(superblock_manager, current_block_size);
      }
      assert(i_heap == kDifferntBlockSizes);

    }

    GlobalHeapGroup(const GlobalHeap &) = delete;

    GlobalHeapGroup &operator=(const GlobalHeap &) = delete;

    GlobalHeap &operator[](size_t index) {
      assert(index < kDifferntBlockSizes);
      return heaps_[index];
    }

  private:
    GlobalHeap * const heaps_;
    char data_[sizeof(GlobalHeap) * kDifferntBlockSizes];

  };


  class LocalHeapGroup {
  public:
    LocalHeapGroup(GlobalHeapGroup &global_heap_group) : heaps_(reinterpret_cast<LocalHeap*>(data_)) {
      for (size_t i_heap = 0; i_heap < kDifferntBlockSizes; ++i_heap) {
        new(&(heaps_[i_heap])) LocalHeap(global_heap_group[i_heap]);
      }
    }

    LocalHeapGroup(const GlobalHeap &) = delete;

    LocalHeapGroup &operator=(const GlobalHeap &) = delete;

    LocalHeap &operator[](size_t index) {
      assert(index < kDifferntBlockSizes);
      return heaps_[index];
    }

  private:
    LocalHeap * const heaps_;
    char data_[sizeof(LocalHeap) * kDifferntBlockSizes];
  };

  const size_t kRealPageSize_;
  const size_t kNumberOfCPU_;
  const size_t kNumberOfHeapGroups_;
  hoard::FreeSuperblockManager superblock_manager_;
  GlobalHeapGroup global_heap_group_;
  LocalHeapGroup *local_heap_groups_; //array initialised and allocated dynamically

};

HoardState::HoardState() : kRealPageSize_(static_cast<size_t >(sysconf(_SC_PAGESIZE))),
                           kNumberOfCPU_ (static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN))),
                           kNumberOfHeapGroups_(kNumberOfCPU_ * kHeapsPerCPU),
                           superblock_manager_(),
                           global_heap_group_(superblock_manager_) {
  trace("HoardState: ", "Construct");
#ifndef NDEBUG
  trace("HoardState: ", "DEBUG");
#else
  trace("HoardState: ", "non-DEBUG");
#endif


  trace("HoardState: ", "PageSize: ", kRealPageSize_, " CPU num: ", kNumberOfCPU_, " HeapGroupsNum: ", kNumberOfHeapGroups_);
  if(kPageSize != kRealPageSize_) {
    fatal_error("change kPageSize and recompile lib for your machine");
  }

  local_heap_groups_ = static_cast<LocalHeapGroup *>(mmapAnonymous(sizeof(LocalHeapGroup) * kNumberOfHeapGroups_));
  check_fatal(local_heap_groups_ != nullptr, "can't allocate local heap groups");

  for (size_t i_group = 0; i_group < kNumberOfHeapGroups_; ++i_group) {
    new(local_heap_groups_ + i_group) LocalHeapGroup(global_heap_group_);
  }
}

LocalHeap &HoardState::GetLocalHeap(size_t block_size) {
  const size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
  const size_t local_group_id = thread_id % kNumberOfHeapGroups_;
  LocalHeap& res =  local_heap_groups_[local_group_id][GetHeapIndexByBlockSize(block_size)];
  trace("GetLocalHeap ", "Thread: ", thread_id, " group_id: ", local_group_id, " got heap: ", &res, " block_size: ", res.one_block_size());
  return res;
}

HoardState::~HoardState() {
  trace("HoardState: ", "Destruct");
  if(munmap(local_heap_groups_, sizeof(LocalHeapGroup) * kNumberOfHeapGroups_) != 0) {
    fatal_error("can't unmap loacal heap groups");
  }
}

} //hoard
#endif //HOARD_STATE_H
