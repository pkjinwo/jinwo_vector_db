# JinWo VecDB 跨平台测试指南

**版本**: v1.0.0
**生成时间**: 2026-04-26
**文档类型**: 对外发布

---

## 一、跨平台测试概述

### 1.1 测试目标

跨平台测试的主要目标是确保 JinWo VecDB 在不同平台和环境下都能正确、稳定地运行。具体包括：

- **平台兼容性**: 验证在不同操作系统上的兼容性
- **编译器兼容性**: 验证在不同编译器下的正确性
- **架构兼容性**: 验证在不同CPU架构上的正确性
- **ABI兼容性**: 验证不同平台间的二进制接口兼容性
- **回归测试**: 确保跨平台修改不会破坏现有功能

### 1.2 测试平台矩阵

| 平台 | 操作系统 | 架构 | 编译器 | 最低版本 |
|------|----------|------|--------|----------|
| Linux | Ubuntu 20.04+ | x86_64 | GCC, Clang | GCC 9.0, Clang 10.0 |
| macOS | macOS 12+ | arm64, x86_64 | Apple Clang | Xcode 13.0 |
| iOS | iOS 12.0+ | arm64, x86_64 | Apple Clang | Xcode 13.0 |
| Android | Android API 21+ | armeabi-v7a, arm64-v8a, x86, x86_64 | Clang (NDK) | NDK 21.0 |
| Windows | Windows 7+ | x86, x64 | MSVC, MinGW | VS 2019 |

### 1.3 测试策略

| 策略 | 描述 | 适用场景 |
|------|------|----------|
| 自动化测试 | CI/CD集成 | 所有平台 |
| 手动测试 | 特殊场景 | 移动设备 |
| 压力测试 | 极限负载 | 生产环境 |
| 兼容性测试 | 新平台引入 | 版本发布 |

---

## 二、Linux 平台测试

### 2.1 测试环境配置

#### 2.1.1 依赖安装

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y gcc g++ clang
sudo apt install -y valgrind libasan

# CentOS/RHEL
sudo yum install -y gcc gcc-c++ make cmake
sudo yum install -y clang
```

#### 2.1.2 编译测试

```bash
# 创建构建目录
mkdir -p build && cd build

# Debug 构建
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
make -j$(nproc)

# Release 构建
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
make -j$(nproc)

# Clang 构建
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
make -j$(nproc)
```

### 2.2 测试执行

#### 2.2.1 单元测试

```bash
# 启用测试
cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_TESTS=ON
make -j$(nproc)

# 运行所有测试
ctest -V

# 运行特定测试
ctest -R test_vecdb -V

# 生成覆盖率报告
make coverage
```

#### 2.2.2 内存检测

```bash
# 使用 Valgrind 检测内存问题
valgrind --leak-check=full --show-leak-kinds=all ./tests/test_vecdb

# 使用 AddressSanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
make -j$(nproc)
./tests/test_vecdb
```

#### 2.2.3 性能基准

```bash
# 运行性能测试
./examples/vector_bench

# 使用 perf 分析
perf record -g ./examples/vector_bench
perf report
```

### 2.3 回归测试清单

| 测试项 | 描述 | 预期结果 |
|--------|------|----------|
| 基本操作 | open/close/create/drop | 成功 |
| 向量操作 | insert/search/delete | 正确 |
| 集合操作 | create/list/drop | 正确 |
| 并发操作 | 多线程访问 | 正确，无竞态 |
| 内存使用 | 内存泄漏检测 | 无泄漏 |
| 文件操作 | 持久化/恢复 | 正确 |

---

## 三、macOS 平台测试

### 3.1 测试环境配置

#### 3.1.1 依赖安装

```bash
# 安装 Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake gcc clang
brew install valgrind
```

#### 3.1.2 编译测试

```bash
# 创建构建目录
mkdir -p build && cd build

# 使用 GCC
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-13 \
    -DCMAKE_CXX_COMPILER=g++-13
make -j$(sysctl -n hw.ncpu)

# 使用 Apple Clang
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### 3.2 测试执行

#### 3.2.1 单元测试

```bash
# 运行测试
ctest -V

# 使用 Instruments 检测内存
# 打开 Xcode -> Product -> Profile -> Leaks
./tests/test_vecdb
```

