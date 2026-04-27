[English](README.en_us.md) | [中文](README.md)

# JinWo VecDB

**JinWo VecDB** is an open-source, cross-platform, embedded vector database designed specifically for mobile and embedded devices.

## Features

- 🔥 **Pure C Implementation** - C99 standard, no external dependencies, easy to integrate
- 🚀 **Cross-platform** - Supports Android, iOS, macOS, Windows, Linux (five major platforms)
- 🎯 **High-performance Indexing** - Supports IVF and HNSW indexing algorithms
- ⚡ **SIMD Acceleration** - Automatically detects and uses SSE/AVX/NEON instruction sets
- 📦 **Zero Configuration** - Ready to use out of the box, no complex configuration required
- 📊 **Vector Quantization** - Supports PQ (Product Quantization) and SQ (Scalar Quantization)
- 📱 **Mobile Optimization** - Memory and performance optimized for mobile devices and embedded systems
- 🔧 **Complete API** - Provides full vector database operation API
- 🧪 **Comprehensive Testing** - Includes comprehensive unit tests, concurrent tests, and performance tests
- 🌐 **Multi-language Support** - Supports C, C#, Java, Swift, Python, Go, Node.js, PHP, Rust, Kotlin, TypeScript, R, Dart, Julia
- 🔄 **CI/CD Integration** - Complete continuous integration and deployment pipeline

## Quick Start

### Build the Project

```bash
# Clone the repository
git clone https://github.com/pkjinwo/jinwo_vector_db.git  #International
git clone https://gitee.com/pkjinwo/jinwo_vector_db.git  #China
cd jinwo_vector_db

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
make test

# Install
sudo make install
```

### Basic Usage

```c
#include <jw_vecdb.h>

int main() {
    // Initialize
    jw_init();
    
    // Create/open database
    jw_vecdb_t *db;
    jw_vecdb_open("my_vectors.jwv", JW_VECDB_CREATE, &db);
    
    // Create Collection
    jw_collection_t *coll;
    jw_vecdb_create_collection(db, "documents", 1536, &coll);
    
    // Insert vector
    float vec[1536] = { /* embedding data */ };
    jw_vid_t vid;
    jw_collection_insert(coll, vec, &vid);
    
    // Search for similar vectors
    jw_search_result_t results[10];
    jw_size_t count = jw_collection_search(coll, query_vec, 10, results);
    
    // Close database
    jw_vecdb_close(db);
    
    jw_cleanup();
    return 0;
}
```

## Project Structure

```
jw_vecdb/
├── include/           # Header files
│   ├── jw_types.h     # Basic type definitions
│   ├── jw_arena.h     # Memory pool
│   ├── jw_vector.h    # Vector operations
│   ├── jw_lock.h      # Lock mechanism
│   ├── jw_index.h     # Index structure
│   ├── jw_collection.h# Vector collection
│   ├── jw_storage.h   # Storage abstraction layer
│   ├── jw_quant.h     # Vector quantization
│   ├── jw_config.h    # Configuration management
│   ├── jw_file.h      # File operations
│   ├── jw_hash.h      # Hash table
│   ├── jw_log.h       # Logging system
│   ├── jw_math.h      # Math utilities
│   ├── jw_sort.h      # Sorting algorithms
│   ├── jw_string.h    # String operations
│   └── jw_vecdb.h     # Main interface
├── src/               # Source code implementation
├── examples/          # Example programs
│   ├── c/             # C language examples
│   ├── csharp/        # C# examples
│   ├── java/          # Java examples
│   ├── swift/         # Swift examples
│   ├── python/        # Python examples
│   ├── go/            # Go examples
│   ├── nodejs/        # Node.js examples
│   ├── php/           # PHP examples
│   ├── rust/          # Rust examples
│   ├── kotlin/        # Kotlin examples
│   ├── typescript/    # TypeScript examples
│   ├── r/             # R language examples
│   ├── dart/          # Dart examples
│   ├── julia/         # Julia examples
│   ├── android/       # Android examples
│   ├── ios/           # iOS examples
│   ├── windows/       # Windows examples
│   ├── linux/         # Linux examples
│   └── macos/         # macOS examples
├── tests/             # Unit tests
│   ├── test_types.c   # Basic type tests
│   ├── test_string.c  # String tests
│   ├── test_arena.c   # Memory pool tests
│   ├── test_vector.c  # Vector operation tests
│   ├── test_math.c    # Math function tests
│   ├── test_sort.c    # Sorting algorithm tests
│   ├── test_hash.c    # Hash table tests
│   ├── test_lock.c    # Lock mechanism tests
│   ├── test_file.c    # File operation tests
│   ├── test_storage.c # Storage layer tests
│   ├── test_index.c   # Index algorithm tests
│   ├── test_quantization.c # Quantization tests
│   ├── test_config.c  # Configuration tests
│   ├── test_vecdb.c   # Main interface tests
│   ├── test_collection.c # Collection tests
│   ├── test_concurrent.c # Concurrent tests
│   └── performance/   # Performance tests
├── cmake/             # CMake configuration
├── docs/              # Documentation
│   ├── en_us/         # English documentation
│   └── zh_cn/         # Chinese documentation
├── local_doc/         # Local documentation
├── .github/workflows/ # CI/CD configuration
└── CMakeLists.txt     # Build configuration
```

