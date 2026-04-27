# JinWo VecDB Android Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains an Android example for JinWo VecDB, demonstrating how to use Java and JNI to call the C API for the vector database.

## Prerequisites

- Android Studio Hedgehog (2023.1.1) or higher
- Android NDK r25+
- Android SDK API 34
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Open Android Project

```bash
# Open Android project
cd ../examples/android
# Open the project in Android Studio
```

### 3. Build and Run

- In Android Studio, select target device
- Click Run or press Shift+F10
- Or build using command line:
  ```bash
  ./gradlew assembleDebug
  adb install app/build/outputs/apk/debug/app-debug.apk
  ```

## Feature Demonstration

The Android example demonstrates the following features:

- **Database Operations**
  - Open/close database
  - Create new database or open existing database

- **Collection Operations**
  - Create collection
  - List all collections

- **Vector Operations**
  - Insert vectors
  - Search similar vectors (KNN search)

- **Version Information**
  - Get JinWo VecDB version number

## Code Structure

- `app/src/main/java/com/jinwo/vecdb/demo/MainActivity.java` - Main activity class
- `app/src/main/jni/android_demo_jni.cpp` - JNI wrapper
- `app/build.gradle` - App-level build configuration
- `build.gradle` - Project-level build configuration

## Technical Implementation

- **Java Layer**: Calls native library through JNI
- **JNI Layer**: Wraps C API, provides Java-callable interface
- **Native Layer**: Directly calls JinWo VecDB C API

## Troubleshooting

### NDK Build Failure
- **Issue**: NDK build failure
- **Solution**: Ensure NDK is properly installed and configured in Android Studio

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and correctly referenced in CMakeLists.txt

### API Level Issues
- **Issue**: API level incompatibility
- **Solution**: Ensure using API 23 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Android Integration Guide](../../docs/en_us/android_integration.en_us.md)
