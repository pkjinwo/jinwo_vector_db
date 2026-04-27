# JinWo VecDB Cross-Platform Testing Guide

**Version**: v1.0.0
**Generated**: 2026-04-26
**Document Type**: Public Release

---

## 1. Cross-Platform Testing Overview

### 1.1 Testing Goals

The main goal of cross-platform testing is to ensure that JinWo VecDB can run correctly and stably on different platforms and environments. Specifically including:

- **Platform compatibility**: Verify compatibility on different operating systems
- **Compiler compatibility**: Verify correctness under different compilers
- **Architecture compatibility**: Verify correctness on different CPU architectures
- **ABI compatibility**: Verify binary interface compatibility between different platforms
- **Regression testing**: Ensure cross-platform modifications do not break existing functionality

### 1.2 Testing Platform Matrix

| Platform | Operating System | Architecture | Compiler | Minimum Version |
|----------|------------------|--------------|----------|----------------|
| Linux | Ubuntu 20.04+ | x86_64 | GCC, Clang | GCC 9.0, Clang 10.0 |
| macOS | macOS 12+ | arm64, x86_64 | Apple Clang | Xcode 13.0 |
| iOS | iOS 12.0+ | arm64, x86_64 | Apple Clang | Xcode 13.0 |
| Android | Android API 21+ | armeabi-v7a, arm64-v8a, x86, x86_64 | Clang (NDK) | NDK 21.0 |
| Windows | Windows 7+ | x86, x64 | MSVC, MinGW | VS 2019 |

### 1.3 Testing Strategy

| Strategy | Description | Application Scenario |
|----------|-------------|----------------------|
| Automated testing | CI/CD integration | All platforms |
| Manual testing | Special scenarios | Mobile devices |
| Stress testing | Extreme load | Production environment |
| Compatibility testing | New platform introduction | Version release |

---

## 2. Linux Platform Testing

### 2.1 Testing Environment Configuration

#### 2.1.1 Dependency Installation

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

#### 2.1.2 Compilation Testing

```bash
# Create build directory
mkdir -p build && cd build

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
make -j$(nproc)

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
make -j$(nproc)

# Clang build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
make -j$(nproc)
```

### 2.2 Test Execution

#### 2.2.1 Unit Tests

```bash
# Enable tests
cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_TESTS=ON
make -j$(nproc)

# Run all tests
ctest -V

# Run specific test
ctest -R test_vecdb -V

# Generate coverage report
make coverage
```

#### 2.2.2 Memory Detection

```bash
# Use Valgrind to detect memory issues
valgrind --leak-check=full --show-leak-kinds=all ./tests/test_vecdb

# Use AddressSanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
make -j$(nproc)
./tests/test_vecdb
```

#### 2.2.3 Performance Benchmark

```bash
# Run performance test
./examples/vector_bench

# Use perf analysis
perf record -g ./examples/vector_bench
perf report
```

### 2.3 Regression Test Checklist

| Test Item | Description | Expected Result |
|-----------|-------------|----------------|
| Basic operations | open/close/create/drop | Success |
| Vector operations | insert/search/delete | Correct |
| Collection operations | create/list/drop | Correct |
| Concurrent operations | Multi-thread access | Correct, no race |
| Memory usage | Memory leak detection | No leak |
| File operations | Persistence/recovery | Correct |

---

## 3. macOS Platform Testing

### 3.1 Testing Environment Configuration

#### 3.1.1 Dependency Installation

```bash
# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake gcc clang
brew install valgrind
```

#### 3.1.2 Compilation Testing

```bash
# Create build directory
mkdir -p build && cd build

# Using GCC
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-13 \
    -DCMAKE_CXX_COMPILER=g++-13
make -j$(sysctl -n hw.ncpu)

# Using Apple Clang
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### 3.2 Test Execution

#### 3.2.1 Unit Tests

```bash
# Run tests
ctest -V

# Use Instruments to detect memory
# Open Xcode -> Product -> Profile -> Leaks
./tests/test_vecdb
```

#### 3.2.2 Universal Binary Testing

```bash
# Build Universal Binary (arm64 + x86_64)
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
make -j$(sysctl -n hw.ncpu)

# Verify architecture
lipo -info libjwvecdb.dylib
```

### 3.3 Regression Test Checklist

| Test Item | Description | Expected Result |
|-----------|-------------|----------------|
| Basic operations | open/close | Success |
| Vector operations | insert/search | Correct |
| Apple Silicon | ARM64 running | Correct |
| Intel Mac | x86_64 running | Correct |
| Universal Binary | Dual architecture running | Correct |

---

## 4. iOS Platform Testing

### 4.1 Testing Environment Configuration

#### 4.1.1 Dependency Installation

```bash
# Install Xcode command line tools
xcode-select --install