#### 3.2.2 Universal Binary 测试

```bash
# 构建 Universal Binary (arm64 + x86_64)
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
make -j$(sysctl -n hw.ncpu)

# 验证架构
lipo -info libjwvecdb.dylib
```

### 3.3 回归测试清单

| 测试项 | 描述 | 预期结果 |
|--------|------|----------|
| 基本操作 | open/close | 成功 |
| 向量操作 | insert/search | 正确 |
| Apple Silicon | ARM64 运行 | 正确 |
| Intel Mac | x86_64 运行 | 正确 |
| Universal Binary | 双架构运行 | 正确 |

---

## 四、iOS 平台测试

### 4.1 测试环境配置

#### 4.1.1 依赖安装

```bash
# 安装 Xcode 命令行工具
xcode-select --install

# 安装 CMake
brew install cmake

# 验证 iOS SDK
xcrun simctl list devices
```

#### 4.1.2 iOS Simulator 构建

```bash
# 创建构建目录
mkdir -p build_ios && cd build_ios

# 配置 iOS Simulator 构建 (x86_64)
cmake .. \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path) \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=ON

make -j$(sysctl -n hw.ncpu)
```

#### 4.1.3 真机构建

```bash
# 真机构建 (arm64)
cmake .. \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path) \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=OFF

make -j$(sysctl -n hw.ncpu)
```

### 4.2 测试执行

#### 4.2.1 Simulator 测试

```bash
# 启动 Simulator
xcrun simctl boot "iPhone 15 Pro"
xcrun simctl install "iPhone 15 Pro" ./build_ios/libjwvecdb.dylib

# 运行测试（通过 Xcode IDE）
open ios_test_project.xcodeproj
```

#### 4.2.2 XCTest 测试

```bash
# 在 Xcode 中运行
# Product -> Test (Cmd+U)
```

### 4.3 回归测试清单

| 测试项 | 描述 | 设备 | 预期结果 |
|--------|------|------|----------|
| 基本操作 | open/close | Simulator | 成功 |
| 向量操作 | insert/search | Simulator | 正确 |
| ARM64 | iPhone 真机 | arm64 | 正确 |
| x86_64 | Intel Mac Simulator | x86_64 | 正确 |
| 内存限制 | iOS 内存限制 | 真机 | 正常处理 |

---

## 五、Android 平台测试

### 5.1 测试环境配置

#### 5.1.1 依赖安装

```bash
# 安装 Android Studio
# 下载 Android Studio from https://developer.android.com/studio

# 安装 NDK
# Android Studio -> SDK Manager -> NDK -> 21.0+

# 安装 CMake
# Android Studio -> SDK Manager -> CMake -> 3.18.0+
```

#### 5.1.2 NDK 构建

```bash
# 创建构建目录
mkdir -p build_android && cd build_android

# 配置 arm64-v8a
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=OFF

make -j$(nproc)

# 复制库到项目
mkdir -p ../android_app/app/src/main/jniLibs/arm64-v8a/
cp libjwvecdb.so ../android_app/app/src/main/jniLibs/arm64-v8a/
```

#### 5.1.3 多ABI构建

```bash
# 构建所有支持的 ABI
for ABI in armeabi-v7a arm64-v8a x86 x86_64; do
    mkdir -p build_$ABI && cd build_$ABI
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ABI \
        -DANDROID_PLATFORM=android-21 \
        -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    cd ..
done
```

### 5.2 测试执行

#### 5.2.1 JUnit 测试

```java
// 在 Android Studio 中运行
// Run -> Run 'Tests' (Shift+F10)
```

#### 5.2.2 Native 测试

```java
// 使用 AndroidNativeTest
public class NativeTest {
    static {
        System.loadLibrary("jw_vecdb");
    }

    public native int testOpenClose();
    public native int testVectorInsert();
    public native int testVectorSearch();
}
```

### 5.3 回归测试清单

| 测试项 | 描述 | ABI | 预期结果 |
|--------|------|-----|----------|
| 基本操作 | open/close | arm64-v8a | 成功 |
| 向量操作 | insert/search | arm64-v8a | 正确 |
| 内存限制 | Android MEMLIMIT | 所有 | 正常处理 |
| armeabi-v7a | 32位支持 | armeabi-v7a | 正确 |
| x86 | 模拟器 | x86 | 正确 |
| x86_64 | 模拟器 | x86_64 | 正确 |

