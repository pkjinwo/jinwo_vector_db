# JinWo VecDB Examples

**版本**: v1.0.0
**更新日期**: 2026-04-26

---

## 目录结构

```
examples/
├── README.zh_cn.md           # 中文说明文档
├── README.en_us.md           # 英文说明文档
├── demo.c                    # 基础演示程序（C语言）
├── vector_bench.c           # 性能测试程序
├── CMakeLists.txt           # CMake 构建配置
├── windows/                 # Windows 平台示例
│   ├── windows_demo.cs      # C# 控制台程序
│   ├── JinWoVecDBDemo.sln   # Visual Studio 解决方案
│   └── JinWoVecDBDemo.csproj # Visual Studio 项目文件
├── linux/                   # Linux 平台示例
│   ├── linux_demo.c         # C 语言程序
│   ├── CMakeLists.txt       # CMake 构建配置
│   └── Makefile            # Makefile
├── android/                 # Android 平台示例
│   ├── app/
│   │   ├── build.gradle     # Android 应用构建配置
│   │   └── src/main/
│   │       ├── AndroidManifest.xml
│   │       ├── java/com/jinwo/vecdb/demo/
│   │       │   └── MainActivity.java
│   │       ├── jni/
│   │       │   ├── android_demo_jni.cpp
│   │       │   └── CMakeLists.txt
│   │       └── res/
│   │           ├── layout/activity_main.xml
│   │           └── values/
│   ├── build.gradle         # 项目级构建配置
│   ├── settings.gradle      # 项目设置
│   └── gradle.properties    # Gradle 配置
├── ios/                     # iOS 平台示例
│   ├── JinWoVecDBiOSDemo.xcodeproj/
│   │   └── project.pbxproj  # Xcode 项目文件
│   ├── JinWoVecDBiOSDemo/
│   │   ├── Sources/
│   │   │   ├── AppDelegate.swift
│   │   │   ├── ViewController.swift
│   │   │   └── iOSDemo.swift
│   │   ├── Resources/
│   │   │   ├── Main.storyboard
│   │   │   ├── LaunchScreen.storyboard
│   │   │   └── Assets.xcassets/
│   │   └── Supporting Files/
│   │       ├── Info.plist
│   │       └── JinWoVecDBiOSDemo.entitlements
│   └── ios_demo.swift       # iOS 示例代码
├── python/                  # Python 平台示例
│   ├── python_demo.py       # Python 演示程序
│   ├── requirements.txt     # Python 依赖项
│   └── README.md            # Python 示例说明
├── go/                      # Go 平台示例
│   ├── go_demo.go           # Go 演示程序
│   ├── go.mod               # Go 模块定义
│   └── README.md            # Go 示例说明
├── nodejs/                  # Node.js 平台示例
│   ├── nodejs_demo.js       # Node.js 演示程序
│   ├── package.json         # Node.js 项目配置
│   └── README.md            # Node.js 示例说明
├── php/                     # PHP 平台示例
│   ├── php_demo.php         # PHP 演示程序
│   └── README.md            # PHP 示例说明
├── rust/                    # Rust 平台示例
│   ├── Cargo.toml           # Rust 项目配置
│   ├── src/                 # Rust 源代码
│   │   └── main.rs          # Rust 演示程序
│   ├── README.zh_cn.md      # Rust 示例中文说明
│   └── README.en_us.md      # Rust 示例英文说明
├── kotlin/                  # Kotlin 平台示例
│   ├── build.gradle.kts     # Gradle 构建配置
│   ├── src/                 # Kotlin 源代码
│   │   └── main/kotlin/     # Kotlin 主代码
│   │       └── Main.kt      # Kotlin 演示程序
│   ├── README.zh_cn.md      # Kotlin 示例中文说明
│   └── README.en_us.md      # Kotlin 示例英文说明
├── typescript/              # TypeScript 平台示例
│   ├── package.json         # Node.js 项目配置
│   ├── tsconfig.json        # TypeScript 配置
│   ├── src/                 # TypeScript 源代码
│   │   └── index.ts         # TypeScript 演示程序
│   ├── README.zh_cn.md      # TypeScript 示例中文说明
│   └── README.en_us.md      # TypeScript 示例英文说明
├── r/                       # R 平台示例
│   ├── r_demo.R             # R 演示程序
│   ├── README.zh_cn.md      # R 示例中文说明
│   └── README.en_us.md      # R 示例英文说明
├── dart/                    # Dart 平台示例
│   ├── pubspec.yaml         # Dart 项目配置
│   ├── bin/                 # Dart 可执行文件
│   │   └── dart_demo.dart   # Dart 演示程序
│   ├── README.zh_cn.md      # Dart 示例中文说明
│   └── README.en_us.md      # Dart 示例英文说明
└── julia/                   # Julia 平台示例
    ├── julia_demo.jl        # Julia 演示程序
    ├── README.zh_cn.md      # Julia 示例中文说明
    └── README.en_us.md      # Julia 示例英文说明
```

