# JinWo VecDB Architecture Design Document

**Version**: v1.0.0
**Generated**: 2026-04-26
**Document Type**: Public Release

---

## 1. Architecture Overview

### 1.1 Project Introduction

JinWo VecDB is a lightweight vector database designed for embedded systems and mobile devices, supporting efficient vector storage, indexing, and retrieval. The project is developed in pure C language with the following features:

- **Lightweight**: Minimal dependencies, suitable for embedded and mobile deployment
- **High performance**: Supports SIMD acceleration, providing efficient vector operations
- **Cross-platform**: Supports Linux, Android, iOS, Windows and other mainstream platforms
- **Easy integration**: Provides concise C API and multiple language bindings

### 1.2 Core Architecture

JinWo VecDB adopts a layered architecture design, mainly divided into the following layers:

| Layer | Description | Main Modules |
|-------|-------------|-------------|
| API Layer | External interface layer | jw_vecdb.h |
| Business Logic Layer | Core business processing | Collection, Index, Vector |
| Storage Layer | Data persistence | Storage, File |
| Infrastructure Layer | Basic tools | Arena, Lock, Hash |

### 1.3 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        API Layer                           │
│  jw_vecdb_open/close, jw_collection_*, jw_vector_*          │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     Business Logic Layer                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  Collection │  │   Index     │  │   Vector    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                       Storage Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  Storage    │  │    File     │  │   Config    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     Infrastructure Layer                   │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │  Arena  │ │  Lock   │ │  Hash   │ │  Math   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Core Module Design

### 2.1 VecDB Main Module

**Files**: `src/jw_vecdb.c`, `include/jw_vecdb.h`

**Functions**:
- Database instance management
- Global configuration management
- Collection management entry

**Key Data Structures**:

```c
typedef struct {
    char* db_path;
    bool is_open;
    Arena* arena;
    Collection* collections;
    size_t collection_count;
    size_t max_collections;
} jw_vecdb_t;
```

**API Interfaces**:

| Function | Description |
|----------|-------------|
| `jw_vecdb_open` | Open database |
| `jw_vecdb_close` | Close database |
| `jw_vecdb_create_collection` | Create collection |
| `jw_vecdb_drop_collection` | Drop collection |
| `jw_vecdb_list_collections` | List collections |

### 2.2 Collection Module

**Files**: `src/jw_collection.c`, `include/jw_collection.h`

**Functions**:
- Collection management
- Vector index management
- Collection-level configuration

**Key Data Structures**:

```c
typedef struct {
    char* name;
    size_t vector_dimension;
    size_t vector_count;
    size_t max_vectors;
    Index* index;
    Arena* arena;
    bool is_open;
} Collection;
```

**API Interfaces**:

| Function | Description |
|----------|-------------|
| `jw_collection_create` | Create collection |
| `jw_collection_open` | Open collection |
| `jw_collection_close` | Close collection |
| `jw_collection_insert` | Insert vector |
| `jw_collection_search` | Search vector |

### 2.3 Index Module

**Files**: `src/jw_index.c`, `include/jw_index.h`

**Functions**:
- Vector index construction
- Similarity search
- Index optimization

**Index Algorithms**:

| Algorithm | Description | Application Scenario |
|-----------|-------------|----------------------|
| Brute-force | Linear scan of all vectors | Small datasets |
| IVF | Inverted File Index | Medium datasets |
| PQ | Product Quantization | Large datasets, memory-constrained |

**Key Data Structures**:

```c
typedef struct {
    IndexType type;
    size_t dimension;
    size_t vector_count;
    void* index_data;
    Quantizer* quantizer;
} Index;
```

### 2.4 Vector Module

**Files**: `src/jw_vector.c`, `include/jw_vector.h`

**Functions**:
- Vector data management
- Vector operations
- Distance calculation

**Key Data Structures**:

```c
typedef struct {
    uint64_t id;
    float* data;
    size_t dimension;
    uint32_t flags;
} Vector;
```

**Distance Metrics**:

| Metric | Formula | Application Scenario |
|--------|---------|----------------------|
| L2 | √(Σ(a-b)²) | Image, audio features |
| Cosine | a·b/(|a||b|) | Text embeddings |
| Dot Product | Σab | Recommendation systems |

### 2.5 Storage Module

**Files**: `src/jw_storage.c`, `include/jw_storage.h`

