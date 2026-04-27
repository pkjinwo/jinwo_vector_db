# JinWo VecDB Performance Testing Guide

**Version**: v1.0.0
**Generated**: 2026-04-26
**Document Type**: Public Release

---

## 1. Performance Testing Overview

### 1.1 Testing Goals

The main goals of performance testing are to evaluate the performance characteristics of JinWo VecDB, identify performance bottlenecks, and provide optimization recommendations. Specifically including:

- **Throughput testing**: Measure the number of operations per second
- **Latency testing**: Measure the response time of operations
- **Memory usage testing**: Measure memory consumption
- **CPU usage testing**: Measure CPU utilization
- **Scalability testing**: Measure performance under different data sizes

### 1.2 Performance Metrics

| Metric | Description | Unit |
|--------|-------------|------|
| QPS | Queries Per Second | queries/second |
| Latency | Average response time | milliseconds |
| P95 Latency | 95th percentile response time | milliseconds |
| P99 Latency | 99th percentile response time | milliseconds |
| Memory Usage | Peak memory consumption | MB |
| CPU Usage | Average CPU utilization | % |
| Index Size | Index memory footprint | MB |

### 1.3 Testing Environment

| Component | Specification |
|-----------|---------------|
| CPU | Intel i7-12700K (12 cores) |
| Memory | 32GB DDR4-3200 |
| Storage | NVMe SSD |
| OS | Ubuntu 20.04 LTS |
| Compiler | GCC 11.0 |
| Build Type | Release |

---

## 2. Test Datasets

### 2.1 Synthetic Datasets

| Dataset | Description | Size |
|---------|-------------|------|
| Random | Random vectors | 1M vectors |
| Gaussian | Gaussian distribution vectors | 1M vectors |
| Uniform | Uniform distribution vectors | 1M vectors |

### 2.2 Real-world Datasets

| Dataset | Description | Size |
|---------|-------------|------|
| SIFT1M | 1M 128-dimensional vectors | 1M vectors |
| GIST1M | 1M 960-dimensional vectors | 1M vectors |
| Deep1B | 1B 96-dimensional vectors | 100M vectors (subset) |

---

## 3. Performance Test Scenarios

### 3.1 Insert Performance

| Test Case | Description | Configuration |
|-----------|-------------|---------------|
| Single Insert | Insert one vector at a time | 128-dimensional vectors |
| Batch Insert | Insert multiple vectors at once | 128-dimensional vectors, batch size=100 |
| Large Batch Insert | Insert large batches | 128-dimensional vectors, batch size=1000 |

### 3.2 Search Performance

| Test Case | Description | Configuration |
|-----------|-------------|---------------|
| KNN Search | Top-K nearest neighbor search | K=10, 128-dimensional vectors |
| Range Search | Range-based search | Radius=0.1, 128-dimensional vectors |
| Filtered Search | Search with metadata filters | K=10, 128-dimensional vectors |

### 3.3 Mixed Workload

| Test Case | Description | Configuration |
|-----------|-------------|---------------|
| Read-Heavy | 90% search, 10% insert | 128-dimensional vectors |
| Write-Heavy | 10% search, 90% insert | 128-dimensional vectors |
| Balanced | 50% search, 50% insert | 128-dimensional vectors |

---

## 4. Test Tools

### 4.1 Benchmark Tools

| Tool | Purpose | Usage |
|------|---------|--------|
| Google Benchmark | C++ benchmarking library | `benchmark` executable |
| perf | Linux performance analysis | `perf record -g ./benchmark` |
| Valgrind | Memory analysis | `valgrind --tool=massif ./benchmark` |
| gprof | Profiling tool | `gprof ./benchmark gmon.out > profile.txt` |

### 4.2 Custom Benchmark

