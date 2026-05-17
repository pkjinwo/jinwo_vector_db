cmake_minimum_required(VERSION 3.15)
project(jinwo_vecdb_python)

set(CMAKE_C_STANDARD 99)

# 获取项目根目录（build_python_pypi 的上一级）
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

include_directories(${PROJECT_ROOT}/include)
include_directories(${PROJECT_ROOT}/src)

set(SOURCES
    py_jinwo.c
    ${PROJECT_ROOT}/src/jw_arena.c
    ${PROJECT_ROOT}/src/jw_collection.c
    ${PROJECT_ROOT}/src/jw_config.c
    ${PROJECT_ROOT}/src/jw_error.c
    ${PROJECT_ROOT}/src/jw_file.c
    ${PROJECT_ROOT}/src/jw_hash.c
    ${PROJECT_ROOT}/src/jw_index.c
    ${PROJECT_ROOT}/src/jw_lock.c
    ${PROJECT_ROOT}/src/jw_log.c
    ${PROJECT_ROOT}/src/jw_math.c
    ${PROJECT_ROOT}/src/jw_quant.c
    ${PROJECT_ROOT}/src/jw_sort.c
    ${PROJECT_ROOT}/src/jw_stdio.c
    ${PROJECT_ROOT}/src/jw_storage.c
    ${PROJECT_ROOT}/src/jw_string.c
    ${PROJECT_ROOT}/src/jw_types.c
    ${PROJECT_ROOT}/src/jw_vecdb.c
)

add_library(jinwo_vecdb._jinwo MODULE ${SOURCES})
target_link_libraries(jinwo_vecdb._jinwo m)
