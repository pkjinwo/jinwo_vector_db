# JinWo VecDB Examples

**Version**: v1.0.0
**Last Updated**: 2026-04-26

---

## Directory Structure

```
examples/
├── README.zh_cn.md           # Chinese documentation
├── README.en_us.md           # English documentation
├── demo.c                    # Basic demonstration program (C language)
├── vector_bench.c           # Performance testing program
├── CMakeLists.txt           # CMake build configuration
├── windows/                 # Windows platform examples
│   ├── windows_demo.cs      # C# console program
│   ├── JinWoVecDBDemo.sln   # Visual Studio solution
│   └── JinWoVecDBDemo.csproj # Visual Studio project file
├── linux/                   # Linux platform examples
│   ├── linux_demo.c         # C language program
│   ├── CMakeLists.txt       # CMake build configuration
│   └── Makefile            # Makefile
├── android/                 # Android platform examples
│   ├── app/
│   │   ├── build.gradle     # Android app build configuration
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
│   ├── build.gradle         # Project-level build configuration
│   ├── settings.gradle      # Project settings
│   └── gradle.properties    # Gradle configuration
├── ios/                     # iOS platform examples
│   ├── JinWoVecDBiOSDemo.xcodeproj/
│   │   └── project.pbxproj  # Xcode project file
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
│   └── ios_demo.swift       # iOS example code
├── python/                  # Python platform examples
│   ├── python_demo.py       # Python demonstration program
│   ├── requirements.txt     # Python dependencies
│   └── README.md            # Python example documentation
├── go/                      # Go platform examples
│   ├── go_demo.go           # Go demonstration program
│   ├── go.mod               # Go module definition
│   └── README.md            # Go example documentation
├── nodejs/                  # Node.js platform examples
│   ├── nodejs_demo.js       # Node.js demonstration program
│   ├── package.json         # Node.js project configuration
│   └── README.md            # Node.js example documentation
├── php/                     # PHP platform example
│   ├── php_demo.php         # PHP demonstration program
│   └── README.md            # PHP example documentation
├── rust/                    # Rust platform example
│   ├── Cargo.toml           # Rust project configuration
│   ├── src/                 # Rust source code
│   │   └── main.rs          # Rust demonstration program
│   ├── README.zh_cn.md      # Rust example Chinese documentation
│   └── README.en_us.md      # Rust example English documentation
├── kotlin/                  # Kotlin platform example
│   ├── build.gradle.kts     # Gradle build configuration
│   ├── src/                 # Kotlin source code
│   │   └── main/kotlin/     # Kotlin main code
│   │       └── Main.kt      # Kotlin demonstration program
│   ├── README.zh_cn.md      # Kotlin example Chinese documentation
│   └── README.en_us.md      # Kotlin example English documentation
├── typescript/              # TypeScript platform example
│   ├── package.json         # Node.js project configuration
│   ├── tsconfig.json        # TypeScript configuration
│   ├── src/                 # TypeScript source code
│   │   └── index.ts         # TypeScript demonstration program
│   ├── README.zh_cn.md      # TypeScript example Chinese documentation
│   └── README.en_us.md      # TypeScript example English documentation
├── r/                       # R platform example
│   ├── r_demo.R             # R demonstration program
│   ├── README.zh_cn.md      # R example Chinese documentation
│   └── README.en_us.md      # R example English documentation
├── dart/                    # Dart platform example
│   ├── pubspec.yaml         # Dart project configuration
│   ├── bin/                 # Dart executable files
│   │   └── dart_demo.dart   # Dart demonstration program
│   ├── README.zh_cn.md      # Dart example Chinese documentation
│   └── README.en_us.md      # Dart example English documentation
└── julia/                   # Julia platform example
    ├── julia_demo.jl        # Julia demonstration program
    ├── README.zh_cn.md      # Julia example Chinese documentation
    └── README.en_us.md      # Julia example English documentation
```

---

## Platform Support

| Platform | Language | Build Tool | Minimum Version |
|----------|----------|------------|----------------|
| Windows | C# | Visual Studio 2022 | .NET 8.0 |
| Linux | C | CMake/Make | GCC 11+ |
| Android | Java + C++ | Android Studio | API 23+ |
| iOS | Swift | Xcode | iOS 13.0+ |
| Python | Python | Native | Python 3.6+ |
| Go | Go | go build | Go 1.18+ |
| Node.js | JavaScript | npm | Node.js 14+ |
| PHP | PHP | Native | PHP 7.4+ |
| Rust | Rust | cargo | Rust 1.60+ |
| Kotlin | Kotlin | gradle | JDK 11+ |
| TypeScript | TypeScript | npm | Node.js 14+ |
| R | R | Rscript | R 4.0+ |
| Dart | Dart | pub | Dart SDK 3.0+ |
| Julia | Julia | julia | Julia 1.6+ |

---

## Quick Start

### Windows

1. **Prerequisites**
   - Visual Studio 2022
   - .NET 8.0 SDK

2. **Build Steps**
   ```bash
   cd examples/windows
   dotnet build JinWoVecDBDemo.csproj -c Release
   ```

3. **Run**
   ```bash
   cd bin/Release/net8.0
   ./JinWoVecDBDemo.exe
   ```

### Linux

1. **Prerequisites**
   - GCC 11+
   - CMake 3.20+
   - Built JinWo VecDB library

2. **Build Steps**
   ```bash
   # First build JinWo VecDB library
   cd ../..
   mkdir -p build && cd build
   cmake .. && make

   # Build examples
   cd ../examples/linux
   mkdir -p build && cd build
   cmake .. && make
   ```

