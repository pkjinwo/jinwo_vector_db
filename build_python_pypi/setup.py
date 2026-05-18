"""
JinWo VecDB - 构建脚本

用于 cibuildwheel 编译 C 扩展
"""

import os
import sys
import subprocess
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# 相对于 setup.py 的路径
BASE_DIR = Path(__file__).parent.resolve()

# 相对路径（相对于 setup.py 所在目录）
REL_SRC_DIR = "../../src"
REL_INCLUDE_DIR = "../../include"
REL_BINDING = "jinwo_vecdb/py_jinwo/py_jinwo.c"

# C 源文件列表（相对路径）
SOURCE_FILES = [
    f"{REL_SRC_DIR}/jw_arena.c",
    f"{REL_SRC_DIR}/jw_collection.c",
    f"{REL_SRC_DIR}/jw_config.c",
    f"{REL_SRC_DIR}/jw_error.c",
    f"{REL_SRC_DIR}/jw_file.c",
    f"{REL_SRC_DIR}/jw_hash.c",
    f"{REL_SRC_DIR}/jw_index.c",
    f"{REL_SRC_DIR}/jw_lock.c",
    f"{REL_SRC_DIR}/jw_log.c",
    f"{REL_SRC_DIR}/jw_math.c",
    f"{REL_SRC_DIR}/jw_quant.c",
    f"{REL_SRC_DIR}/jw_sort.c",
    f"{REL_SRC_DIR}/jw_stdio.c",
    f"{REL_SRC_DIR}/jw_storage.c",
    f"{REL_SRC_DIR}/jw_string.c",
    f"{REL_SRC_DIR}/jw_types.c",
    f"{REL_SRC_DIR}/jw_vecdb.c",
]

ext_modules = [
    Extension(
        "jinwo_vecdb._jinwo",
        sources=[REL_BINDING] + SOURCE_FILES,
        include_dirs=[REL_INCLUDE_DIR],
        libraries=["m"] if sys.platform != "win32" else [],
        extra_compile_args=["-O2"],
    ),
]

setup(
    ext_modules=ext_modules,
    zip_safe=False,
)
