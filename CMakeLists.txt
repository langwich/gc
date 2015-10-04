cmake_minimum_required(VERSION 3.3)
project(gc_1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    mark-and-sweep/gc_ms.c
    mark-and-sweep/gc_ms.h
    mark-and-sweep/ms_test.c)

add_executable(gc_1 ${SOURCE_FILES})