---

## 六、Windows 平台测试

### 6.1 测试环境配置

#### 6.1.1 Visual Studio

```bash
# 使用 Developer Command Prompt
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"

# 配置 x64
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### 6.1.2 MinGW-w64

```bash
# 使用 MinGW-w64
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
mingw32-make -j$(nproc)
```

### 6.2 测试执行

#### 6.2.1 Visual Studio 测试

```bash
# 运行测试
ctest -C Release -V

# 使用 Visual Studio GUI
# Test -> Run All Tests
```

#### 6.2.2 Windows 特定测试

```powershell
# Unicode 路径测试
$path = "C:\测试数据\向量库"
.\tests\test_vecdb.exe $path

# 长路径测试
$longPath = "C:\" + ("a" * 200) + "\vecdb"
.\tests\test_vecdb.exe $longPath
```

### 6.3 回归测试清单

| 测试项 | 描述 | 配置 | 预期结果 |
|--------|------|------|----------|
| 基本操作 | open/close | x64 Release | 成功 |
| 向量操作 | insert/search | x64 Release | 正确 |
| Unicode | 中文路径 | x64 | 正确 |
| x86 | 32位构建 | x86 | 正确 |
| 静态链接 | 静态库 | x64 | 正确 |
| DLL | 动态库 | x64 | 正确 |

---

## 七、跨平台测试用例

### 7.1 基础功能测试

| 用例ID | 测试项 | 输入 | 预期输出 | 适用平台 |
|--------|--------|------|----------|----------|
| PT001 | 数据库打开 | 正常路径 | 返回 JW_OK | ALL |
| PT002 | 数据库关闭 | 正常关闭 | 返回 JW_OK | ALL |
| PT003 | 创建集合 | 正常参数 | 返回 JW_OK | ALL |
| PT004 | 插入向量 | 正常向量 | 返回向量ID | ALL |
| PT005 | 搜索向量 | 正常查询 | 返回相似向量 | ALL |
| PT006 | 删除向量 | 存在的ID | 返回 JW_OK | ALL |
| PT007 | 列举集合 | 存在的集合 | 返回集合列表 | ALL |

### 7.2 边界情况测试

| 用例ID | 测试项 | 输入 | 预期输出 | 适用平台 |
|--------|--------|------|----------|----------|
| BT001 | 空集合搜索 | 空集合 | 返回空结果 | ALL |
| BT002 | 极大向量维度 | 10000维 | 正常处理 | ALL |
| BT003 | 极大数据量 | 10M向量 | 正常处理 | ALL |
| BT004 | 特殊字符路径 | 中文字符 | 正常处理 | WIN, AND |
| BT005 | 空路径 | 空字符串 | 返回错误码 | ALL |
| BT006 | 非法ID | 负数ID | 返回错误码 | ALL |

### 7.3 并发测试

| 用例ID | 测试项 | 输入 | 预期输出 | 适用平台 |
|--------|--------|------|----------|----------|
| CT001 | 多线程插入 | 4线程 | 无数据丢失 | ALL |
| CT002 | 多线程搜索 | 4线程 | 结果正确 | ALL |
| CT003 | 读写并发 | 2读2写 | 无崩溃 | ALL |
| CT004 | 极端并发 | 16线程 | 正常处理 | ALL |

### 7.4 内存测试

| 用例ID | 测试项 | 输入 | 预期输出 | 适用平台 |
|--------|--------|------|----------|----------|
| MT001 | 内存泄漏 | 长时间运行 | 无泄漏 | ALL |
| MT002 | 内存限制 | 内存受限环境 | 正常处理 | AND, IOS |
| MT003 | 大内存分配 | 1GB向量 | 正常处理 | ALL |
| MT004 | 内存回收 | 大量插入删除 | 内存回收 | ALL |

---

## 八、CI/CD 集成

### 8.1 GitHub Actions

```yaml
# .github/workflows/cross-platform.yml
name: Cross-Platform CI

on:
  push:
    branches: [main, master]
  pull_request:
    branches: [main, master]

