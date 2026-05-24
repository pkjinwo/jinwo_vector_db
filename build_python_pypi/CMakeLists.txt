cmake_minimum_required(VERSION 3.15)
project(jinwo_vecdb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================
# 2026-05-20: 修复 Windows 编译错误
# 原因: BUILD_SHARED_LIBS ON 与 add_library(jinwo STATIC ...) 冲突
#       导致所有函数被标记为 dllimport，但 STATIC 库不应该有导出
# 备份: 
# if(WIN32)
#     set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
#     set(BUILD_SHARED_LIBS ON)
# endif()
# ============================================================

# 项目根目录（相对于 build_python 目录）
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/..)

include_directories(${PROJECT_ROOT}/include)

# C 源文件
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

# Python 绑定源文件
set(PYBIND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/jinwo_vecdb/py_jinwo/py_jinwo.c
)

# 获取 Python 扩展名和目录
# Development.Module: 构建 Python 扩展模块（不需要 Python_LIBRARIES）
# 2026-05-24: Linux (manylinux) 强制优先查找 .so（共享库），
#             避免 CMake 默认找 .a（静态库）嵌入 libpython → SIGSEGV
if(NOT APPLE AND NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a")
endif()
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
if(NOT APPLE AND NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP})
endif()
message(STATUS "Python extension suffix: ${Python_SOABI}")
message(STATUS "Python include dir: ${Python_INCLUDE_DIRS}")

# 设置输出目录（由 setup.py 通过 -DCMAKE_LIBRARY_OUTPUT_DIRECTORY 传入）
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# ============================================================
# 2026-05-24: 修复 Windows ctypes 找不到 jw_* 函数的问题
# 原因: STATIC + JW_STATIC 导致 jw_* 函数在 .pyd 中不导出
#       改用 OBJECT 库 + JW_EXPORTS，使 ctypes 能加载到符号
# 备注: JW_EXPORTS 仅影响 Windows (dllexport)，macOS JW_API 不受影响
# ============================================================
add_library(jinwo OBJECT ${JINWO_SOURCES})
set_target_properties(jinwo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(jinwo PRIVATE JW_EXPORTS)

# 构建 Python 绑定
add_library(_jinwo MODULE ${PYBIND_SOURCES} $<TARGET_OBJECTS:jinwo>)

# 2026-05-24: 在所有平台链接 Python::Module
# - Windows: 需要导入库解析 Python API 符号
# - Linux:   find_package 前设 .so 优先，确保动态链接共享 libpython
#            （避免静态嵌入导致符号冲突 SIGSEGV）
# - macOS:   -undefined dynamic_lookup 覆盖，符号运行时解析
target_link_libraries(_jinwo Python::Module)

target_include_directories(_jinwo PRIVATE ${Python_INCLUDE_DIRS})
target_compile_definitions(_jinwo PRIVATE JW_EXPORTS)

# 设置 Python 模块输出名称
set_target_properties(_jinwo PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

if(APPLE)
    set_target_properties(_jinwo PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()
