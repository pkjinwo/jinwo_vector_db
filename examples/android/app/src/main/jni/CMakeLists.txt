cmake_minimum_required(VERSION 3.22.1)

project("jinwo_vecdb_android_demo")

set(JW_VECDB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

add_library(jw_vecdb SHARED
    ${JW_VECDB_SOURCE_DIR}/src/jw_vecdb.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_collection.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_vector.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_index.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_memory.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_storage.c
    ${JW_VECDB_SOURCE_DIR}/src/jw_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jni/android_demo_jni.cpp
)

target_include_directories(jw_vecdb PRIVATE
    ${JW_VECDB_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
)

find_library(log-lib log)
target_link_libraries(jw_vecdb ${log-lib})
