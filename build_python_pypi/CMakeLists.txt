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
find_package(Python COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python extension suffix: ${Python_EXTENSION_SUFFIX}")
message(STATUS "Python library: ${Python_LIBRARIES}")
message(STATUS "Python include dir: ${Python_INCLUDE_DIRS}")

# 设置输出目录（由 setup.py 通过 -DCMAKE_LIBRARY_OUTPUT_DIRECTORY 传入）
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# 构建动态库 (静态链接到 Python 扩展)
add_library(jinwo STATIC ${JINWO_SOURCES})
set_target_properties(jinwo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)
# ============================================================
# 2026-05-21: 修复 Windows 编译错误
# 原因: STATIC 库不应该使用 dllexport/dllimport
#       需要定义 JW_STATIC 让头文件知道正在构建静态库
# ============================================================
target_compile_definitions(jinwo PRIVATE JW_STATIC)

# 构建 Python 绑定
add_library(_jinwo MODULE ${PYBIND_SOURCES})
target_link_libraries(_jinwo jinwo Python::Python)
target_include_directories(_jinwo PRIVATE ${Python_INCLUDE_DIRS})

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
