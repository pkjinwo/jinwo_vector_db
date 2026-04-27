# JinWo VecDB Kotlin Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Kotlin example for JinWo VecDB, demonstrating how to use Kotlin and JNA to call the C API for the vector database.

## Prerequisites

- JDK 11+
- Gradle 7.0+
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run Kotlin Example

```bash
# Run Kotlin example
cd ../examples/kotlin
./gradlew run
```

## Feature Demonstration

The Kotlin example demonstrates the following features:

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

- `build.gradle.kts` - Gradle build configuration
- `src/main/kotlin/Main.kt` - Kotlin demonstration program

## Technical Implementation

- Uses Kotlin's JNA library to call C API
- Wraps C API, provides Kotlin-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Compilation Errors
- **Issue**: Compilation failure
- **Solution**: Ensure JDK 11+ and Gradle 7.0+ are installed

### JNA Related Issues
- **Issue**: JNA loading failure
- **Solution**: Ensure JNA dependency is properly configured in build.gradle.kts

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [JNA Documentation](https://github.com/java-native-access/jna)
