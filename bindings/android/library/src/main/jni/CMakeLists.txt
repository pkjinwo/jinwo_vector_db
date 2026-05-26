cmake_minimum_required(VERSION 3.22.1)
project("jinwo_vecdb")

# JinWo VecDB C sources (relative to repo root)
set(JW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)

set(JW_SRCS
    ${JW_ROOT}/src/jw_types.c
    ${JW_ROOT}/src/jw_string.c
    ${JW_ROOT}/src/jw_stdio.c
    ${JW_ROOT}/src/jw_hash.c
    ${JW_ROOT}/src/jw_sort.c
    ${JW_ROOT}/src/jw_math.c
    ${JW_ROOT}/src/jw_error.c
    ${JW_ROOT}/src/jw_log.c
    ${JW_ROOT}/src/jw_file.c
    ${JW_ROOT}/src/jw_lock.c
    ${JW_ROOT}/src/jw_config.c
    ${JW_ROOT}/src/jw_arena.c
    ${JW_ROOT}/src/jw_vector.c
    ${JW_ROOT}/src/jw_quant.c
    ${JW_ROOT}/src/jw_index.c
    ${JW_ROOT}/src/jw_collection.c
    ${JW_ROOT}/src/jw_storage.c
    ${JW_ROOT}/src/jw_vecdb.c
)

add_library(jinwo_vecdb SHARED
    jinwo_vecdb_jni.cpp
    ${JW_SRCS}
)

target_include_directories(jinwo_vecdb PRIVATE
    ${JW_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)
target_link_libraries(jinwo_vecdb ${log-lib})
