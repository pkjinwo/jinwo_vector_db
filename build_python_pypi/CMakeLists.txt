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
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
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
# 2026-05-24: 分离纯 C 库和 Python 模块，解决 Linux/macOS SIGSEGV
#
# 问题: 之前 _jinwo.so 同时包含纯 C 函数和 Python C API 代码。
#       ctypes.CDLL 加载时，Python API 符号嵌入静态 libpython
#       与运行中解释器冲突 → SIGSEGV。或不链接则符号未定义。
#
# 方案: jinwo  → SHARED 纯 C 库 (ctypes 加载, 无 Python 依赖)
#       _jinwo → MODULE Python 扩展 (import 使用, 有 Python::Module)
# ============================================================
add_library(jinwo SHARED ${JINWO_SOURCES})
target_compile_definitions(jinwo PRIVATE JW_EXPORTS)
set_target_properties(jinwo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""  # 去掉 lib 前缀，统一文件名
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)
if(WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set_target_properties(jinwo PROPERTIES
        PREFIX "lib"  # Windows/Linux: libjinwo.*, 避免 jinwo.so 与 jinwo.py 冲突
    )
endif()

# 构建 Python 绑定模块（仅 py_jinwo.c, 包含 Python C API）
add_library(_jinwo MODULE ${PYBIND_SOURCES})
target_link_libraries(_jinwo jinwo Python::Module)
target_include_directories(_jinwo PRIVATE ${Python_INCLUDE_DIRS})
target_compile_definitions(_jinwo PRIVATE JW_EXPORTS)
set_target_properties(_jinwo PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# 2026-06-02: 修复 Windows MSVC 多配置生成器 Release/ 子目录问题
# MSVC 默认将输出放到 Release/ 子目录，导致 DLL 不被 setuptools 打包进 wheel
if(WIN32)
    set_target_properties(jinwo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
    set_target_properties(_jinwo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
endif()

if(APPLE)
    set_target_properties(_jinwo PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()
