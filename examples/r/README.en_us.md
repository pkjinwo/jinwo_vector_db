# JinWo VecDB R Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains an R example for JinWo VecDB, demonstrating how to use R and Rcpp to call the C API for the vector database.

## Prerequisites

- R 4.0+
- Rcpp package
- Built JinWo VecDB library

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Run R Example

```bash
# Run R example
cd ../examples/r
Rscript r_demo.R
```

## Feature Demonstration

The R example demonstrates the following features:

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

- `r_demo.R` - R demonstration program

## Technical Implementation

- Uses R's Rcpp package to call C API
- Wraps C API, provides R-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Library Not Found
- **Issue**: Cannot find JinWo VecDB library
- **Solution**: Ensure the library is properly built and located in the correct path (`../../build/`)

### Rcpp Related Issues
- **Issue**: Rcpp compilation failure
- **Solution**: Ensure Rcpp package is installed: `install.packages("Rcpp")`

### R Version Issues
- **Issue**: R version incompatibility
- **Solution**: Use R 4.0 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Rcpp Documentation](https://cran.r-project.org/web/packages/Rcpp/index.html)