# Install CMake
brew install cmake

# Verify iOS SDK
xcrun simctl list devices
```

#### 4.1.2 iOS Simulator Build

```bash
# Create build directory
mkdir -p build_ios && cd build_ios

# Configure iOS Simulator build (x86_64)
cmake .. \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path) \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=ON

make -j$(sysctl -n hw.ncpu)
```

#### 4.1.3 Device Build

```bash
# Device build (arm64)
cmake .. \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path) \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=OFF

make -j$(sysctl -n hw.ncpu)
```

### 4.2 Test Execution

#### 4.2.1 Simulator Testing

```bash
# Start Simulator
xcrun simctl boot "iPhone 15 Pro"
xcrun simctl install "iPhone 15 Pro" ./build_ios/libjwvecdb.dylib

# Run tests (via Xcode IDE)
open ios_test_project.xcodeproj
```

#### 4.2.2 XCTest Testing

```bash
# Run in Xcode
# Product -> Test (Cmd+U)
```

### 4.3 Regression Test Checklist

| Test Item | Description | Device | Expected Result |
|-----------|-------------|--------|----------------|
| Basic operations | open/close | Simulator | Success |
| Vector operations | insert/search | Simulator | Correct |
| ARM64 | iPhone device | arm64 | Correct |
| x86_64 | Intel Mac Simulator | x86_64 | Correct |
| Memory limit | iOS memory limit | Device | Normal handling |

---

## 5. Android Platform Testing

### 5.1 Testing Environment Configuration

#### 5.1.1 Dependency Installation

```bash
# Install Android Studio
# Download Android Studio from https://developer.android.com/studio

# Install NDK
# Android Studio -> SDK Manager -> NDK -> 21.0+

# Install CMake
# Android Studio -> SDK Manager -> CMake -> 3.18.0+
```

#### 5.1.2 NDK Build

```bash
# Create build directory
mkdir -p build_android && cd build_android

# Configure arm64-v8a
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_TESTS=OFF

make -j$(nproc)

# Copy library to project
mkdir -p ../android_app/app/src/main/jniLibs/arm64-v8a/
cp libjwvecdb.so ../android_app/app/src/main/jniLibs/arm64-v8a/
```

#### 5.1.3 Multi-ABI Build

```bash
# Build all supported ABIs
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

### 5.2 Test Execution

#### 5.2.1 JUnit Testing

```java
// Run in Android Studio
// Run -> Run 'Tests' (Shift+F10)
```

#### 5.2.2 Native Testing

```java
// Using AndroidNativeTest
public class NativeTest {
    static {
        System.loadLibrary("jw_vecdb");
    }

    public native int testOpenClose();
    public native int testVectorInsert();
    public native int testVectorSearch();
}
```

### 5.3 Regression Test Checklist

| Test Item | Description | ABI | Expected Result |
|-----------|-------------|-----|----------------|
| Basic operations | open/close | arm64-v8a | Success |
| Vector operations | insert/search | arm64-v8a | Correct |
| Memory limit | Android MEMLIMIT | All | Normal handling |
| armeabi-v7a | 32-bit support | armeabi-v7a | Correct |
| x86 | Emulator | x86 | Correct |
| x86_64 | Emulator | x86_64 | Correct |

---

## 6. Windows Platform Testing

### 6.1 Testing Environment Configuration

#### 6.1.1 Visual Studio

```bash
# Using Developer Command Prompt
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"

# Configure x64
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### 6.1.2 MinGW-w64

```bash
# Using MinGW-w64
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
mingw32-make -j$(nproc)
```

### 6.2 Test Execution

#### 6.2.1 Visual Studio Testing

```bash
# Run tests
ctest -C Release -V

# Using Visual Studio GUI
# Test -> Run All Tests
```

#### 6.2.2 Windows Specific Testing

```powershell
# Unicode path test
$path = "C:\Test Data\Vector Database"
.\tests\test_vecdb.exe $path

