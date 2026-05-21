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


class CMakeExtension(Extension):
    """使用 CMake 构建的扩展"""

    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuild(build_ext):
    """CMake 构建命令"""

    def build_extension(self, ext: CMakeExtension) -> None:
        build_temp = Path(self.build_temp).resolve()
        build_lib = Path(self.build_lib).resolve()
        build_temp.mkdir(parents=True, exist_ok=True)
        build_lib.mkdir(parents=True, exist_ok=True)

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH={build_lib}",
            f"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH={build_lib}",
            f"-DPython_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]

        if sys.platform.startswith("darwin"):
            cmake_args += ["-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"]
        elif sys.platform == "win32":
            cmake_args += ["-G", "Visual Studio 17 2022", "-A", "x64"]

        build_cmd = ["cmake", str(ext.sourcedir)] + cmake_args
        print(f"Running cmake from: {build_temp}")
        print(f"Output directory: {build_lib}")
        subprocess.run(build_cmd, cwd=build_temp, check=True)

        # ============================================================
        # 2026-05-21: 修复 Windows MSBuild 并行编译参数
        # MSBuild 使用 /m 而不是 -j
        # ============================================================
        if sys.platform == "win32":
            build_cmd = ["cmake", "--build", ".", "--config", "Release", "--", "/m"]
        else:
            build_cmd = ["cmake", "--build", ".", "--", "-j4"]
        subprocess.run(build_cmd, cwd=build_temp, check=True)
        
        # 确保扩展文件在正确位置
        ext_path = self.get_ext_fullpath(ext.name)
        print(f"Extension path: {ext_path}")
        
        # 调试：检查输出文件
        print(f"Output directory contents: {list(build_lib.rglob('*'))}")


ext_modules = [
    CMakeExtension("jinwo_vecdb._jinwo"),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
