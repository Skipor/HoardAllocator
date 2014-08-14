project(allocator)
cmake_minimum_required(VERSION 2.8)
find_package(Boost COMPONENTS filesystem system REQUIRED)
aux_source_directory(src SRC_LIST)
add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
message("my info boost inc dirs is :" ${Boost_INCLUDE_DIR})
message("my info boost lib dir" ${Boost_LIBRARY_DIR})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${warnings}")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
message("cxx flags:" ${CMAKE_CXX_FLAGS})
message("cxx flags debug:" ${CMAKE_CXX_FLAGS_DEBUG})
message("cxx flags release:" ${CMAKE_CXX_FLAGS_RELEASE})