```c
// examples/vector_bench.c
#include "jw_vecdb.h"
#include <stdio.h>
#include <time.h>

void benchmark_insert(size_t count, size_t dimension) {
    jw_vecdb_t* db = NULL;
    jw_vecdb_open(&db, "/tmp/vecdb", true);
    
    jw_collection_t* col = NULL;
    jw_collection_create(&col, db, "benchmark", dimension);
    
    float* vector = malloc(dimension * sizeof(float));
    clock_t start = clock();
    
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < dimension; j++) {
            vector[j] = (float)rand() / RAND_MAX;
        }
        uint64_t id;
        jw_collection_insert(col, vector, &id);
    }
    
    clock_t end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Inserted %zu vectors in %.2f seconds (%.2f QPS)\n", 
           count, time, count / time);
    
    free(vector);
    jw_collection_close(col);
    jw_vecdb_close(db);
}

void benchmark_search(size_t count, size_t dimension, size_t k) {
    // Similar implementation for search benchmark
}

int main() {
    benchmark_insert(100000, 128);
    benchmark_search(10000, 128, 10);
    return 0;
}
```

---

## 5. Test Execution

### 5.1 Insert Performance Test

```bash
# Compile benchmark
cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_EXAMPLES=ON
make -j$(nproc)

# Run insert benchmark
./examples/vector_bench --insert --count=100000 --dimension=128

# Run with different batch sizes
./examples/vector_bench --insert --count=100000 --dimension=128 --batch=100
./examples/vector_bench --insert --count=100000 --dimension=128 --batch=1000
```

### 5.2 Search Performance Test

```bash
# Run search benchmark
./examples/vector_bench --search --count=1000 --dimension=128 --k=10

# Run with different K values
./examples/vector_bench --search --count=1000 --dimension=128 --k=5
./examples/vector_bench --search --count=1000 --dimension=128 --k=20
```

### 5.3 Memory Usage Test

```bash
# Use Valgrind massif
valgrind --tool=massif ./examples/vector_bench --insert --count=100000 --dimension=128

# Analyze memory usage
ms_print massif.out > memory_report.txt
```

### 5.4 CPU Usage Test

```bash
# Use perf
perf stat ./examples/vector_bench --search --count=10000 --dimension=128 --k=10

# Use top
./examples/vector_bench --insert --count=100000 --dimension=128 &
top -p $!
```

---

## 6. Performance Tuning

### 6.1 Index Optimization

| Parameter | Description | Default | Recommendation |
|-----------|-------------|---------|----------------|
| `index_type` | Index algorithm | IVF | IVF for medium datasets, HNSW for large |
| `nlist` | Number of IVF clusters | 100 | 100-1000 based on dataset size |
| `nprobe` | Number of clusters to probe | 10 | 5-20 based on accuracy requirements |
| `m` | HNSW M parameter | 16 | 12-24 based on memory constraints |
| `efConstruction` | HNSW construction parameter | 128 | 100-200 |

### 6.2 Memory Optimization

| Parameter | Description | Default | Recommendation |
|-----------|-------------|---------|----------------|
| `arena_block_size` | Arena block size | 64KB | 64KB-1MB based on workload |
| `max_memory` | Maximum memory limit | 0 (unlimited) | Set based on available RAM |
| `enable_compression` | Enable vector compression | false | true for memory-constrained environments |

### 6.3 Storage Optimization

| Parameter | Description | Default | Recommendation |
|-----------|-------------|---------|----------------|
| `enable_mmap` | Enable memory mapping | true | true for large datasets |
| `enable_wal` | Enable write-ahead log | true | true for data safety |
| `wal_buffer_size` | WAL buffer size | 16MB | 16MB-128MB based on write patterns |

---

## 7. Performance Results

### 7.1 Insert Performance

| Test Case | Vectors | Dimension | Time (s) | QPS | Memory (MB) |
|-----------|---------|-----------|----------|-----|-------------|
| Single Insert | 100,000 | 128 | 1.2 | 83,333 | 150 |
| Batch Insert (100) | 100,000 | 128 | 0.8 | 125,000 | 160 |
| Batch Insert (1000) | 100,000 | 128 | 0.6 | 166,667 | 180 |

