project(Hoard-Allocator)
cmake_minimum_required(VERSION 2.8.11)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -fPIC -Wall -pedantic")
#set(CMAKE_BUILD_TYPE "debug")
set(CMAKE_BUILD_TYPE "release")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -march=native -O3") #compiler optimisation flags

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

enable_testing()
add_subdirectory(src/)


