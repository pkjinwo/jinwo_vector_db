# JinWo VecDB PHP Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a PHP example for JinWo VecDB, demonstrating how to use PHP and FFI extension to call the C API for the vector database.

## Prerequisites

- PHP 7.4+ (with FFI extension enabled)
- Built JinWo VecDB library
- For Linux: `libjw_vecdb.so` in `../../build/`
- For Windows: `jw_vecdb.dll` in `../../build/`

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Enable FFI Extension

Ensure the FFI extension is enabled in your `php.ini` file:

```ini
; php.ini
; Uncomment the following line to enable FFI extension
extension=ffi
```

### 3. Run PHP Example

```bash
# Run PHP example
cd ../examples/php
php php_demo.php
```

## Feature Demonstration

The PHP example demonstrates the following features:

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

- `php_demo.php` - PHP demonstration program

## Technical Implementation

- Uses PHP's FFI extension to call C API
- Wraps C API, provides PHP-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### FFI Extension Not Available
- **Issue**: `Error: PHP FFI extension is not available`
- **Solution**: Enable the FFI extension in your `php.ini` file

### Library Not Found
- **Issue**: `Error: Could not find JinWo VecDB library`
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### PHP Version Issues
- **Issue**: PHP version incompatibility
- **Solution**: Use PHP 7.4 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [PHP FFI Documentation](https://www.php.net/manual/en/book.ffi.php)