jobs:
  linux:
    runs-on: ubuntu-20.04
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: sudo apt-get update && sudo apt-get install -y cmake gcc g++ valgrind
      - name: Build
        run: mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
      - name: Test
        run: cd build && ctest -V
      - name: Memory check
        run: valgrind --leak-check=full ./tests/test_vecdb

  macos:
    runs-on: macos-12
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: brew install cmake
      - name: Build
        run: mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.ncpu)
      - name: Test
        run: cd build && ctest -V

  windows:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: |
          cmake .. -G "Visual Studio 17 2022" -A x64
          cmake --build . --config Release
      - name: Test
        run: ctest -C Release -V
```

### 8.2 GitLab CI

```yaml
# .gitlab-ci.yml
stages:
  - build
  - test

linux-build:
  stage: build
  image: ubuntu:20.04
  script:
    - apt-get update && apt-get install -y cmake gcc g++ valgrind
    - mkdir -p build && cd build && cmake .. && make -j$(nproc)
  artifacts:
    paths:
      - build/libjwvecdb.*

linux-test:
  stage: test
  image: ubuntu:20.04
  script:
    - apt-get update && apt-get install -y valgrind
    - cd build && ctest -V

macos-build:
  stage: build
  tags:
    - macos
  script:
    - mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.ncpu)

windows-build:
  stage: build
  tags:
    - windows
  script:
    - cmake .. -G "Visual Studio 17 2022" -A x64
    - cmake --build . --config Release
```

### 8.3 本地测试脚本

```bash
#!/bin/bash
# scripts/cross_platform_test.sh

set -e

echo "=== JinWo VecDB Cross-Platform Test ==="

# Linux
test_linux() {
    echo "Testing on Linux..."
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    ctest -V
    cd ..
    echo "Linux test passed!"
}

# macOS
test_macos() {
    echo "Testing on macOS..."
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(sysctl -n hw.ncpu)
    ctest -V
    cd ..
    echo "macOS test passed!"
}

# Android (requires NDK)
test_android() {
    echo "Testing on Android..."
    export ANDROID_NDK=$HOME/Android/Ndk/latest
    mkdir -p build_android && cd build_android
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-21
    make -j$(nproc)
    cd ..
    echo "Android test passed!"
}

# Windows (PowerShell)
test_windows() {
    echo "Testing on Windows..."
    cmake .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release
    ctest -C Release -V
    echo "Windows test passed!"
}

# 根据平台选择测试
case "$(uname -s)" in
    Linux*)     test_linux ;;
    Darwin*)    test_macos ;;
    MINGW*|MSYS*) test_windows ;;
    *)          echo "Unknown platform" && exit 1 ;;
esac

echo "=== All tests passed! ==="
```

---

## 九、测试报告

### 9.1 测试报告模板

| 字段 | 内容 |
|------|------|
| 测试日期 | YYYY-MM-DD |
| 测试平台 | 平台名称和版本 |
| 编译器版本 | GCC/Clang/MSVC 版本 |
| 测试人员 | 测试者姓名 |
| 测试结果 | 通过/失败/阻塞 |
| 发现的问题 | 问题列表 |

### 9.2 测试结果统计

| 指标 | 数值 |
|------|------|
| 总测试用例数 | N |
| 通过用例数 | N |
| 失败用例数 | N |
| 阻塞用例数 | N |
| 通过率 | XX% |
| 总测试时间 | XX min |

---

## 十、故障排除

### 10.1 常见问题

| 问题 | 平台 | 可能原因 | 解决方案 |
|------|------|----------|----------|
| 编译失败 | Linux | GCC版本过低 | 升级GCC到9.0+ |
| 链接错误 | macOS | SDK路径错误 | 设置CMAKE_OSX_SYSROOT |
| ABI不兼容 | Android | NDK版本不匹配 | 使用匹配的NDK版本 |
| 路径问题 | Windows | Unicode编码 | 使用UTF-8编码 |
| 内存不足 | iOS | 内存限制 | 优化内存使用 |

### 10.2 调试技巧

| 平台 | 工具 | 用途 |
|------|------|------|
| Linux | GDB | 调试崩溃 |
| Linux | Valgrind | 内存问题 |
| macOS | LLDB | 调试崩溃 |
| macOS | Instruments | 内存分析 |
| Android | ndk-stack | Native崩溃 |
| Windows | WinDbg | 调试崩溃 |

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
