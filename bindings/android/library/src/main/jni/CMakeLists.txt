cmake_minimum_required(VERSION 3.22.1)
project("jinwo")

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

# 全项目统一库名：libjinwo（不允许第二套名字）
add_library(jinwo SHARED
    jinwo_vecdb_jni.cpp
    ${JW_SRCS}
)

target_include_directories(jinwo PRIVATE
    ${JW_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(log-lib log)
target_link_libraries(jinwo ${log-lib})
