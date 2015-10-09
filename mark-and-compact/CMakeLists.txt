cmake_minimum_required(VERSION 3.1)
project(mark_compact)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    gc.c
    gc.h
    misc.c
    misc.h
    test.c)

add_executable(mark_compact ${SOURCE_FILES})
