# JinWo VecDB Rust Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Rust example for JinWo VecDB, demonstrating how to use Rust and libc to call the C API for the vector database.

## Prerequisites

- Rust 1.60+
- Cargo 1.60+
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run Rust Example

```bash
# Run Rust example
cd ../examples/rust
cargo run
```

## Feature Demonstration

The Rust example demonstrates the following features:

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

- `Cargo.toml` - Rust project configuration
- `src/main.rs` - Rust demonstration program

## Technical Implementation

- Uses Rust's libc library to call C API
- Wraps C API, provides Rust-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Compilation Errors
- **Issue**: Compilation failure
- **Solution**: Ensure Rust version 1.60+ is installed

### Permission Issues
- **Issue**: Permission denied
- **Solution**: Ensure library file has executable permissions: `chmod +x ../../build/libjw_vecdb.so`

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Rust libc Documentation](https://doc.rust-lang.org/libc/)
