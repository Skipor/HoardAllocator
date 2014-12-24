#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include <mutex>
#include <atomic>
#include <assert.h>
#include "hoard_constants.h"
#include "BaseHeap.h"
#include "SuperblockHeader.h"

namespace hoard {

class Superblock {

public:
	Superblock() {
	};

	static Superblock *Get(void *ptr) {
		return reinterpret_cast<Superblock *>(reinterpret_cast<size_t>(ptr) & (kSuperblockSize - 1));
	}

private:
	SuperblockHeader header_;
	constexpr static size_t kDataSize = kSuperblockSize - sizeof(SuperblockHeader);
	char data[kDataSize];

public:
	SuperblockHeader &header() {
		return header_;
	};

};

static_assert(sizeof(Superblock) == kSuperblockSize, "ivalid superblock struct size");
}
#endif // SUPERBLOCK_H