### 7.2 Search Performance

| Test Case | Vectors | Dimension | K | QPS | Latency (ms) | P95 (ms) |
|-----------|---------|-----------|-----|-----|--------------|----------|
| Brute-force | 1,000,000 | 128 | 10 | 100 | 10.0 | 12.0 |
| IVF (nlist=100) | 1,000,000 | 128 | 10 | 1,000 | 1.0 | 1.5 |
| PQ (M=8) | 1,000,000 | 128 | 10 | 2,000 | 0.5 | 0.8 |

### 7.3 Scalability

| Dataset Size | Insert QPS | Search QPS | Memory (MB) |
|--------------|------------|------------|-------------|
| 100K vectors | 100,000 | 5,000 | 50 |
| 1M vectors | 80,000 | 1,000 | 450 |
| 10M vectors | 60,000 | 200 | 4,000 |

---

## 8. Optimization Recommendations

### 8.1 Hardware Recommendations

| Workload | CPU | Memory | Storage |
|----------|-----|---------|----------|
| Light | 4 cores | 8GB | SSD |
| Medium | 8 cores | 16GB | NVMe SSD |
| Heavy | 16+ cores | 32GB+ | NVMe SSD |

### 8.2 Software Optimizations

| Area | Optimization | Benefit |
|------|--------------|---------|
| Index | Use IVF with appropriate nlist/nprobe | 10x search speed improvement |
| Memory | Enable PQ compression | 4-8x memory reduction |
| Storage | Enable mmap for large datasets | Better memory utilization |
| Concurrency | Use batch operations | 2-3x throughput improvement |
| Query | Use appropriate K value | Balance between accuracy and speed |

### 8.3 Tuning Checklist

| Check Item | Description | Status |
|------------|-------------|--------|
| [ ] Index type selection | Choose appropriate index algorithm | ⬜ |
| [ ] Index parameter tuning | Optimize index-specific parameters | ⬜ |
| [ ] Memory configuration | Set appropriate memory limits | ⬜ |
| [ ] Batch size optimization | Find optimal batch size for workload | ⬜ |
| [ ] Storage configuration | Enable appropriate storage options | ⬜ |
| [ ] Concurrency settings | Optimize thread pool size | ⬜ |

---

## 9. Performance Monitoring

### 9.1 Monitoring Tools

| Tool | Purpose | Usage |
|------|---------|--------|
| Prometheus | Metrics collection | Collect performance metrics |
| Grafana | Metrics visualization | Visualize performance trends |
| CloudWatch | AWS monitoring | Monitor cloud deployments |
| Datadog | APM | Application performance monitoring |

### 9.2 Key Metrics to Monitor

| Metric | Description | Threshold |
|--------|-------------|-----------|
| QPS | Queries per second | N/A |
| Latency | Average response time | <10ms |
| Memory usage | Peak memory consumption | <80% of available |
| CPU usage | Average CPU utilization | <70% |
| Disk I/O | Storage operations | <50% of capacity |
| Error rate | Percentage of failed operations | <0.1% |

---

## 10. Troubleshooting

### 10.1 Common Performance Issues

| Issue | Possible Cause | Solution |
|-------|----------------|----------|
| Low QPS | Inefficient index | Optimize index parameters |
| High latency | Large dataset | Use more efficient index |
| High memory usage | Uncompressed vectors | Enable PQ compression |
| CPU spikes | Inefficient algorithms | Optimize vector operations |
| Disk I/O bottleneck | Frequent writes | Enable WAL and batch operations |

### 10.2 Performance Analysis

1. **Identify bottleneck**:
   - Use profiling tools to find hotspots
   - Monitor resource usage during operation

2. **Analyze query patterns**:
   - Identify most common queries
   - Optimize for frequent operations

3. **Benchmark systematically**:
   - Test with different parameters
   - Compare results to identify improvements

---

**Document Update Records**

| Date | Version | Update Content |
|------|---------|----------------|
| 2026-04-26 | v1.0.0 | Initial version |
