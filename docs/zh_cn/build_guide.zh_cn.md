# JinWo VecDB 本地构建指南（C 开发库与头文件）

本文说明如何把 JinWo VecDB 的纯 C 核心（`src/*.c` + `include/*.h`）编译成本地
开发库与头文件，供你在自己的 C/C++ 项目中集成、或基于源码二次开发。

> 本文档对应英文版：`docs/en_us/build_guide.en_us.md`
> 构建入口为仓库根目录 `CMakeLists.txt`，与 `build_python_pypi/CMakeLists.txt`
> 共享同一份 C 源码清单，请勿单独维护。

---

## 1. 构建产物

| 类型 | 文件 |
| --- | --- |
| 共享库（Linux） | `libjinwo.so` |
| 共享库（macOS） | `libjinwo.dylib` |
| 共享库（Windows） | `jinwo.dll` |
| 静态库（Unix/macOS） | `libjinwo.a` |
| 静态库（Windows） | `jinwo.lib` |
| 头文件 | `include/` 下的全部 `.h` |

---

## 2. 通用构建流程

```bash
# 1) 获取源码
git clone https://gitee.com/pkjinwo/jinwo_vector_db.git   # 国内
cd jinwo_vector_db

# 2) 配置（默认生成 Makefile / Ninja）
cmake -S . -B build

# 3) 编译（并行）
cmake --build build -j$(nproc)

# 4) 安装库 + 头文件到指定前缀
cmake --install build --prefix /your/install/path
```

安装后目录结构：

```
<your-install-path>/
├── lib/
│   ├── libjinwo.so        # 或 .dylib / .dll / .a / .lib
│   └── libjinwo.a
└── include/
    ├── jw_vecdb.h
    ├── jw_types.h
    └── ...（其余 .h）
```

在你的 C/C++ 工程中即可：

```c
#include <jw_vecdb.h>
```

并链接 `-ljinwo`（共享）或 `libjinwo.a`（静态）。

---

## 3. 分平台构建

### 3.1 Unix / Linux

默认生成 Makefile，直接执行通用流程即可：

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local
```

### 3.2 macOS

**方式 A：命令行（Makefile / Ninja）**

```bash
# 编成 x86_64 + arm64 通用二进制（可选）
cmake -S . -B build -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build -j$(sysctl -n hw.ncpu)
sudo cmake --install build --prefix /usr/local
```

**方式 B：生成 Xcode 工程（可在 GUI 里编译、调试）**

```bash
cmake -G Xcode -S . -B build-xcode
open build-xcode/jinwo_vecdb.xcodeproj
```

打开后选择 `jinwo` / `jinwo_static` target，按常规 Xcode 流程 Build / Run。

### 3.3 Windows（Visual Studio）

```bat
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
cmake --install build --prefix C:\jinwo_vecdb
```

也可以直接双击 `build\jinwo_vecdb.sln` 用 Visual Studio 打开编译。
生成的 `jinwo.lib`（静态）与 `jinwo.dll`（动态）位于
`build\Release\` 下。

---

## 4. 常见问题

- **`cmake ..` 提示找不到 CMakeLists.txt**
  旧文档让人在仓库内 `mkdir build && cd build && cmake ..`，但仓库根目录
  才有 `CMakeLists.txt`。请用 `cmake -S . -B build`（从仓库根执行），
  不要进到 `build/` 里再 `cmake ..`。

- **生成的库名带 `lib` 前缀**
  Windows / Linux 上统一为 `libjinwo.*`，这是刻意设置，避免
  `jinwo.so` 与同名的 `jinwo.py` 冲突（详见 `CMakeLists.txt` 中的
  `PREFIX "lib"`）。

---

## 5. 维护者 / AI 工具须知（同步约束）

根目录 `CMakeLists.txt` 与 `build_python_pypi/CMakeLists.txt` 共享
`JINWO_SOURCES` 源文件清单与编译宏。在**任一**文件中做以下改动时，
必须同步修改**另一**文件，否则会出现“Python 能编、C 库编不出”或反之：

1. 新增 / 删除 `src/*.c`（更新 `JINWO_SOURCES`）；
2. 改动 `JW_EXPORTS` 等 `target_compile_definitions`；
3. 改动平台特殊处理（WIN32 MSVC 子目录、`PREFIX "lib"`、APPLE
   `-undefined dynamic_lookup`）。

两份文件顶部均有醒目的“同步提醒”注释块，修改前请先阅读。
