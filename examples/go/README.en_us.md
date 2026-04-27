# JinWo VecDB Go Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Go example for JinWo VecDB, demonstrating how to use Go and cgo to call the C API for the vector database.

## Prerequisites

- Go 1.18+
- C compiler (gcc for Linux, MinGW for Windows)
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run Go Example

```bash
# Run Go example
cd ../examples/go
go run go_demo.go
```

## Feature Demonstration

The Go example demonstrates the following features:

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

- `go_demo.go` - Go demonstration program
- `go.mod` - Go module definition

## Technical Implementation

- Uses Go's cgo feature to call C API
- Wraps C API, provides Go-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Library not found during linking
- **Solution**: Ensure JinWo VecDB library is properly built and located at `../../build/libjw_vecdb.a`

### CGO Issues
- **Issue**: CGO-related errors
- **Solution**: Ensure C compiler is installed
  - For Linux: `sudo apt install build-essential` (Ubuntu/Debian)
  - For Windows: Install MinGW or use WSL

### Go Version Issues
- **Issue**: Go version incompatibility
- **Solution**: Use Go 1.18 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Go CGO Documentation](https://golang.org/cmd/cgo/)
