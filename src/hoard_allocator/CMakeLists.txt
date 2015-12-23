add_subdirectory(dummy/)
add_subdirectory(hoard_common/)

include_directories(hoard_common/)

add_library(hoard_core
  hoard_common/AllocFreeHashMap.h
  BaseHeap.h
  Block.h
        BlockLinkedStack.h
  FreeSuperblockManager.h
  GlobalHeap.h
  Internals.h
  Internals.cpp
  LocalHeap.h
  SuperblockHeader.h
  Superblock.h
  SuperblockStack.h
  HoardState.h
)
target_link_libraries(hoard_core
  hoard_common
)
target_include_directories(hoard_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(malloc_intercept SHARED
  MallocIntercept.cpp
)

target_link_libraries(malloc_intercept
  hoard_core
)