# Long path test
$longPath = "C:\" + ("a" * 200) + "\vecdb"
.\tests\test_vecdb.exe $longPath
```

### 6.3 Regression Test Checklist

| Test Item | Description | Configuration | Expected Result |
|-----------|-------------|---------------|----------------|
| Basic operations | open/close | x64 Release | Success |
| Vector operations | insert/search | x64 Release | Correct |
| Unicode | Chinese path | x64 | Correct |
| x86 | 32-bit build | x86 | Correct |
| Static linking | Static library | x64 | Correct |
| DLL | Dynamic library | x64 | Correct |

---

## 7. Cross-Platform Test Cases

### 7.1 Basic Functionality Tests

| Case ID | Test Item | Input | Expected Output | Platforms |
|---------|-----------|-------|----------------|-----------|
| PT001 | Database open | Normal path | Return JW_OK | ALL |
| PT002 | Database close | Normal close | Return JW_OK | ALL |
| PT003 | Create collection | Normal parameters | Return JW_OK | ALL |
| PT004 | Insert vector | Normal vector | Return vector ID | ALL |
| PT005 | Search vector | Normal query | Return similar vectors | ALL |
| PT006 | Delete vector | Existing ID | Return JW_OK | ALL |
| PT007 | List collections | Existing collections | Return collection list | ALL |

### 7.2 Boundary Case Tests

| Case ID | Test Item | Input | Expected Output | Platforms |
|---------|-----------|-------|----------------|-----------|
| BT001 | Empty collection search | Empty collection | Return empty result | ALL |
| BT002 | Large vector dimension | 10000 dimensions | Normal handling | ALL |
| BT003 | Large data volume | 10M vectors | Normal handling | ALL |
| BT004 | Special character path | Chinese characters | Normal handling | WIN, AND |
| BT005 | Empty path | Empty string | Return error code | ALL |
| BT006 | Invalid ID | Negative ID | Return error code | ALL |

### 7.3 Concurrent Tests

| Case ID | Test Item | Input | Expected Output | Platforms |
|---------|-----------|-------|----------------|-----------|
| CT001 | Multi-thread insert | 4 threads | No data loss | ALL |
| CT002 | Multi-thread search | 4 threads | Correct results | ALL |
| CT003 | Read-write concurrent | 2 read 2 write | No crash | ALL |
| CT004 | Extreme concurrent | 16 threads | Normal handling | ALL |

### 7.4 Memory Tests

| Case ID | Test Item | Input | Expected Output | Platforms |
|---------|-----------|-------|----------------|-----------|
| MT001 | Memory leak | Long running | No leak | ALL |
| MT002 | Memory limit | Memory constrained | Normal handling | AND, IOS |
| MT003 | Large memory allocation | 1GB vector | Normal handling | ALL |
| MT004 | Memory reclamation | Mass insert/delete | Memory reclaimed | ALL |

---

## 8. CI/CD Integration

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

### 8.3 Local Test Script

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

# Select test based on platform
case "$(uname -s)" in
    Linux*)     test_linux ;;
    Darwin*)    test_macos ;;
    MINGW*|MSYS*) test_windows ;;
    *)          echo "Unknown platform" && exit 1 ;;
esac

echo "=== All tests passed! ==="
```

---

## 9. Test Report

### 9.1 Test Report Template

| Field | Content |
|-------|---------|
| Test date | YYYY-MM-DD |
| Test platform | Platform name and version |
| Compiler version | GCC/Clang/MSVC version |
| Tester | Tester name |
| Test result | Pass/Fail/Blocked |
| Found issues | Issue list |

### 9.2 Test Result Statistics

| Metric | Value |
|--------|-------|
| Total test cases | N |
| Passed cases | N |
| Failed cases | N |
| Blocked cases | N |
| Pass rate | XX% |
| Total test time | XX min |

---

## 10. Troubleshooting

### 10.1 Common Issues

| Issue | Platform | Possible Cause | Solution |
|-------|----------|----------------|----------|
| Compilation failure | Linux | GCC version too low | Upgrade GCC to 9.0+ |
| Link error | macOS | SDK path error | Set CMAKE_OSX_SYSROOT |
| ABI incompatibility | Android | NDK version mismatch | Use matching NDK version |
| Path issue | Windows | Unicode encoding | Use UTF-8 encoding |
| Out of memory | iOS | Memory limit | Optimize memory usage |

### 10.2 Debugging Tips

| Platform | Tool | Purpose |
|----------|------|---------|
| Linux | GDB | Debug crash |
| Linux | Valgrind | Memory issues |
| macOS | LLDB | Debug crash |
| macOS | Instruments | Memory analysis |
| Android | ndk-stack | Native crash |
| Windows | WinDbg | Debug crash |

---

**Document Update Records**

| Date | Version | Update Content |
|------|---------|----------------|
| 2026-04-26 | v1.0.0 | Initial version |
