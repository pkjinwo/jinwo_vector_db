# ============================================================
# JinWo VecDB - 根构建配置（本地 C 开发库 + 头文件）
#
# 目的：把纯 C 核心（src/*.c + include/*.h）编译成本地开发库
#       （libjinwo.a / libjinwo.so / libjinwo.dylib / libjinwo.dll）
#       并安装头文件，供本地 C/C++ 开发集成使用。
#
# ⚠️ 命名过渡（2026-10）：本文件产出“已发布的 C/C++ 本地开发库” libjinwo。
#    面向尚未发布语言的跨平台 FFI 底座统一命名为 libjinwo_vecdb，
#    由 .github/workflows/build_native_lib.yml 跨平台编译并发布
#    （产物 libjinwo_vecdb-<平台>.*，供 C#/PHP 等复用）。
#    现有已发布语言（C/C++/Go/Python/Android）沿用 libjinwo，
#    计划在 0.1.35 版本统一切换为 libjinwo_vecdb。
#
# ⚠️⚠️⚠️ 同步提醒（AI 工具 / 维护者必读）⚠️⚠️⚠️
#   本文件与 build_python_pypi/CMakeLists.txt 共享同一份 C 源码清单
#   (JINWO_SOURCES) 与同一套编译宏 / 平台特殊处理。两者只是“同一套
#   源码的两种产物形态”：
#       - 本文件        → 本地 C 开发库 + 头文件（给 C/C++ 集成用）
#       - build_python_pypi → Python 轮子（libjinwo + _jinwo 模块）
#   改动同步清单（任一处修改都必须同步另一处，否则会出现
#   “Python 能编、C 库编不出”或反之的不一致）：
#     1. JINWO_SOURCES 源文件列表（新增/删除 src/*.c）
#     2. target_compile_definitions(jinwo ...) 中的 JW_EXPORTS 等宏
#     3. 平台特殊处理：
#          - WIN32：MSVC 多配置 Release/ 子目录、PREFIX "lib"
#          - Linux：PREFIX "lib"（避免 jinwo.so 与 jinwo.py 冲突）
#          - APPLE：-undefined dynamic_lookup（仅 Python 模块需要）
# ============================================================

cmake_minimum_required(VERSION 3.15)
project(jinwo_vecdb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 仓库根目录（本 CMakeLists 位于根目录，故直接取 SOURCE_DIR）
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

include_directories(${PROJECT_ROOT}/include)

# ============================================================
# C 源文件清单 —— 必须与 build_python_pypi/CMakeLists.txt 的
# JINWO_SOURCES 保持一致（同步提醒见文件顶部）。
# ============================================================
set(JINWO_SOURCES
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
    ${PROJECT_ROOT}/src/jw_vector.c
)

# ============================================================
# 共享库（.so / .dylib / .dll）—— 命名为 libjinwo，作为已发布的
# C/C++ 本地开发库（与 Go 静态库 libjinwo.a、Python 轮子 libjinwo 同名）。
# 产出文件：Linux libjinwo.so / macOS libjinwo.dylib / Windows libjinwo.dll
#
# ⚠️ 跨平台 FFI 底座（C#/PHP 等尚未发布语言）请改用 libjinwo_vecdb，
#    由 .github/workflows/build_native_lib.yml 统一产出，0.1.35 起本地库也将改名。
# ============================================================
add_library(jinwo SHARED ${JINWO_SOURCES})
target_compile_definitions(jinwo PRIVATE JW_EXPORTS)
set_target_properties(jinwo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME jinwo
    PREFIX "lib"  # libjinwo.*（Windows 亦保持 lib 前缀，避免与 jinwo.py 冲突）
)

# ============================================================
# 静态库（.a / .lib）—— 本地 C 开发集成常用；Go binding 也链接 libjinwo.a
# 目标名固定为 jw_vecdb_static，与 tests/CMakeLists.txt 及
# build_python_pypi/CMakeLists.txt 保持一致（同步提醒见文件顶部）。
# ============================================================
add_library(jw_vecdb_static STATIC ${JINWO_SOURCES})
target_compile_definitions(jw_vecdb_static PRIVATE JW_EXPORTS)
set_target_properties(jw_vecdb_static PROPERTIES PREFIX "")

# ============================================================
# 安装：库文件 + 头文件
#   cmake --install build --prefix <dir>
#   安装后：<prefix>/lib/libjinwo.* , <prefix>/include/*.h
# ============================================================
install(TARGETS jinwo jw_vecdb_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${PROJECT_ROOT}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================
# 可选：构建 tests/ 下的单元测试（ctest）
#   用法：cmake -S . -B build -DJW_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build
#   与 build_python_pypi 的测试体系对齐；tests/CMakeLists.txt 链接 jw_vecdb_static。
# ============================================================
option(JW_BUILD_TESTS "Build unit tests under tests/" OFF)
if(JW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
