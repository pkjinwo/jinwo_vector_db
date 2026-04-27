# JinWo VecDB Linux Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Linux example for JinWo VecDB, demonstrating how to use C language to directly call the C API for the vector database.

## Prerequisites

- GCC 11+
- CMake 3.20+
- Built JinWo VecDB library (`libjw_vecdb.a`)

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Build and Run Example

#### Using CMake

```bash
# Build using CMake
cd ../examples/linux
mkdir -p build && cd build
cmake .. && make
./linux_demo
```

#### Using Makefile

```bash
# Build using Makefile
cd ../examples/linux
make
./linux_demo

# Or run directly
make run
```

## Feature Demonstration

The Linux example demonstrates the following features:

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

- `linux_demo.c` - C language demonstration program
- `CMakeLists.txt` - CMake build configuration
- `Makefile` - Make build configuration

## Technical Implementation

- Directly calls JinWo VecDB C API
- Uses CMake or Make build system
- Native C language implementation

## Troubleshooting

### Link Errors
- **Issue**: Library not found during linking
- **Solution**: Ensure JinWo VecDB library is properly built and located at `../../build/libjw_vecdb.a`

### Compilation Errors
- **Issue**: Compilation failure
- **Solution**: Ensure GCC version 11+ is installed

### Permission Issues
- **Issue**: Permission denied
- **Solution**: Ensure executable permissions: `chmod +x linux_demo`

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