**Functions**:
- Data persistence
- Memory mapping
- Data recovery

**Storage Format**:

```
┌────────────────────────────────────────────┐
│              Storage Header                │
│  magic, version, checksum, metadata        │
├────────────────────────────────────────────┤
│           Collection Metadata               │
│  collection count, collection offsets       │
├────────────────────────────────────────────┤
│           Collection Data #1                │
│  vectors, index, metadata                   │
├────────────────────────────────────────────┤
│           Collection Data #2                │
│  ...                                        │
└────────────────────────────────────────────┘
```

### 2.6 Arena Memory Pool Module

**Files**: `src/jw_arena.c`, `include/jw_arena.h`

**Functions**:
- Memory allocation management
- Memory pool implementation
- Resource tracking

**Key Features**:

| Feature | Description |
|---------|-------------|
| Linear Allocation | Simple and efficient memory allocation |
| Batch Release | Release all memory at once |
| Memory Tracking | Record memory usage |

---

## 3. Data Flow Design

### 3.1 Vector Insertion Flow

```
User Request
    │
    ▼
jw_collection_insert()
    │
    ▼
Parameter Validation ─────────────────────┐
    │                                     │
    ▼                                     ▼
Arena Memory Allocation            Return Error
    │
    ▼
Vector Creation
    │
    ▼
Index Update ────► Asynchronous Index Building
    │
    ▼
Storage Persistence ────► Background Write
    │
    ▼
Return Vector ID
```

### 3.2 Vector Search Flow

```
User Request
    │
    ▼
jw_collection_search()
    │
    ▼
Parameter Validation ─────────────────────┐
    │                                     │
    ▼                                     ▼
Index Query                            Return Error
    │
    ▼
Distance Calculation (Optional SIMD Acceleration)
    │
    ▼
Result Sorting
    │
    ▼
Return Top-K Results
```

---

## 4. Memory Management Design

### 4.1 Memory Architecture

| Component | Memory Type | Description |
|-----------|-------------|-------------|
| Arena | Heap Memory | Main memory pool |
| Vector Data | Heap Memory | Vector data storage |
| Index Structure | Heap Memory | Index data structure |
| MMAP Area | Virtual Memory | Memory-mapped files |

### 4.2 Memory Allocation Strategy

| Operation | Allocation Strategy | Release Strategy |
|-----------|---------------------|------------------|
| Small Objects (<1KB) | Arena linear allocation | Arena unified release |
| Medium Objects (1KB-1MB) | Individual malloc | Individual free |
| Large Objects (>1MB) | mmap | munmap |

### 4.3 Memory Safety Mechanisms

| Mechanism | Description | Implementation |
|-----------|-------------|----------------|
| Boundary Check | Array boundary validation | assert/if check |
| Memory Initialization | Clear memory on allocation | memset |
| Double Free Detection | Detect duplicate free | Flag bit |
| Memory Leak Detection | Track allocation and release | Logging/tools |

---

## 5. Concurrency Design

### 5.1 Thread Model

```
┌─────────────────┐
│   Main Thread   │
│  (Initialization/Close)  │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│   Worker Thread 1     │────▶│   Worker Thread 2     │
│  (Vector Insertion)    │     │  (Vector Search)      │
└─────────────────┘     └─────────────────┘
         │                       │
         └───────────┬───────────┘
                     ▼
         ┌─────────────────┐
         │   Lock Manager   │
         │  (Read/Write Locks)│
         └─────────────────┘
                     │
         ┌───────────┴───────────┐
         ▼                       ▼
┌─────────────────┐     ┌─────────────────┐
│   Collection 1   │     │   Collection 2  │
└─────────────────┘     └─────────────────┘
```

### 5.2 Lock Strategy

| Lock Type | Application Scenario | Granularity |
|-----------|----------------------|-------------|
| Read-Write Lock | Collection access | Collection-level |
| Spin Lock | Short critical sections | Arena-level |
| Atomic Operation | Counters | Field-level |

### 5.3 Thread-Safe APIs

```c
jw_vecdb_open();        // Thread-safe
jw_vecdb_close();       // Thread-safe
jw_collection_insert(); // Thread-safe (single Collection)
jw_collection_search(); // Thread-safe (read operation)
jw_collection_delete(); // Thread-safe (single Collection)
```