---

## 平台支持

| 平台 | 语言 | 构建工具 | 最低版本 |
|------|------|----------|----------|
| Windows | C# | Visual Studio 2022 | .NET 8.0 |
| Linux | C | CMake/Make | GCC 11+ |
| Android | Java + C++ | Android Studio | API 23+ |
| iOS | Swift | Xcode | iOS 13.0+ |
| Python | Python | 原生 | Python 3.6+ |
| Go | Go | go build | Go 1.18+ |
| Node.js | JavaScript | npm | Node.js 14+ |
| PHP | PHP | 原生 | PHP 7.4+ |
| Rust | Rust | cargo | Rust 1.60+ |
| Kotlin | Kotlin | gradle | JDK 11+ |
| TypeScript | TypeScript | npm | Node.js 14+ |
| R | R | Rscript | R 4.0+ |
| Dart | Dart | pub | Dart SDK 3.0+ |
| Julia | Julia | julia | Julia 1.6+ |

---

## 快速开始

### Windows

1. **前置条件**
   - Visual Studio 2022
   - .NET 8.0 SDK

2. **构建步骤**
   ```bash
   cd examples/windows
   dotnet build JinWoVecDBDemo.csproj -c Release
   ```

3. **运行**
   ```bash
   cd bin/Release/net8.0
   ./JinWoVecDBDemo.exe
   ```

### Linux

1. **前置条件**
   - GCC 11+
   - CMake 3.20+
   - 已构建的 JinWo VecDB 库

2. **构建步骤**
   ```bash
   # 首先构建 JinWo VecDB 库
   cd ../..
   mkdir -p build && cd build
   cmake .. && make

   # 构建示例
   cd ../examples/linux
   mkdir -p build && cd build
   cmake .. && make
   ```

3. **运行**
   ```bash
   ./linux_demo
   ```

   或使用 Makefile：
   ```bash
   cd ../examples/linux
   make run
   ```

### Android

1. **前置条件**
   - Android Studio Hedgehog (2023.1.1) 或更高版本
   - Android NDK r25+
   - Android SDK API 34

2. **构建步骤**
   ```bash
   cd examples/android
   ./gradlew assembleRelease
   ```

3. **运行**
   - 将 APK 安装到设备：
     ```bash
     adb install app/build/outputs/apk/release/app-release.apk
     ```
   - 或在 Android Studio 中直接运行

### iOS

1. **前置条件**
   - Xcode 15.0+
   - 已构建的 JinWo VecDB 静态库

2. **构建步骤**
   ```bash
   cd examples/ios
   open JinWoVecDBiOSDemo.xcodeproj
   ```
   - 在 Xcode 中选择目标设备和构建配置
   - 点击 Run 或按 Cmd+R

3. **注意**
   - 需要将 JinWo VecDB 静态库集成到项目中
   - 库文件应放置在 `../../build/libjw_vecdb.a`

### Python

1. **前置条件**
   - Python 3.6+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 Python 演示
   cd examples/python
   python python_demo.py
   ```

### Go

1. **前置条件**
   - Go 1.18+
   - 已构建的 JinWo VecDB 库
   - C 编译器（Linux 上的 gcc，Windows 上的 MinGW）

2. **运行步骤**
   ```bash
   # 运行 Go 演示
   cd examples/go
   go run go_demo.go
   ```

### Node.js

1. **前置条件**
   - Node.js 14+
   - npm 6+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 安装依赖
   cd examples/nodejs
   npm install
   
   # 运行 Node.js 演示
   npm start
   ```

