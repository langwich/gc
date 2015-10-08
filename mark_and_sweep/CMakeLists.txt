cmake_minimum_required(VERSION 3.3)
project(GC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
   gc_ms.c
   gc_ms.h
   ms_test.c)
add_executable(GC ${SOURCE_FILES})
