#ifndef SUPERBLOCK_STACK_H
#define SUPERBLOCK_STACK_H

#include "Superblock.h"

namespace hoard {


class SuperblockStack  {

public:
	SuperblockStack() : head_(nullptr), size_(0) {
	}

	SuperblockStack(const SuperblockStack &) = delete;
	SuperblockStack & operator=(const SuperblockStack &) = delete;

	void Push(Superblock *t) {
    t->header().set_next(head_);
    t->header().set_prev(nullptr);
    if(head_) {
      head_->header().set_prev(t);
    }
		head_ = t;
    ++size_;
	}

	Superblock *Pop() {
		assert(!IsEmpty());
		Superblock * const t = head_;
    head_ = head_->header().next();
    if(head_) {
      head_->header().set_prev(nullptr);
    }
    t->header().set_next(nullptr);
    t->header().set_prev(nullptr);
    --size_;
		return t;
	}

	Superblock * Top() {
		return head_;
	}

	bool IsEmpty() {
		return head_ == nullptr;
	}

  void Reset() {
    head_ = nullptr;
    size_ = 0;
  }

  bool Contains(Superblock * element) {
    auto * current = head_;
    while (current != nullptr) {
      if (element == current) {
        return true;
      }
      current = current->header().next();
    }
    return false;
  }

  size_t Size() {
    return size_;
  }

  Superblock * Next(Superblock * superblock) {
    return  superblock->header().next();
  }

  void CheckInvariantsOrDie() {
#ifndef NDEBUG
    size_t current_size = 0;
    auto * current = head_;
    while (current != nullptr) {
      ++current_size;
      current = Next(current);
    }
    check_fatal(current_size == size_, "current size: ", current_size, ", but ", size_, " expected");
#endif
  }


	void Remove(Superblock *superblock) {
    trace("stack remove");
#ifndef NDEBUG
    // O(n) check, that superblock is in this stack
    assert(Contains(superblock) && "no such element in Stack");
#endif //debug
    Superblock * const prev = superblock->header().prev();
    Superblock * const next = superblock->header().next();
    if (head_ == superblock) {
      head_ = head_->header().next();
    }
		if(prev != nullptr) {
      prev->header().set_next(next);
    }
		if(next != nullptr) {
      next->header().set_prev(prev);
    }
    --size_;
    superblock->header().set_next(nullptr);
    superblock->header().set_prev(nullptr);
    CheckInvariantsOrDie();
    trace("stack after remove");
	}

protected:
  Superblock *head_;
  size_t size_;

};

}
#endif // SUPERBLOCK_STACK_H
