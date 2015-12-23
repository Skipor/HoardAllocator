#ifndef LOCAL_HEAP_H
#define LOCAL_HEAP_H

#include <array>
#include <stddef.h>
#include "GlobalHeap.h"
#include "BaseHeap.h"

namespace hoard {


class LocalHeap : public BaseHeap {
private:
  constexpr static size_t kBinCount = 7; //blocks_allocated / size = 0,  <=1/8, <=1/4, <=2/4, <=3/4, <1, 1
  constexpr static size_t kAlmostFullBlocksBinNum = kBinCount - 2; // 3/4 <  blocks_allocated / size < 1
  constexpr static size_t kBellowThresholdSuperblocksBinNum = 1; // 0 <  blocks_allocated / size <= 3/4
  constexpr static size_t kEmptySuperblocksBinNum = 0; // blocks_allocated / size = 0


public:
	LocalHeap(GlobalHeap &parent_heap)
			: BaseHeap(parent_heap.one_block_size()),
        parent_heap_(parent_heap),
				blocks_allocated_(0),
				size_(0),
        superblock_count_(0){
    trace("LocalHeap: ", this, ". Construct.");
  }

	void * Alloc() {
		lock_guard guard(lock);
		if (blocks_allocated_ == size_) {
			GetSuperblock();
    }

		for(size_t i_bin = kAlmostFullBlocksBinNum; i_bin >= 0; --i_bin) {
      SuperblockStack & bin = bins_[i_bin];
			if (!bin.IsEmpty()) {
				SuperblockHeader & header = bin.Top()->header();
				assert(header.owner() == this);
        void * allocated =  header.Alloc();
        const size_t new_bin_num = GetBinNum(header);
        if (new_bin_num != i_bin) {
          Superblock * const superblock = bin.Pop();
					bins_[new_bin_num].Push(superblock);
				}
        trace("LocalHeap: ", this, ". Alloc ", allocated);
        ++blocks_allocated_;
        CheckInvariantsOrDie();
				return allocated;
			}
		}
		fatal_error("no superblock choosed, shouldn't happen");
    return nullptr; // not reached
	}

  virtual void Free(Superblock * superblock, void * ptr) override {
    trace("LocalHeap: ", this, ". Free: ", superblock);
    superblock->header().Free(ptr);
    OnFreeSuperblock(superblock);
  }

  size_t blocks_allocated() const {
    return blocks_allocated_;
  }

  size_t size() const {
    return size_;
  }

  size_t superblock_count() const {
    return superblock_count_;
  }

  GlobalHeap & parent_heap() const {
    return parent_heap_;
  }

protected:

  virtual void OnFreeSuperblock(Superblock* superblock) {
    SuperblockHeader& header = superblock->header();
    assert(header.owner() == this);
    assert(blocks_allocated_ > 0);
    --blocks_allocated_;
    const size_t old_bin_num = GetBinNum(header.blocks_allocated() + 1, header.size());
    bins_[old_bin_num].Remove(superblock);
    const size_t new_bin_num = GetBinNum(header);
    bins_[new_bin_num].Push(superblock);
    while (HeapBellowEmptynessThreshold()) {
      TransferSuperblockToParent();
    }
    CheckInvariantsOrDie();
  }

private:

	GlobalHeap& parent_heap_;
	std::array<SuperblockStack, kBinCount> bins_;
	size_t blocks_allocated_;
	size_t size_;
	size_t superblock_count_;

	void GetSuperblock() {
		Superblock *superblock;
		{
			lock_guard guard(parent_heap_.lock);
			superblock = parent_heap_.GetSuperblock();
			SuperblockHeader& header = superblock->header();
      assert(header.owner() == &parent_heap_);
      header.set_owner(this);
		}

    trace("LocalHeap: ", this, ". GetSuperblock: ", superblock);
		assert(BellowEmptynessThreshold(superblock->header()));

    AcceptSuperblock(superblock);
  }

  void AcceptSuperblock(Superblock * superblock) {
    SuperblockHeader& header = superblock->header();
    trace("BinNum: ", GetBinNum(header));
    assert(header.one_block_size() == one_block_size_);
    bins_[GetBinNum(header)].Push(superblock);
    blocks_allocated_ += header.blocks_allocated();
    size_ += header.size();
    ++superblock_count_;
  }

  // Transfer almost empty superblock to parent
	void TransferSuperblockToParent() {
		for(size_t i_bin = kEmptySuperblocksBinNum; i_bin <= kBellowThresholdSuperblocksBinNum; ++i_bin) {
			SuperblockStack & bin = bins_[i_bin];
			if (!bin.IsEmpty()) {
				Superblock *superblock = bin.Pop();

        trace("LocalHeap: ", this, ". TransferSuperblockToParent: ", superblock);

				SuperblockHeader &header = superblock->header();
				assert(BellowEmptynessThreshold(header)
						&& "Transfered superblock, shoul be bellow emptyness threshold");
				blocks_allocated_ -= header.blocks_allocated();
				size_ -= header.size();
				--superblock_count_;
				{
					lock_guard guard(parent_heap_.lock);
					parent_heap_.AddSuperblock(superblock);
				}
                return;
			}
		}
		fatal_error("no nonempty bins");
	}


  void CheckInvariantsOrDie() {
#ifndef NDEBUG
    size_t current_blocks_allocated_ = 0;
    size_t current_size_ = 0;
    size_t current_superblock_count_ = 0;

    for (size_t i_bin = 0; i_bin < kBinCount; ++i_bin) {
      SuperblockStack& bin = bins_[i_bin];
      for(Superblock * superblock = bin.Top(); superblock != nullptr; superblock = superblock->header().next()) {
        auto& header = superblock->header();
        ++current_superblock_count_;
        current_size_ += header.size();
        current_blocks_allocated_ += header.blocks_allocated();
        assert(header.owner() == this);
        assert(i_bin == GetBinNum(header));
      }
      bin.CheckInvariantsOrDie();
    }


    assert(current_superblock_count_ == superblock_count());
    assert(current_size_ == size());
    assert(current_blocks_allocated_ == blocks_allocated());
    assert(!HeapBellowEmptynessThreshold());
#endif
  }

  bool HeapBellowEmptynessThreshold() {
    return superblock_count_ > kSuperblocksInLocalHeapLowBound && BellowEmptynessThreshold(blocks_allocated_, size_);
  }

  static bool BellowEmptynessThreshold(size_t blocks_allocated, size_t blocks_count) {
    return (blocks_allocated * kEmptynessFactor) / blocks_count == 0;
  }

  static bool BellowEmptynessThreshold(SuperblockHeader & header) {
    return BellowEmptynessThreshold(header.blocks_allocated(), header.size());
  }

  static size_t GetBinNum(SuperblockHeader& header) {
    return GetBinNum(header.blocks_allocated(), header.size());
  }

	static size_t GetBinNum(size_t allocated_blocks, size_t blocks_count) {
		switch (RoundUp(allocated_blocks * 8, blocks_count) / blocks_count) { //TODO change this
			case 0: return 0; // e == 0
			case 1: return 1; // x <= 1/8
			case 2: return 2; // <= 1/4
			case 3:
			case 4: return 3; // <= 2/4
			case 5:
			case 6: return 4; // <= 3/4
			case 7:
			case 8: return allocated_blocks == blocks_count ? 6 : 5;
		}
		assert(false && "invalid bin num");
    std::abort();
  }
};

}
#endif // LOCAL_HEAP_H
