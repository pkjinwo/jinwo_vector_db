# CMakeLists.txt - JinWo VecDB 构建配置
#
# Copyright 2026 北京金幄科技有限公司
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0

cmake_minimum_required(VERSION 3.10)
project(jw_vecdb 
    VERSION 0.1.0
    DESCRIPTION "JinWo VecDB - 嵌入式向量数据库"
    LANGUAGES C
)

# 项目选项
option(JW_BUILD_SHARED "Build shared library" ON)
option(JW_BUILD_STATIC "Build static library" ON)
option(JW_BUILD_EXAMPLES "Build example programs" OFF)
option(JW_BUILD_TESTS "Build test programs" OFF)
option(JW_ENABLE_SIMD "Enable SIMD optimizations" OFF)
option(JW_ENABLE_DEBUG "Enable debug output" OFF)

# C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
    if(JW_ENABLE_DEBUG)
        add_compile_options(/Zi)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(JW_ENABLE_DEBUG)
        add_compile_options(-g -O0 -DJW_DEBUG)
    else()
        add_compile_options(-O2)
    endif()
endif()

# SIMD支持
if(JW_ENABLE_SIMD)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i586")
        if(MSVC)
            add_compile_options(/arch:AVX2)
        else()
            add_compile_options(-march=native -msse4.2 -mavx2)
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
        if(NOT MSVC)
            add_compile_options(-mfpu=neon)
        endif()
    endif()
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(JW_VECDB_SOURCES
    src/jw_types.c
    src/jw_hash.c
    src/jw_arena.c
    src/jw_vector.c
    src/jw_lock.c
    src/jw_index.c
    src/jw_collection.c
    src/jw_storage.c
    src/jw_vecdb.c
    src/jw_config.c
    src/jw_file.c
    src/jw_log.c
    src/jw_math.c
    src/jw_sort.c
    src/jw_stdio.c
    src/jw_string.c
    src/jw_quant.c
)

# 库目标
if(JW_BUILD_SHARED)
    add_library(jw_vecdb SHARED ${JW_VECDB_SOURCES})
    target_compile_definitions(jw_vecdb PRIVATE JW_EXPORTS)
    set_target_properties(jw_vecdb PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME jwvecdb
    )
endif()

if(JW_BUILD_STATIC)
    add_library(jw_vecdb_static STATIC ${JW_VECDB_SOURCES})
    set_target_properties(jw_vecdb_static PROPERTIES
        OUTPUT_NAME jwvecdb
    )
endif()

# 平台特定链接库
if(NOT WIN32)
    find_library(PTHREAD_LIBRARY pthread)
    if(PTHREAD_LIBRARY)
        if(JW_BUILD_SHARED)
            target_link_libraries(jw_vecdb PRIVATE ${PTHREAD_LIBRARY})
        endif()
        if(JW_BUILD_STATIC)
            target_link_libraries(jw_vecdb_static PRIVATE ${PTHREAD_LIBRARY})
        endif()
    endif()
    
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        if(JW_BUILD_SHARED)
            target_link_libraries(jw_vecdb PRIVATE ${MATH_LIBRARY})
        endif()
        if(JW_BUILD_STATIC)
            target_link_libraries(jw_vecdb_static PRIVATE ${MATH_LIBRARY})
        endif()
    endif()
endif()

# 示例程序
if(JW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试程序
if(JW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装规则
include(GNUInstallDirs)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jwvecdb
    FILES_MATCHING PATTERN "*.h"
)

if(JW_BUILD_SHARED)
    install(TARGETS jw_vecdb
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(JW_BUILD_STATIC)
    install(TARGETS jw_vecdb_static
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# 生成配置文件
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/jw_vecdb_config.h.in
    ${CMAKE_BINARY_DIR}/jw_vecdb_config.h
)

# 包配置文件
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/jw_vecdb.pc.in
    ${CMAKE_BINARY_DIR}/jw_vecdb.pc
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/jw_vecdb.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# 打印配置摘要
message(STATUS "")
message(STATUS "JinWo VecDB Configuration Summary")
message(STATUS "================================")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library:   ${JW_BUILD_SHARED}")
message(STATUS "  Static library:   ${JW_BUILD_STATIC}")
message(STATUS "  Examples:         ${JW_BUILD_EXAMPLES}")
message(STATUS "  Tests:            ${JW_BUILD_TESTS}")
message(STATUS "  SIMD:             ${JW_ENABLE_SIMD}")
message(STATUS "  Debug:            ${JW_ENABLE_DEBUG}")
message(STATUS "  Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
