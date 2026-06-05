# JinWo VecDB API Reference Documentation

**Version**: 1.0.0
**Generated**: 2026-04-26
**Project**: JinWo VecDB (金幄向量数据库)

---

## Table of Contents

1. [Overview](#overview)
2. [Core Types](#core-types)
3. [Database API](#database-api)
4. [Collection API](#collection-api)
5. [Vector Operations API](#vector-operations-api)
6. [Index API](#index-api)
7. [Quantization API](#quantization-api)
8. [Storage API](#storage-api)
9. [Error Codes](#error-codes)
10. [Usage Examples](#usage-examples)

---

## Overview

JinWo VecDB is an embedded vector database library written in pure C99. It provides:

- **Zero-configuration**: Works out of the box with no external dependencies
- **Cross-platform**: Supports Linux, Android, iOS, macOS, and Windows
- **High performance**: SIMD acceleration (SSE/AVX/NEON)
- **Multiple index types**: IVF, HNSW, and more
- **Vector quantization**: PQ and SQ support for memory optimization

### Version Information

```c
#define JW_VERSION_MAJOR       0
#define JW_VERSION_MINOR       1
#define JW_VERSION_PATCH       30
#define JW_VERSION_STRING      "0.1.32"
```

---

## Core Types

### Basic Integer Types

| Type | Description |
|------|-------------|
| `jw_int8_t` | 8-bit signed integer |
| `jw_uint8_t` | 8-bit unsigned integer |
| `jw_int16_t` | 16-bit signed integer |
| `jw_uint16_t` | 16-bit unsigned integer |
| `jw_int32_t` | 32-bit signed integer |
| `jw_uint32_t` | 32-bit unsigned integer |
| `jw_int64_t` | 64-bit signed integer |
| `jw_uint64_t` | 64-bit unsigned integer |

### Floating Point Types

| Type | Description |
|------|-------------|
| `jw_float32_t` | Single precision float (32-bit) |
| `jw_float64_t` | Double precision float (64-bit) |

### Boolean Type

```c
typedef int jw_bool_t;
#define JW_TRUE  1
#define JW_FALSE 0
```

### Status Codes

All API functions return `jw_status_t` status codes:
- `JW_SUCCESS` (0) indicates success
- Negative values indicate errors
- Positive values may indicate counts or sizes

---

## Database API

### Main Header

```c
#include "jw_vecdb.h"
```

### Database Lifecycle

#### `jw_vecdb_open`

Opens or creates a database.

```c
jw_status_t jw_vecdb_open(
    const jw_str_t *path,    // Database path (NULL for in-memory)
    jw_uint32_t flags,       // Open flags
    jw_vecdb_t **db          // Output database handle
);
```

**Flags**:
- `JW_VECDB_READONLY` - Open in read-only mode
- `JW_VECDB_READWRITE` - Open in read-write mode
- `JW_VECDB_CREATE` - Create if not exists
- `JW_VECDB_TRUNCATE` - Truncate existing database
- `JW_VECDB_MEMORY` - In-memory database
- `JW_VECDB_NOMMAP` - Disable memory mapping
- `JW_VECDB_SYNC` - Synchronous writes

**Example**:
```c
jw_vecdb_t *db;
jw_status_t status = jw_vecdb_open(
    jw_str("my_vecs.db"),
    JW_VECDB_CREATE | JW_VECDB_READWRITE,
    &db
);
```

#### `jw_vecdb_open_ex`

Opens database with detailed configuration.

```c
jw_status_t jw_vecdb_open_ex(
    const jw_vecdb_config_t *config,
    jw_vecdb_t **db
);
```

#### `jw_vecdb_close`

Closes the database.

```c
jw_status_t jw_vecdb_close(jw_vecdb_t *db);
```

#### `jw_vecdb_sync`

Synchronizes database to disk.

```c
jw_status_t jw_vecdb_sync(jw_vecdb_t *db);
```

### Collection Management

#### `jw_vecdb_create_collection`

Creates a new collection.

```c
jw_status_t jw_vecdb_create_collection(
    jw_vecdb_t *db,
    const jw_str_t *name,    // Collection name
    jw_dim_t dim,            // Vector dimension
    jw_collection_t **coll   // Output collection pointer
);
```

#### `jw_vecdb_create_collection_ex`

Creates collection with detailed configuration.

```c
jw_status_t jw_vecdb_create_collection_ex(
    jw_vecdb_t *db,
    const jw_collection_config_t *config,
    jw_collection_t **coll
);
```

#### `jw_vecdb_get_collection`

Gets an existing collection.

```c
jw_collection_t *jw_vecdb_get_collection(
    jw_vecdb_t *db,
    const jw_str_t *name
);
// Returns NULL if not found
```

#### `jw_vecdb_drop_collection`

Drops a collection.

```c
jw_status_t jw_vecdb_drop_collection(
    jw_vecdb_t *db,
    const jw_str_t *name
);
```

#### `jw_vecdb_has_collection`

Checks if a collection exists.

```c
jw_bool_t jw_vecdb_has_collection(
    const jw_vecdb_t *db,
    const jw_str_t *name
);
```

#### `jw_vecdb_list_collections`

Lists all collections.

```c
jw_size_t jw_vecdb_list_collections(
    const jw_vecdb_t *db,
    jw_str_t *names,         // Output array
    jw_size_t capacity       // Array capacity
);
```

### Convenience APIs

#### `jw_vecdb_insert`

Quick insert without explicit collection creation.

```c
jw_status_t jw_vecdb_insert(
    jw_vecdb_t *db,
    const jw_str_t *coll_name,
    jw_cvec_t vec,           // Vector data
    jw_dim_t dim,            // Vector dimension
    jw_vid_t *vid            // Output vector ID
);
```

#### `jw_vecdb_search`

Quick search.

```c
jw_size_t jw_vecdb_search(
    jw_vecdb_t *db,
    const jw_str_t *coll_name,
    jw_cvec_t query,         // Query vector
    jw_dim_t dim,
    jw_size_t k,              // Number of results
    jw_search_result_t *results
);
```

#### `jw_vecdb_insert_batch`

Batch insert.

```c
jw_status_t jw_vecdb_insert_batch(
    jw_vecdb_t *db,
    const jw_str_t *coll_name,
    jw_cvec_t vectors,
    jw_dim_t dim,
    jw_size_t count,
    jw_vid_t *vids           // Output ID array (can be NULL)
);
```

### Transaction APIs

#### `jw_vecdb_begin_transaction`

```c
jw_status_t jw_vecdb_begin_transaction(jw_vecdb_t *db);
```

#### `jw_vecdb_commit`

```c
jw_status_t jw_vecdb_commit(jw_vecdb_t *db);
```

#### `jw_vecdb_rollback`

```c
jw_status_t jw_vecdb_rollback(jw_vecdb_t *db);
```

#### `jw_vecdb_in_transaction`

```c
jw_bool_t jw_vecdb_in_transaction(const jw_vecdb_t *db);
```

### Backup and Restore APIs

#### `jw_vecdb_backup`

```c
jw_status_t jw_vecdb_backup(
    jw_vecdb_t *db,
    const jw_str_t *dest_path
);
```

#### `jw_vecdb_restore`

```c
jw_status_t jw_vecdb_restore(
    jw_vecdb_t **db,
    const jw_str_t *src_path
);
```

### Statistics APIs

#### `jw_vecdb_stats_t`

Statistics structure:

```c
typedef struct jw_vecdb_stats {
    jw_size_t collection_count;    // Number of collections
    jw_size_t total_vectors;       // Total vector count
    jw_uint64_t database_size;     // Database file size

    jw_uint64_t memory_used;       // Memory usage
    jw_uint64_t cache_used;        // Cache usage
    jw_uint64_t arena_used;        // Arena memory usage

    jw_uint64_t read_count;       // Read operations
    jw_uint64_t write_count;      // Write operations
    jw_uint64_t cache_hits;       // Cache hits
    jw_uint64_t cache_misses;     // Cache misses
    jw_float32_t cache_hit_rate;  // Cache hit rate
} jw_vecdb_stats_t;
```

#### `jw_vecdb_get_stats`

```c
jw_status_t jw_vecdb_get_stats(
    const jw_vecdb_t *db,
    jw_vecdb_stats_t *stats
);
```

#### `jw_vecdb_vacuum`

Performs database maintenance (defragmentation, optimization).

```c
jw_status_t jw_vecdb_vacuum(jw_vecdb_t *db);
```

#### `jw_vecdb_verify`

Verifies database integrity.

```c
jw_status_t jw_vecdb_verify(const jw_vecdb_t *db);
```

### Error Handling APIs

#### `jw_vecdb_get_last_error`

```c
jw_status_t jw_vecdb_get_last_error(const jw_vecdb_t *db);
```

#### `jw_vecdb_strerror`

```c
const char *jw_vecdb_strerror(jw_status_t status);
```

#### `jw_vecdb_get_error_message`

```c
const char *jw_vecdb_get_error_message(const jw_vecdb_t *db);
```

### Global Configuration APIs

#### `jw_vecdb_set_log_callback`

```c
typedef void (*jw_log_callback)(int level, const char *msg, void *user_data);
void jw_vecdb_set_log_callback(jw_log_callback callback, void *user_data);
```

#### `jw_vecdb_set_allocator`

```c
typedef void *(*jw_alloc_func)(size_t size);
typedef void *(*jw_realloc_func)(void *ptr, size_t size);
typedef void (*jw_free_func)(void *ptr);

jw_status_t jw_vecdb_set_allocator(
    jw_alloc_func alloc,
    jw_realloc_func realloc,
    jw_free_func free
);
```

#### `jw_vecdb_set_simd_enabled`

```c
void jw_vecdb_set_simd_enabled(jw_bool_t enable);
```

#### `jw_vecdb_is_simd_available`

```c
jw_bool_t jw_vecdb_is_simd_available(void);
```

---

## Collection API

### Main Header

```c
#include "jw_collection.h"
```

### Collection Structure

```c
typedef struct jw_collection_t {
    char *name;                 // Collection name
    jw_dim_t dim;              // Vector dimension
    jw_metric_t metric;         // Distance metric

    jw_record_t *records;      // Record array
    jw_size_t count;            // Current record count
    jw_size_t capacity;         // Capacity

    jw_index_t *index;         // Index pointer
    jw_bool_t index_enabled;   // Index enabled flag
    jw_size_t index_threshold; // Build index threshold

    jw_arena_t *arena;         // Memory arena
    jw_rwlock_t *lock;         // Read-write lock
} jw_collection_t;
```

### Vector Insertion

#### `jw_collection_insert`

```c
jw_status_t jw_collection_insert(
    jw_collection_t *coll,
    jw_cvec_t vec,
    jw_vid_t *vid              // Output vector ID (can be NULL)
);
```

#### `jw_collection_insert_with_meta`

```c
jw_status_t jw_collection_insert_with_meta(
    jw_collection_t *coll,
    jw_cvec_t vec,
    const jw_meta_field_t *fields,
    jw_size_t field_count,
    jw_vid_t *vid
);
```

#### `jw_collection_insert_batch`

```c
jw_status_t jw_collection_insert_batch(
    jw_collection_t *coll,
    jw_cvec_t vectors,
    jw_size_t count,
    jw_vid_t *vids             // Output ID array (can be NULL)
);
```

#### `jw_collection_upsert`

```c
jw_status_t jw_collection_upsert(
    jw_collection_t *coll,
    jw_vid_t vid,
    jw_cvec_t vec
);
```

### Vector Deletion

#### `jw_collection_delete`

```c
jw_status_t jw_collection_delete(
    jw_collection_t *coll,
    jw_vid_t vid
);
```

#### `jw_collection_delete_batch`

```c
jw_status_t jw_collection_delete_batch(
    jw_collection_t *coll,
    const jw_vid_t *vids,
    jw_size_t count
);
```

#### `jw_collection_clear`

```c
jw_status_t jw_collection_clear(jw_collection_t *coll);
```

### Vector Search

#### `jw_collection_search`

```c
typedef struct jw_search_result {
    jw_vid_t vid;              // Vector ID
    jw_score_t score;          // Similarity score
    void *metadata;            // Optional metadata
} jw_search_result_t;

jw_size_t jw_collection_search(
    jw_collection_t *coll,
    jw_cvec_t query,
    jw_size_t k,
    jw_search_result_t *results
);
```

#### `jw_collection_search_by_id`

Search by vector ID.

```c
jw_size_t jw_collection_search_by_id(
    jw_collection_t *coll,
    jw_vid_t vid,
    jw_size_t k,
    jw_search_result_t *results
);
```

---

## Vector Operations API

### Main Header

```c
#include "jw_vector.h"
```

### Distance Metrics

```c
typedef enum jw_metric {
    JW_METRIC_L2 = 0,          // Euclidean distance
    JW_METRIC_COSINE,          // Cosine similarity
    JW_METRIC_IP,              // Inner product
} jw_metric_t;
```

### Vector Operations

#### `jw_vec_normalize`

Normalizes a vector.

```c
void jw_vec_normalize(jw_vec_t vec, jw_dim_t dim);
```

#### `jw_vec_distance_l2`

Calculates L2 distance.

```c
jw_float32_t jw_vec_distance_l2(
    jw_cvec_t a,
    jw_cvec_t b,
    jw_dim_t dim
);
```

#### `jw_vec_distance_cosine`

Calculates cosine similarity.

```c
jw_float32_t jw_vec_distance_cosine(
    jw_cvec_t a,
    jw_cvec_t b,
    jw_dim_t dim
);
```

#### `jw_vec_dot_product`

Calculates dot product.

```c
jw_float32_t jw_vec_dot_product(
    jw_cvec_t a,
    jw_cvec_t b,
    jw_dim_t dim
);
```

---

## Index API

### Main Header

```c
#include "jw_index.h"
```

### Index Types

```c
typedef enum jw_index_type {
    JW_INDEX_NONE = -1,
    JW_INDEX_FLAT = 0,         // Brute force
    JW_INDEX_IVF,               // Inverted File Index
    JW_INDEX_HNSW,             // Hierarchical NSW
    JW_INDEX_IVF_PQ,           // IVF with PQ
} jw_index_type_t;
```

### Index Configuration

#### IVF Configuration

```c
typedef struct jw_ivf_config {
    jw_uint32_t nlist;          // Number of clusters
    jw_uint32_t nprobe;         // Clusters to search
} jw_ivf_config_t;
```

#### HNSW Configuration

```c
typedef struct jw_hnsw_config {
    jw_uint32_t m;              // Max connections per layer
    jw_uint32_t ef_construction; // Search width during build
    jw_uint32_t ef_search;      // Search width during search
    jw_uint32_t level;          // Number of levels
} jw_hnsw_config_t;
```

### Index APIs

#### `jw_index_create`

```c
jw_index_t *jw_index_create(
    jw_index_type_t type,
    jw_dim_t dim,
    const void *config
);
```

#### `jw_index_build`

```c
jw_status_t jw_index_build(
    jw_index_t *index,
    jw_cvec_t *vectors,
    jw_size_t count
);
```

#### `jw_index_search`

```c
jw_size_t jw_index_search(
    jw_index_t *index,
    jw_cvec_t query,
    jw_size_t k,
    jw_vid_t *result_ids,
    jw_score_t *result_scores
);
```

---

## Quantization API

### Main Header

```c
#include "jw_quant.h"
```

### Quantization Types

```c
typedef enum jw_quant_type {
    JW_QUANT_NONE = 0,
    JW_QUANT_UINT8,             // 8-bit unsigned
    JW_QUANT_INT8,              // 8-bit signed
    JW_QUANT_FLOAT16,           // 16-bit float
    JW_QUANT_PQ                 // Product Quantization
} jw_quant_type_t;
```

### Scalar Quantization

#### `jw_sq_create`

```c
jw_sq_t *jw_sq_create(
    jw_dim_t dim,
    jw_quant_type_t type
);
```

#### `jw_sq_train`

```c
jw_status_t jw_sq_train(
    jw_sq_t *sq,
    jw_cvec_t *vectors,
    jw_size_t count
);
```

#### `jw_sq_encode`

```c
jw_status_t jw_sq_encode(
    jw_sq_t *sq,
    jw_cvec_t vec,
    jw_uint8_t *out
);
```

#### `jw_sq_decode`

```c
jw_status_t jw_sq_decode(
    jw_sq_t *sq,
    const jw_uint8_t *codes,
    jw_vec_t vec
);
```

### Product Quantization

#### `jw_pq_create`

```c
jw_pq_t *jw_pq_create(
    jw_dim_t dim,
    jw_uint32_t m,              // Number of subvectors
    jw_uint32_t kbits           // Bits per subvector
);
```

#### `jw_pq_train`

```c
jw_status_t jw_pq_train(
    jw_pq_t *pq,
    jw_cvec_t *vectors,
    jw_size_t count
);
```

#### `jw_pq_encode`

```c
jw_status_t jw_pq_encode(
    jw_pq_t *pq,
    jw_cvec_t vec,
    jw_uint8_t *out
);
```

#### `jw_pq_decode`

```c
jw_status_t jw_pq_decode(
    jw_pq_t *pq,
    const jw_uint8_t *codes,
    jw_vec_t vec
);
```

#### `jw_pq_compute_distance`

```c
jw_float32_t jw_pq_compute_distance(
    jw_pq_t *pq,
    const jw_uint8_t *code,
    jw_cvec_t vec
);
```

---

## Storage API

### Main Header

```c
#include "jw_storage.h"
```

### Storage Modes

```c
typedef enum jw_storage_mode {
    JW_STORAGE_FILE = 0,        // File-based storage
    JW_STORAGE_MMAP,            // Memory-mapped file
    JW_STORAGE_HYBRID,          // Hybrid (mmap + file)
    JW_STORAGE_MEMORY            // In-memory storage
} jw_storage_mode_t;
```

### Storage Configuration

```c
typedef struct jw_storage_config {
    jw_storage_mode_t mode;     // Storage mode
    jw_size_t page_size;        // Page size
    jw_size_t cache_size;       // Cache size
    jw_bool_t sync_on_write;    // Sync after write
} jw_storage_config_t;
```

### Storage APIs

#### `jw_storage_create`

```c
jw_storage_t *jw_storage_create(
    const jw_str_t *path,
    const jw_storage_config_t *config
);
```

#### `jw_storage_open`

```c
jw_status_t jw_storage_open(
    jw_storage_t *storage
);
```

#### `jw_storage_close`

```c
jw_status_t jw_storage_close(
    jw_storage_t *storage
);
```

#### `jw_storage_write`

```c
jw_status_t jw_storage_write(
    jw_storage_t *storage,
    jw_off_t offset,
    const void *data,
    jw_size_t size
);
```

#### `jw_storage_read`

```c
jw_status_t jw_storage_read(
    jw_storage_t *storage,
    jw_off_t offset,
    void *data,
    jw_size_t size
);
```

---

## Error Codes

### General Errors (1-99)

| Code | Constant | Description |
|------|----------|-------------|
| 0 | `JW_SUCCESS` | Success |
| -1 | `JW_UNKNOWN_ERROR` | Unknown error |
| -2 | `JW_INVALID_PARAM` | Invalid parameter |
| -3 | `JW_OUT_OF_MEMORY` | Out of memory |
| -4 | `JW_NOT_FOUND` | Not found |
| -5 | `JW_ALREADY_EXISTS` | Already exists |
| -6 | `JW_BUFFER_TOO_SMALL` | Buffer too small |
| -7 | `JW_NOT_SUPPORTED` | Not supported |
| -8 | `JW_PERMISSION_DENIED` | Permission denied |
| -9 | `JW_TIMEOUT` | Timeout |
| -10 | `JW_BUSY` | Resource busy |
| -11 | `JW_EMPTY` | Empty object |
| -12 | `JW_TOO_BIG` | Too large |
| -13 | `JW_CANCELLED` | Cancelled |

### Vector Database Errors (100-199)

| Code | Constant | Description |
|------|----------|-------------|
| -100 | `JW_INVALID_VECTOR` | Invalid vector |
| -101 | `JW_INDEX_CORRUPTED` | Index corrupted |
| -102 | `JW_COLLECTION_FULL` | Collection full |
| -103 | `JW_INVALID_DIMENSION` | Invalid dimension |
| -104 | `JW_INDEX_NOT_READY` | Index not ready |
| -105 | `JW_VECTOR_EXISTS` | Vector exists |
| -106 | `JW_VECTOR_NOT_FOUND` | Vector not found |
| -107 | `JW_COLLECTION_EXISTS` | Collection exists |
| -108 | `JW_COLLECTION_NOT_FOUND` | Collection not found |
| -109 | `JW_INDEX_TYPE_MISMATCH` | Index type mismatch |
| -110 | `JW_QUANTIZATION_ERROR` | Quantization error |

### Storage Errors (200-299)

| Code | Constant | Description |
|------|----------|-------------|
| -200 | `JW_FILE_NOT_FOUND` | File not found |
| -201 | `JW_FILE_CORRUPTED` | File corrupted |
| -202 | `JW_DISK_FULL` | Disk full |
| -203 | `JW_IO_ERROR` | I/O error |
| -204 | `JW_FILE_LOCKED` | File locked |
| -205 | `JW_PATH_TOO_LONG` | Path too long |
| -206 | `JW_READ_ONLY` | Read-only mode |

### Concurrency Errors (300-399)

| Code | Constant | Description |
|------|----------|-------------|
| -300 | `JW_LOCK_TIMEOUT` | Lock timeout |
| -301 | `JW_DEADLOCK` | Deadlock |
| -302 | `JW_THREAD_ERROR` | Thread error |
| -303 | `JW_MUTEX_ERROR` | Mutex error |

---

## Usage Examples

### Basic Usage

```c
#include <jw_vecdb.h>

int main() {
    // Initialize
    jw_init();

    // Open database
    jw_vecdb_t *db;
    jw_status_t status = jw_vecdb_open(
        jw_str("my_vectors.jwv"),
        JW_VECDB_CREATE | JW_VECDB_READWRITE,
        &db
    );
    if (status != JW_SUCCESS) {
        printf("Failed to open database: %s\n",
               jw_vecdb_strerror(status));
        return -1;
    }

    // Create collection
    jw_collection_t *coll;
    status = jw_vecdb_create_collection(
        db,
        jw_str("documents"),
        1536,
        &coll
    );

    // Insert vectors
    float vec[1536] = { /* embedding data */ };
    jw_vid_t vid;
    jw_collection_insert(coll, vec, &vid);

    // Search
    jw_search_result_t results[10];
    jw_size_t count = jw_collection_search(
        coll,
        query_vec,
        10,
        results
    );

    // Close database
    jw_vecdb_close(db);

    // Cleanup
    jw_cleanup();
    return 0;
}
```

### In-Memory Database

```c
jw_vecdb_t *db;
jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db);
```

### Batch Operations

```c
// Batch insert
float vectors[1000][1536];
jw_vid_t vids[1000];

jw_collection_insert_batch(
    coll,
    (jw_cvec_t)vectors,
    1000,
    vids
);
```

### With Error Handling

```c
jw_status_t status;
const char *msg;

status = jw_vecdb_create_collection(db, jw_str("test"), 128, &coll);
if (status != JW_SUCCESS) {
    msg = jw_vecdb_strerror(status);
    printf("Error: %s\n", msg);
    // Handle error
}
```

---

## Platform Notes

### Linux

JinWo VecDB uses standard POSIX APIs on Linux. No special configuration required.

### macOS

Same as Linux. Tested with Xcode and GCC/Clang.

### iOS

- Compile as static library or framework
- Disable SIMD on arm64 simulators (use NEON instead)
- Test memory management carefully on devices

### Android

- Use NDK for compilation
- Consider disabling SIMD for broad compatibility
- Test with various device configurations

### Windows

- MSVC or MinGW-w64 recommended
- Disable SIMD for maximum compatibility
- Use UTF-8 encoding for paths

---

## Thread Safety

JinWo VecDB is thread-safe for:
- Multiple readers can access the same database
- Writers are serialized with locks
- Each collection has its own lock

**Note**: Do not share collection handles between threads without proper synchronization.

---

## Memory Management

- Use `jw_arena` for efficient memory allocation
- Database automatically cleans up resources on close
- Set custom allocators with `jw_vecdb_set_allocator` if needed

---

## Performance Tips

1. **Build index after bulk insert** - Call `jw_index_build` after batch insert
2. **Use appropriate index type** - HNSW for speed, IVF for memory
3. **Enable SIMD** - Use `jw_vecdb_set_simd_enabled(JW_TRUE)`
4. **Use quantization for large datasets** - PQ reduces memory significantly
5. **Batch operations** - Use batch APIs instead of single-item operations

---

**Document Version**: 1.0.0
**Last Updated**: 2026-04-26
