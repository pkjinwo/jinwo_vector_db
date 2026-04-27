# JinWo VecDB Dart Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Dart example for JinWo VecDB, demonstrating how to use Dart and dart:ffi to call the C API for the vector database.

## Prerequisites

- Dart SDK 3.0+
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run Dart Example

```bash
# Run Dart example
cd ../examples/dart
pub get
dart run bin/dart_demo.dart
```

## Feature Demonstration

The Dart example demonstrates the following features:

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

- `pubspec.yaml` - Dart project configuration
- `bin/dart_demo.dart` - Dart demonstration program

## Technical Implementation

- Uses Dart's dart:ffi library to call C API
- Wraps C API, provides Dart-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Compilation Errors
- **Issue**: Compilation failure
- **Solution**: Ensure Dart SDK 3.0+ is installed

### FFI Related Issues
- **Issue**: FFI loading failure
- **Solution**: Ensure dart:ffi library is properly used

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Dart FFI Documentation](https://dart.dev/guides/libraries/c-interop)