---

## 6. Storage Design

### 6.1 Persistence Format

**File Structure**:

```
database.jwd/
├── header.db         # Database header
├── collection_0.db  # Collection data
├── collection_1.db  # Collection data
└── wal.db           # Write-Ahead Log
```

**Header Format**:

```c
typedef struct {
    uint32_t magic;        // Magic number 0x4A574442 ("JWDB")
    uint32_t version;      // Version number
    uint32_t checksum;     // Checksum
    uint64_t created_at;  // Creation time
    uint64_t modified_at; // Modification time
    size_t collection_count;
    size_t metadata_size;
} StorageHeader;
```

### 6.2 Write-Ahead Log (WAL)

| Feature | Description |
|---------|-------------|
| Atomicity | Ensure data consistency |
| Recovery | Recovery after system crash |
| Performance | Batch write optimization |

### 6.3 Memory Mapping

| Mode | Application Scenario | Advantage |
|------|----------------------|-----------|
| MAP_SHARED | Production environment | Persistence support |
| MAP_PRIVATE | Temporary data | Write performance |

---

## 7. Extensibility Design

### 7.1 Module Extension

| Extension Point | Description | Implementation |
|-----------------|-------------|----------------|
| Index Algorithm | Add new index types | Implement Index interface |
| Quantization Method | Add new quantization algorithms | Implement Quantizer interface |
| Distance Metric | Add new distance functions | Function pointer registration |
| Storage Backend | Add new storage engines | Implement Storage interface |

### 7.2 Interface Extension

```c
typedef struct {
    int (*init)(void** ctx);
    int (*insert)(void* ctx, const void* data, size_t size, uint64_t* out_id);
    int (*search)(void* ctx, const void* query, size_t k, uint64_t* out_ids, float* scores);
    int (*destroy)(void* ctx);
} IndexInterface;
```

### 7.3 Configuration Extension

```c
typedef struct {
    size_t max_vectors;
    size_t dimension;
    IndexType index_type;
    QuantizerType quantizer_type;
    DistanceMetric distance_metric;
    void* custom_params;
} CollectionConfig;
```

---

## 8. Error Handling Design

### 8.1 Error Code System

| Error Code | Description | Severity |
|------------|-------------|----------|
| JW_OK | Success | - |
| JW_ERROR | General error | Low |
| JW_OUT_OF_MEMORY | Out of memory | Medium |
| JW_INVALID_PARAMETER | Invalid parameter | Medium |
| JW_NOT_FOUND | Not found | Low |
| JW_ALREADY_EXISTS | Already exists | Low |
| JW_STORAGE_ERROR | Storage error | High |
| JW_CORRUPTED | Data corrupted | High |

### 8.2 Error Handling Strategy

| Scenario | Handling Strategy |
|----------|-------------------|
| Memory allocation failure | Return error, clean up resources |
| Storage write failure | Retry, log record |
| Data corruption | Try to recover, backup rollback |
| Concurrency conflict | Retry or return error |

---

## 9. Configuration Management

### 9.1 Runtime Configuration

```c
typedef struct {
    size_t arena_block_size;     // Arena block size
    size_t max_memory;           // Maximum memory limit
    size_t thread_pool_size;     // Thread pool size
    bool enable_wal;             // Enable write-ahead log
    bool enable_mmap;            // Enable memory mapping
    LogLevel log_level;          // Log level
} VecDBConfig;
```

### 9.2 Collection Configuration

```c
typedef struct {
    size_t max_vectors;          // Maximum number of vectors
    size_t dimension;            // Vector dimension
    IndexType index_type;        // Index type
    size_t index_params;        // Index parameters
    bool enable_compression;     // Enable compression
} CollectionConfig;
```

---

## 10. Version Compatibility

### 10.1 Storage Format Compatibility

| Version | Compatibility | Description |
|---------|---------------|-------------|
| v0.x | Incompatible | Early versions |
| v1.x | Backward compatible | Support reading old version data |

### 10.2 API Compatibility

| Type | Compatibility Strategy |
|------|------------------------|
| Public API | Stable, not deleted |
| Internal API | Can be changed |
| Experimental API | May be changed |

---

**Document Update Records**

| Date | Version | Update Content |
|------|---------|----------------|
| 2026-04-26 | v1.0.0 | Initial version |