### PHP

1. **前置条件**
   - PHP 7.4+（启用 FFI 扩展）
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 PHP 示例
   cd examples/php
   php php_demo.php
   ```

3. **注意**
   - 需要在 php.ini 中启用 FFI 扩展：`extension=ffi`

### Rust

1. **前置条件**
   - Rust 1.60+
   - Cargo 1.60+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 Rust 示例
   cd examples/rust
   cargo run
   ```

### Kotlin

1. **前置条件**
   - JDK 11+
   - Gradle 7.0+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 Kotlin 示例
   cd examples/kotlin
   ./gradlew run
   ```

### TypeScript

1. **前置条件**
   - Node.js 14+
   - npm 6+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 安装依赖
   cd examples/typescript
   npm install
   
   # 运行 TypeScript 示例
   npm start
   ```

### R

1. **前置条件**
   - R 4.0+
   - Rcpp 包
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 R 示例
   cd examples/r
   Rscript r_demo.R
   ```

### Dart

1. **前置条件**
   - Dart SDK 3.0+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 Dart 示例
   cd examples/dart
   pub get
   dart run bin/dart_demo.dart
   ```

### Julia

1. **前置条件**
   - Julia 1.6+
   - 已构建的 JinWo VecDB 库

2. **运行步骤**
   ```bash
   # 运行 Julia 示例
   cd examples/julia
   julia julia_demo.jl
   ```

---

## 功能演示

所有平台的示例程序都演示了以下功能：

### 1. 数据库操作
- 打开/关闭数据库
- 创建新数据库或打开现有数据库

### 2. 集合操作
- 创建集合（Collection）
- 列出所有集合
- 删除集合

### 3. 向量操作
- 插入向量
- 搜索相似向量（KNN 搜索）
- 删除向量

### 4. 版本信息
- 获取 JinWo VecDB 版本号

---

## 示例代码功能说明

| 平台 | 文件 | 功能 |
|------|------|------|
| Windows | windows_demo.cs | C# 控制台应用，使用 P/Invoke 调用原生 DLL |
| Linux | linux_demo.c | C 语言控制台应用，直接调用 C API |
| Android | MainActivity.java + JNI | Java Android 应用，通过 JNI 调用原生库 |
| iOS | iOSDemo.swift | Swift iOS 应用，使用 Swift/Objective-C 桥接 |
| Python | python_demo.py | Python 脚本，使用 ctypes 调用 C API |
| Go | go_demo.go | Go 程序，使用 cgo 调用 C API |
| Node.js | nodejs_demo.js | Node.js 脚本，使用 ffi-napi 调用 C API |
| PHP | php_demo.php | PHP 脚本，使用 FFI 调用 C API |
| Rust | main.rs | Rust 程序，使用 libc 调用 C API |
| Kotlin | Main.kt | Kotlin 程序，使用 JNA 调用 C API |
| TypeScript | index.ts | TypeScript 程序，使用 ffi-napi 调用 C API |
| R | r_demo.R | R 脚本，使用 Rcpp 调用 C API |
| Dart | dart_demo.dart | Dart 程序，使用 dart:ffi 调用 C API |
| Julia | julia_demo.jl | Julia 脚本，使用 Libdl 调用 C API |

---

## 故障排除

### Windows
- **问题**: 找不到 DLL
- **解决**: 确保 `jinwo_vecdb.dll` 在可执行文件目录或系统 PATH 中

### Linux
- **问题**: 链接错误
- **解决**: 确保 JinWo VecDB 库已正确构建并位于 `../../build/libjw_vecdb.a`

### Android
- **问题**: NDK 构建失败
- **解决**: 确保 NDK 已正确安装并在 Android Studio 中配置

### iOS
- **问题**: 找不到静态库
- **解决**: 确保 `libjw_vecdb.a` 已正确集成到 Xcode 项目中

---

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Android 集成指南](../../docs/zh_cn/android_integration.zh_cn.md)
- [iOS 集成指南](../../docs/zh_cn/ios_integration.zh_cn.md)
- [Windows 集成指南](../../docs/zh_cn/windows_integration.zh_cn.md)
- [架构设计文档](../../docs/zh_cn/architecture_design.zh_cn.md)

---

**版权所有** 2026 北京金幄科技有限公司
