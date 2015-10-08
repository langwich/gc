cmake_minimum_required(VERSION 3.3)
project(GC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
   gc_mns.c
   gc_mns.h
   mns_test.c)
add_executable(GC ${SOURCE_FILES})
