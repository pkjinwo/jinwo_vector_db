"""
JinWo VecDB - 构建脚本

用于 cibuildwheel 编译 C 扩展
"""

import os
import sys
import sysconfig
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

        python_include = sysconfig.get_path('include')

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH={build_lib}",
            f"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH={build_lib}",
            f"-DPython_EXECUTABLE={sys.executable}",
            f"-DPython_ROOT_DIR={sys.prefix}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]
        if python_include and os.path.isdir(python_include):
            cmake_args.insert(-1, f"-DPython_INCLUDE_DIR={python_include}")
        else:
            print(f"Python include not found: {python_include}")

        if sys.platform.startswith("darwin"):
            cmake_args += ["-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"]
        elif sys.platform == "win32":
            cmake_args += ["-G", "Visual Studio 17 2022", "-A", "x64"]

        def _run_cmake(cmd, cwd, step_name):
            """运行 cmake，成功时简洁输出，失败时打印详细错误"""
            result = subprocess.run(cmd, cwd=cwd,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True)
            if result.returncode != 0:
                # 提取含 error/fatal/not found 的行，确保出现在日志末尾
                lines = result.stdout.splitlines()
                keywords = ('error', 'fatal', 'not found', 'could not', 'missing')
                key_lines = [l for l in lines if any(k in l.lower() for k in keywords)]
                if not key_lines:
                    key_lines = lines[-20:]
                print("--- cmake {} FAILED (exit {}) ---".format(step_name, result.returncode))
                for l in key_lines:
                    print(l)
                sys.exit(1)  # 不用 raise，避免长 traceback 挤掉关键错误信息
            print("cmake {} OK".format(step_name))

        build_cmd = ["cmake", str(ext.sourcedir)] + cmake_args
        print("cmake configure: {} -> {}".format(ext.sourcedir, build_temp))
        _run_cmake(build_cmd, build_temp, "configure")

        # ============================================================
        # 2026-05-21: 修复 Windows MSBuild 并行编译参数
        # MSBuild 使用 /m 而不是 -j
        # ============================================================
        if sys.platform == "win32":
            build_cmd = ["cmake", "--build", ".", "--config", "Release", "--", "/m"]
        else:
            build_cmd = ["cmake", "--build", ".", "--", "-j4"]
        _run_cmake(build_cmd, build_temp, "build")
        
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