## API Design

### Core Modules

| Module | Description |
|--------|-------------|
| jw_types | Basic type definitions, cross-platform compatible |
| jw_arena | Memory pool management, efficient memory allocation |
| jw_vector | Vector operations, SIMD accelerated |
| jw_lock | Lock mechanism, multi-thread support |
| jw_index | Index algorithms, IVF/HNSW |
| jw_collection | Vector collection management |
| jw_storage | Storage abstraction layer |
| jw_quant | Vector quantization, PQ/SQ support |
| jw_config | Configuration management |
| jw_file | File operations |
| jw_hash | Hash table implementation |
| jw_log | Logging system |
| jw_math | Math utility functions |
| jw_sort | Sorting algorithms |
| jw_string | String operations |
| jw_vecdb | Main interface API |

### Index Algorithms

#### IVF (Inverted File Index)
- Suitable for large-scale datasets (over 10 million)
- Low memory usage
- Fast location through cluster centers

#### HNSW (Hierarchical Navigable Small World)
- Fast query speed, high accuracy
- Suitable for medium-small scale data (up to 1 million)
- Graph structure storage, friendly to incremental updates

## Performance

Benchmark results in standard desktop environment (128-dimensional vectors):

| Operation | Performance |
|-----------|-------------|
| Dot Product | ~10M ops/sec |
| L2 Distance | ~8M ops/sec |
| Cosine Similarity | ~6M ops/sec |
| Normalization | ~5M ops/sec |

*Actual performance depends on hardware configuration and SIMD support*

## Roadmap

### v0.1.0 (Completed)
- [x] Basic type definitions
- [x] Memory pool management
- [x] Vector operations (with SIMD acceleration)
- [x] Lock mechanism
- [x] Index structure design

### v0.2.0 (Completed)
- [x] IVF index complete implementation
- [x] HNSW index complete implementation
- [x] Storage layer implementation
- [x] Collection complete implementation

### v0.3.0 (Completed)
- [x] PQ/SQ quantization support
- [x] Batch operation optimization
- [x] Parallel query

### v1.0.0 (Completed)
- [x] Complete feature set
- [x] Stable API
- [x] Comprehensive documentation
- [x] Full platform support
- [x] Multi-language bindings
- [x] Concurrent testing
- [x] CI/CD integration

## Platform Support

| Platform | Support Status | Integration Documentation | Example Code |
|----------|----------------|---------------------------|-------------|
| Linux | ✅ Completed | ✅ Provided | ✅ Provided |
| macOS | ✅ Completed | ✅ Provided | ✅ Provided |
| iOS | ✅ Completed | ✅ Provided | ✅ Provided |
| Android | ✅ Completed | ✅ Provided | ✅ Provided |
| Windows | ✅ Completed | ✅ Provided | ✅ Provided |

## Language Support

| Language | Support Status | Example Code | Documentation |
|----------|----------------|-------------|---------------|
| C | ✅ Completed | ✅ Provided | ✅ Provided |
| C# | ✅ Completed | ✅ Provided | ✅ Provided |
| Java | ✅ Completed | ✅ Provided | ✅ Provided |
| Swift | ✅ Completed | ✅ Provided | ✅ Provided |
| Python | ✅ Completed | ✅ Provided | ✅ Provided |
| Go | ✅ Completed | ✅ Provided | ✅ Provided |
| Node.js | ✅ Completed | ✅ Provided | ✅ Provided |
| PHP | ✅ Completed | ✅ Provided | ✅ Provided |
| Rust | ✅ Completed | ✅ Provided | ✅ Provided |
| Kotlin | ✅ Completed | ✅ Provided | ✅ Provided |
| TypeScript | ✅ Completed | ✅ Provided | ✅ Provided |
| R | ✅ Completed | ✅ Provided | ✅ Provided |
| Dart | ✅ Completed | ✅ Provided | ✅ Provided |
| Julia | ✅ Completed | ✅ Provided | ✅ Provided |

## Contribution Guide

Welcome to contribute code, report issues, or suggest improvements!

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Create a Pull Request

## License

This project is open source under the [Apache 2.0](LICENSE) license.

## About

**JinWo VecDB** is developed and maintained by [Beijing JinWo Technology Co., Ltd.](https://jinwo.site).

- Website: https://jinwo.site
- Documentation: https://docs.jinwo.site/jinwo_vector_db
- Issue Tracking: https://github.com/pkjinwo/jinwo_vector_db/issues  #International
- Issue Tracking: https://gitee.com/pkjinwo/jinwo_vector_db/issues  #China