3. **Run**
   ```bash
   ./linux_demo
   ```

   Or using Makefile:
   ```bash
   cd ../examples/linux
   make run
   ```

### Android

1. **Prerequisites**
   - Android Studio Hedgehog (2023.1.1) or higher
   - Android NDK r25+
   - Android SDK API 34

2. **Build Steps**
   ```bash
   cd examples/android
   ./gradlew assembleRelease
   ```

3. **Run**
   - Install APK to device:
     ```bash
     adb install app/build/outputs/apk/release/app-release.apk
     ```
   - Or run directly in Android Studio

### iOS

1. **Prerequisites**
   - Xcode 15.0+
   - Built JinWo VecDB static library

2. **Build Steps**
   ```bash
   cd examples/ios
   open JinWoVecDBiOSDemo.xcodeproj
   ```
   - Select target device and build configuration in Xcode
   - Click Run or press Cmd+R

3. **Note**
   - Need to integrate JinWo VecDB static library into the project
   - Library file should be placed at `../../build/libjw_vecdb.a`

### Python

1. **Prerequisites**
   - Python 3.6+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run Python demo
   cd examples/python
   python python_demo.py
   ```

### Go

1. **Prerequisites**
   - Go 1.18+
   - Built JinWo VecDB library
   - C compiler (gcc for Linux, MinGW for Windows)

2. **Run Steps**
   ```bash
   # Run Go demo
   cd examples/go
   go run go_demo.go
   ```

### Node.js

1. **Prerequisites**
   - Node.js 14+
   - npm 6+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Install dependencies
   cd examples/nodejs
   npm install
   
   # Run Node.js demo
   npm start
   ```

### PHP

1. **Prerequisites**
   - PHP 7.4+ (with FFI extension enabled)
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run PHP demo
   cd examples/php
   php php_demo.php
   ```

3. **Note**
   - Need to enable FFI extension in php.ini: `extension=ffi`

### Rust

1. **Prerequisites**
   - Rust 1.60+
   - Cargo 1.60+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run Rust demo
   cd examples/rust
   cargo run
   ```

### Kotlin

1. **Prerequisites**
   - JDK 11+
   - Gradle 7.0+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run Kotlin demo
   cd examples/kotlin
   ./gradlew run
   ```

### TypeScript

1. **Prerequisites**
   - Node.js 14+
   - npm 6+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Install dependencies
   cd examples/typescript
   npm install
   
   # Run TypeScript demo
   npm start
   ```

### R

1. **Prerequisites**
   - R 4.0+
   - Rcpp package
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run R demo
   cd examples/r
   Rscript r_demo.R
   ```

### Dart

1. **Prerequisites**
   - Dart SDK 3.0+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run Dart demo
   cd examples/dart
   pub get
   dart run bin/dart_demo.dart
   ```

### Julia

1. **Prerequisites**
   - Julia 1.6+
   - Built JinWo VecDB library

2. **Run Steps**
   ```bash
   # Run Julia demo
   cd examples/julia
   julia julia_demo.jl
   ```

---

## Feature Demonstration

All platform examples demonstrate the following features:

### 1. Database Operations
- Open/close database
- Create new database or open existing database

### 2. Collection Operations
- Create collection
- List all collections
- Delete collection

### 3. Vector Operations
- Insert vectors
- Search similar vectors (KNN search)
- Delete vectors

### 4. Version Information
- Get JinWo VecDB version number

---

## Example Code Function Description

| Platform | File | Function |
|----------|------|----------|
| Windows | windows_demo.cs | C# console application, using P/Invoke to call native DLL |
| Linux | linux_demo.c | C language console application, directly calling C API |
| Android | MainActivity.java + JNI | Java Android application, calling native library through JNI |
| iOS | iOSDemo.swift | Swift iOS application, using Swift/Objective-C bridging |
| Python | python_demo.py | Python script, using ctypes to call C API |
| Go | go_demo.go | Go program, using cgo to call C API |
| Node.js | nodejs_demo.js | Node.js script, using ffi-napi to call C API |
| PHP | php_demo.php | PHP script, using FFI to call C API |
| Rust | main.rs | Rust program, using libc to call C API |
| Kotlin | Main.kt | Kotlin program, using JNA to call C API |
| TypeScript | index.ts | TypeScript program, using ffi-napi to call C API |
| R | r_demo.R | R script, using Rcpp to call C API |
| Dart | dart_demo.dart | Dart program, using dart:ffi to call C API |
| Julia | julia_demo.jl | Julia script, using Libdl to call C API |

---

## Troubleshooting

### Windows
- **Issue**: Cannot find DLL
- **Solution**: Ensure `jinwo_vecdb.dll` is in the executable directory or system PATH

### Linux
- **Issue**: Link error
- **Solution**: Ensure JinWo VecDB library is properly built and located at `../../build/libjw_vecdb.a`

### Android
- **Issue**: NDK build failure
- **Solution**: Ensure NDK is properly installed and configured in Android Studio

### iOS
- **Issue**: Cannot find static library
- **Solution**: Ensure `libjw_vecdb.a` is properly integrated into the Xcode project

---

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Android Integration Guide](../../docs/en_us/android_integration.en_us.md)
- [iOS Integration Guide](../../docs/en_us/ios_integration.en_us.md)
- [Windows Integration Guide](../../docs/en_us/windows_integration.en_us.md)
- [Architecture Design Document](../../docs/en_us/architecture_design.en_us.md)

---

**Copyright** 2026 Beijing JinWo Technology Co., Ltd.
