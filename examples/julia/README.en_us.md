# JinWo VecDB Julia Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Julia example for JinWo VecDB, demonstrating how to use Julia and Libdl to call the C API for the vector database.

## Prerequisites

- Julia 1.6+
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run Julia Example

```bash
# Run Julia example
cd ../examples/julia
julia julia_demo.jl
```

## Feature Demonstration

The Julia example demonstrates the following features:

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

- `julia_demo.jl` - Julia demonstration program

## Technical Implementation

- Uses Julia's Libdl library to call C API
- Wraps C API, provides Julia-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Runtime Errors
- **Issue**: Runtime errors
- **Solution**: Ensure Julia version 1.6+ is installed

### Permission Issues
- **Issue**: Permission denied
- **Solution**: Ensure library file has executable permissions: `chmod +x ../../build/libjw_vecdb.so`

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Julia Libdl Documentation](https://docs.julialang.org/en/v1/base/libdl/)
