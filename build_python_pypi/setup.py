"""
JinWo VecDB - 构建脚本

用于 cibuildwheel 编译 C 扩展
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext


# 相对于 setup.py 的路径
BASE_DIR = Path(__file__).parent.resolve()

import atexit

# Stage source/include into package directory to ensure sdist contains C files
SOURCE_ROOT = BASE_DIR.parent / "src"
INCLUDE_ROOT = BASE_DIR.parent / "include"
STAGE_SRC = BASE_DIR / "src"
STAGE_INCLUDE = BASE_DIR / "include"

def safe_remove(path):
    try:
        if path.is_symlink() or path.is_file():
            path.unlink()
        elif path.exists():
            shutil.rmtree(path)
    except Exception:
        pass

def stage_tree():
    if SOURCE_ROOT.exists():
        safe_remove(STAGE_SRC)
        shutil.copytree(SOURCE_ROOT, STAGE_SRC)
    if INCLUDE_ROOT.exists():
        safe_remove(STAGE_INCLUDE)
        shutil.copytree(INCLUDE_ROOT, STAGE_INCLUDE)

def cleanup_stage():
    safe_remove(STAGE_SRC)
    safe_remove(STAGE_INCLUDE)

stage_tree()
atexit.register(cleanup_stage)

# Use POSIX-style relative paths (relative to setup.py) to satisfy setuptools
REL_BINDING = "jinwo_vecdb/py_jinwo/py_jinwo.c"

# C 源文件列表（相对于 setup.py 的相对路径字符串）
SOURCE_FILES = [
    "src/jw_arena.c",
    "src/jw_collection.c",
    "src/jw_config.c",
    "src/jw_error.c",
    "src/jw_file.c",
    "src/jw_hash.c",
    "src/jw_index.c",
    "src/jw_lock.c",
    "src/jw_log.c",
    "src/jw_math.c",
    "src/jw_quant.c",
    "src/jw_sort.c",
    "src/jw_stdio.c",
    "src/jw_storage.c",
    "src/jw_string.c",
    "src/jw_types.c",
    "src/jw_vecdb.c",
]

ext_compile_args = ["-O2"]
define_macros = []
if sys.platform == "win32":
    ext_compile_args = ["/O2"]
    define_macros = [("JW_EXPORTS", None)]

ext_modules = [
    Extension(
        "jinwo_vecdb._jinwo",
        sources=[REL_BINDING] + SOURCE_FILES,
        include_dirs=["include"],
        libraries=["m"] if sys.platform != "win32" else [],
        extra_compile_args=ext_compile_args,
        define_macros=define_macros,
    ),
]

from setuptools.command.egg_info import egg_info as _egg_info

class clean_egg_info(_egg_info):
    def run(self):
        super().run()
        try:
            egg_dir = self.egg_info
            sources_txt = os.path.join(egg_dir, 'SOURCES.txt')
            if os.path.exists(sources_txt):
                with open(sources_txt, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
                cleaned = []
                for ln in lines:
                    s = ln.strip()
                    # skip absolute paths and parent-directory references
                    if s.startswith('/') or s.startswith('\\') or '..' in s:
                        continue
                    cleaned.append(ln)
                with open(sources_txt, 'w', encoding='utf-8') as f:
                    f.writelines(cleaned)
        except Exception:
            pass

setup(
    name="jinwo_vecdb",
    packages=find_packages(include=["jinwo_vecdb*"]),
    ext_modules=ext_modules,
    zip_safe=False,
    cmdclass={'egg_info': clean_egg_info},
)
