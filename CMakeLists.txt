cmake_minimum_required(VERSION 3.3)
project(gc_1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        mark_and_sweep/gc_ms.c
        mark_and_sweep/gc_ms.h
        mark_and_sweep/ms_test.c
#        mark_not_sweep/gc_mns.c
#        mark_not_sweep/gc_mns.h
#        mark_not_sweep/mns_test.c
)

add_executable(gc_1 ${SOURCE_FILES})
