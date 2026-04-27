# JinWo VecDB Python Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Python example for JinWo VecDB, demonstrating how to use Python and ctypes to call the C API for the vector database.

## Prerequisites

- Python 3.6+
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

### 2. Run Python Example

```bash
# Run Python example
cd ../examples/python
python python_demo.py
```

## Feature Demonstration

The Python example demonstrates the following features:

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

- `python_demo.py` - Python demonstration program
- `requirements.txt` - Python dependencies

## Technical Implementation

- Uses Python's ctypes library to call C API
- Wraps C API, provides Python-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Permission Issues
- **Issue**: Permission denied
- **Solution**: Ensure library file has executable permissions: `chmod +x ../../build/libjw_vecdb.so`

### Python Version Issues
- **Issue**: Python version incompatibility
- **Solution**: Use Python 3.6 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Python ctypes Documentation](https://docs.python.org/3/library/ctypes.html)
