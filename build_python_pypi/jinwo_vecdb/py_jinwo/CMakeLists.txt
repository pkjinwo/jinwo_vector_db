cmake_minimum_required(VERSION 3.15)
project(jinwo_vecdb_python)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

set(SOURCES
    py_jinwo.c
    ../../src/jw_arena.c
    ../../src/jw_collection.c
    ../../src/jw_config.c
    ../../src/jw_error.c
    ../../src/jw_file.c
    ../../src/jw_hash.c
    ../../src/jw_index.c
    ../../src/jw_lock.c
    ../../src/jw_log.c
    ../../src/jw_math.c
    ../../src/jw_quant.c
    ../../src/jw_sort.c
    ../../src/jw_stdio.c
    ../../src/jw_storage.c
    ../../src/jw_string.c
    ../../src/jw_types.c
    ../../src/jw_vecdb.c
)

add_library(jinwo_vecdb._jinwo MODULE ${SOURCES})
target_link_libraries(jinwo_vecdb._jinwo m)
