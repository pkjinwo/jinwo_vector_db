# JinWo VecDB TypeScript Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a TypeScript example for JinWo VecDB, demonstrating how to use TypeScript and ffi-napi to call the C API for the vector database.

## Prerequisites

- Node.js 14+
- npm 6+
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Install Dependencies

```bash
# Install dependencies
cd ../examples/typescript
npm install
```

### 3. Run TypeScript Example

```bash
# Run TypeScript example
npm start
```

## Feature Demonstration

The TypeScript example demonstrates the following features:

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

- `package.json` - Node.js project configuration
- `tsconfig.json` - TypeScript configuration
- `src/index.ts` - TypeScript demonstration program

## Technical Implementation

- Uses TypeScript's ffi-napi library to call C API
- Wraps C API, provides TypeScript-callable interface
- Directly calls JinWo VecDB C API

## Dependencies

- `ffi-napi` - Node.js foreign function interface for calling C functions
- `ref-napi` - Reference handling for C types
- `ref-struct-napi` - Struct handling for C structures

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### FFI Installation Issues
- **Issue**: Problems installing ffi-napi or related packages
- **Solution**:
  - For Linux: `sudo apt install build-essential` (Ubuntu/Debian)
  - For Windows: Install Visual Studio Build Tools
  - For macOS: Install Xcode Command Line Tools

### Node.js Version Issues
- **Issue**: Node.js version incompatibility
- **Solution**: Use Node.js 14 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [ffi-napi Documentation](https://github.com/node-ffi-napi/node-ffi-napi)
