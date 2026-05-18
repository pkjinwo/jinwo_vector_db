"""
JinWo VecDB - 构建脚本

用于 cibuildwheel 编译 C 扩展
"""

import os
from pathlib import Path
from setuptools import setup, Extension


# 获取项目根目录
ROOT_DIR = Path(__file__).parent.parent.resolve()
SRC_DIR = ROOT_DIR / "src"
INCLUDE_DIR = ROOT_DIR / "include"

# C 源文件列表
SOURCE_FILES = [
    str(SRC_DIR / "jw_arena.c"),
    str(SRC_DIR / "jw_collection.c"),
    str(SRC_DIR / "jw_config.c"),
    str(SRC_DIR / "jw_error.c"),
    str(SRC_DIR / "jw_file.c"),
    str(SRC_DIR / "jw_hash.c"),
    str(SRC_DIR / "jw_index.c"),
    str(SRC_DIR / "jw_lock.c"),
    str(SRC_DIR / "jw_log.c"),
    str(SRC_DIR / "jw_math.c"),
    str(SRC_DIR / "jw_quant.c"),
    str(SRC_DIR / "jw_sort.c"),
    str(SRC_DIR / "jw_stdio.c"),
    str(SRC_DIR / "jw_storage.c"),
    str(SRC_DIR / "jw_string.c"),
    str(SRC_DIR / "jw_types.c"),
    str(SRC_DIR / "jw_vecdb.c"),
]

# Python 绑定源文件
BINDING_SOURCE = str(Path(__file__).parent / "jinwo_vecdb" / "py_jinwo" / "py_jinwo.c")

ext_modules = [
    Extension(
        "jinwo_vecdb._jinwo",
        sources=[BINDING_SOURCE] + SOURCE_FILES,
        include_dirs=[str(INCLUDE_DIR)],
        libraries=["m"],
        extra_compile_args=["-O2"],
    ),
]

setup(
    ext_modules=ext_modules,
    zip_safe=False,
)
