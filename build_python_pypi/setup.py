"""
JinWo VecDB - 构建脚本

用于 cibuildwheel 编译 C 扩展
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# 相对于 setup.py 的路径
BASE_DIR = Path(__file__).parent.resolve()

# 相对路径（相对于 setup.py 所在目录）
REL_SRC_DIR = BASE_DIR / "src"
REL_INCLUDE_DIR = BASE_DIR / "include"
REL_BINDING = (BASE_DIR / "jinwo_vecdb" / "py_jinwo" / "py_jinwo.c").relative_to(BASE_DIR)

# C 源文件列表（相对于 setup.py 的相对路径）
SOURCE_FILES = [
    str((REL_SRC_DIR / "jw_arena.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_collection.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_config.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_error.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_file.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_hash.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_index.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_lock.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_log.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_math.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_quant.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_sort.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_stdio.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_storage.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_string.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_types.c").relative_to(BASE_DIR)),
    str((REL_SRC_DIR / "jw_vecdb.c").relative_to(BASE_DIR)),
]

ext_modules = [
    Extension(
        "jinwo_vecdb._jinwo",
        sources=[str(REL_BINDING)] + SOURCE_FILES,
        include_dirs=[str(REL_INCLUDE_DIR)],
        libraries=["m"] if sys.platform != "win32" else [],
        extra_compile_args=["-O2"],
    ),
]

setup(
    ext_modules=ext_modules,
    zip_safe=False,
)
