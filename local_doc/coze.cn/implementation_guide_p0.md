# JinWo VecDB 移动端优化 - P0 任务实现指南

> 版本：v1.0  
> 编写日期：2026-04-25  
> 目标读者：Trave  
> 预计工作量：7天

---

## 概述

本文档包含三项 P0 优先级任务的详细实现指南。完成这三项后，JinWo VecDB 将具备在移动端运行的基础能力。

| 任务 | 预计工作量 | 完成标志 |
|-----|-----------|---------|
| SQ 量化实现 | 3天 | 向量内存占用减少 4x |
| 索引序列化 | 2天 | 索引可保存到文件并恢复 |
| MMAP 模式完善 | 2天 | 向量数据按需加载，内存占用降低 |

---

## 任务一：SQ（标量量化）实现

### 1.1 什么是 SQ 量化

标量量化（Scalar Quantization）将 32 位浮点向量压缩为 8 位整数向量：

```
原始：float32[1536] = 1536 × 4 bytes = 6144 bytes
量化：uint8[1536]  = 1536 × 1 byte  = 1536 bytes
压缩比：4x
```

### 1.2 核心原理

每个向量维度独立量化：

```c
// 量化（float -> uint8）
uint8_t quantized = (uint8_t)((value - min) / (max - min) * 255);

// 反量化（uint8 -> float）
float restored = min + quantized / 255.0f * (max - min);
```

精度损失约 0.1%-0.5%，对向量搜索影响可忽略。

### 1.3 需要修改的文件

| 文件 | 操作 |
|-----|------|
| `include/jw_quant.h` | 新建：量化接口定义 |
| `src/jw_quant.c` | 新建：量化实现 |
| `include/jw_index.h` | 修改：添加量化类型字段 |
| `src/jw_index.c` | 修改：集成量化到索引流程 |
| `src/jw_collection.c` | 修改：Collection 支持量化配置 |
| `CMakeLists.txt` | 修改：添加新源文件 |

### 1.4 接口设计

#### jw_quant.h

```c
#ifndef JW_QUANT_H
#define JW_QUANT_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * SQ 量化器结构
 */
typedef struct jw_sq_quantizer {
    jw_float32_t *mins;      /* 每个维度的最小值 [dim] */
    jw_float32_t *maxs;      /* 每个维度的最大值 [dim] */
    jw_float32_t *scales;    /* 每个维度的缩放因子 [dim] */
    jw_dim_t dim;            /* 向量维度 */
} jw_sq_quantizer_t;

/*
 * 创建 SQ 量化器
 * 
 * @param pool    内存池
 * @param dim     向量维度
 * @return 量化器指针，失败返回 NULL
 */
JW_API jw_sq_quantizer_t *jw_sq_quantizer_create(jw_pool_t *pool, jw_dim_t dim);

/*
 * 训练量化器（计算 mins/maxs）
 * 
 * @param quant   量化器
 * @param vectors 训练向量集
 * @param count   向量数量
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_sq_quantizer_train(
    jw_sq_quantizer_t *quant,
    const jw_float32_t *vectors,
    jw_size_t count
);

/*
 * 量化单个向量
 * 
 * @param quant    量化器
 * @param src      源向量（float32）
 * @param dst      目标缓冲区（uint8）
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_sq_quantize(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst
);

/*
 * 反量化单个向量
 * 
 * @param quant    量化器
 * @param src      源数据（uint8）
 * @param dst      目标向量（float32）
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_sq_dequantize(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst
);

/*
 * 批量量化
 */
JW_API jw_status_t jw_sq_quantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst,
    jw_size_t count
);

/*
 * 批量反量化
 */
JW_API jw_status_t jw_sq_dequantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst,
    jw_size_t count
);

/*
 * 计算量化向量与浮点向量的距离（无需反量化）
 * 
 * @param quant    量化器
 * @param qvec     量化向量（uint8）
 * @param fvec     浮点查询向量
 * @param metric   距离度量类型
 * @return 距离值
 */
JW_API jw_float32_t jw_sq_distance(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *qvec,
    const jw_float32_t *fvec,
    jw_metric_type_t metric
);

/*
 * 销毁量化器（由内存池管理，通常不需要显式调用）
 */
JW_API void jw_sq_quantizer_destroy(jw_sq_quantizer_t *quant);

JW_END_DECL

#endif /* JW_QUANT_H */
```

### 1.5 实现步骤

#### Step 1：创建量化器框架

```c
// src/jw_quant.c

#include "jw_quant.h"
#include <string.h>
#include <math.h>

JW_API jw_sq_quantizer_t *jw_sq_quantizer_create(jw_pool_t *pool, jw_dim_t dim)
{
    if (pool == NULL || dim == 0) {
        return NULL;
    }
    
    jw_sq_quantizer_t *quant = jw_pool_alloc(pool, sizeof(jw_sq_quantizer_t));
    if (quant == NULL) {
        return NULL;
    }
    
    quant->dim = dim;
    quant->mins = jw_pool_alloc(pool, dim * sizeof(jw_float32_t));
    quant->maxs = jw_pool_alloc(pool, dim * sizeof(jw_float32_t));
    quant->scales = jw_pool_alloc(pool, dim * sizeof(jw_float32_t));
    
    if (quant->mins == NULL || quant->maxs == NULL || quant->scales == NULL) {
        return NULL;
    }
    
    return quant;
}

JW_API jw_status_t jw_sq_quantizer_train(
    jw_sq_quantizer_t *quant,
    const jw_float32_t *vectors,
    jw_size_t count)
{
    if (quant == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    // 初始化：第一个向量作为初始值
    for (jw_dim_t d = 0; d < dim; d++) {
        quant->mins[d] = vectors[d];
        quant->maxs[d] = vectors[d];
    }
    
    // 遍历所有向量，更新 min/max
    for (jw_size_t i = 1; i < count; i++) {
        const jw_float32_t *vec = vectors + i * dim;
        for (jw_dim_t d = 0; d < dim; d++) {
            if (vec[d] < quant->mins[d]) quant->mins[d] = vec[d];
            if (vec[d] > quant->maxs[d]) quant->maxs[d] = vec[d];
        }
    }
    
    // 计算缩放因子，避免除零
    for (jw_dim_t d = 0; d < dim; d++) {
        jw_float32_t range = quant->maxs[d] - quant->mins[d];
        quant->scales[d] = (range > 1e-6f) ? (255.0f / range) : 1.0f;
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_sq_quantize(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst)
{
    if (quant == NULL || src == NULL || dst == NULL) {
        return JW_INVALID_PARAM;
    }
    
    for (jw_dim_t d = 0; d < quant->dim; d++) {
        jw_float32_t normalized = (src[d] - quant->mins[d]) * quant->scales[d];
        // 限制范围 [0, 255]
        normalized = fmaxf(0.0f, fminf(255.0f, normalized));
        dst[d] = (jw_uint8_t)(normalized + 0.5f);  // 四舍五入
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_sq_dequantize(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst)
{
    if (quant == NULL || src == NULL || dst == NULL) {
        return JW_INVALID_PARAM;
    }
    
    for (jw_dim_t d = 0; d < quant->dim; d++) {
        dst[d] = quant->mins[d] + src[d] / quant->scales[d];
    }
    
    return JW_SUCCESS;
}
```

#### Step 2：实现距离计算（关键优化）

```c
JW_API jw_float32_t jw_sq_distance(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *qvec,
    const jw_float32_t *fvec,
    jw_metric_type_t metric)
{
    if (quant == NULL || qvec == NULL || fvec == NULL) {
        return INFINITY;
    }
    
    jw_float32_t dist = 0.0f;
    jw_dim_t dim = quant->dim;
    
    if (metric == JW_METRIC_L2) {
        // L2 距离：sum((fvec - dequantize(qvec))^2)
        // 优化：预计算 mins + qvec/scales，避免反量化
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            jw_float32_t diff = fvec[d] - restored;
            dist += diff * diff;
        }
        return sqrtf(dist);
    }
    else if (metric == JW_METRIC_IP) {
        // 内积：sum(fvec * dequantize(qvec))
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            dist += fvec[d] * restored;
        }
        return -dist;  // 负号使得最大内积对应最小距离
    }
    else if (metric == JW_METRIC_COSINE) {
        // 余弦相似度：需要归一化
        jw_float32_t dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            dot += fvec[d] * restored;
            norm_a += fvec[d] * fvec[d];
            norm_b += restored * restored;
        }
        return 1.0f - dot / (sqrtf(norm_a) * sqrtf(norm_b));
    }
    
    return INFINITY;
}
```

#### Step 3：集成到索引

修改 `jw_index.h`，添加量化器字段：

```c
typedef struct jw_index {
    jw_index_type_t type;
    jw_metric_type_t metric;
    jw_dim_t dim;
    
    /* 新增：量化器 */
    jw_sq_quantizer_t *sq_quantizer;
    jw_bool_t use_quantization;
    
    /* 量化后的向量存储 */
    jw_uint8_t *quantized_vectors;  /* 所有量化向量的连续存储 */
    
    /* ... 其他字段 ... */
} jw_index_t;
```

修改 `jw_collection.c`，在插入向量时使用量化：

```c
JW_API jw_status_t jw_collection_insert(
    jw_collection_t *coll,
    const jw_float32_t *vec,
    jw_vid_t *vid)
{
    // ... 原有逻辑 ...
    
    if (coll->config.use_quantization && coll->index->sq_quantizer != NULL) {
        // 量化后存储
        jw_uint8_t *qvec = jw_pool_alloc(coll->pool, coll->dim);
        jw_sq_quantize(coll->index->sq_quantizer, vec, qvec);
        // 存储到索引
    } else {
        // 原始浮点存储
    }
    
    // ... 后续逻辑 ...
}
```

### 1.6 测试验证

创建测试文件 `tests/test_quant.c`：

```c
/* 测试 SQ 量化 */
void test_sq_quantization(void)
{
    jw_pool_t *pool = jw_pool_create(1024 * 1024);
    jw_dim_t dim = 128;
    jw_size_t count = 1000;
    
    /* 生成测试向量 */
    jw_float32_t *vectors = malloc(count * dim * sizeof(jw_float32_t));
    for (jw_size_t i = 0; i < count * dim; i++) {
        vectors[i] = (jw_float32_t)rand() / RAND_MAX * 2.0f - 1.0f;
    }
    
    /* 创建并训练量化器 */
    jw_sq_quantizer_t *quant = jw_sq_quantizer_create(pool, dim);
    jw_status_t status = jw_sq_quantizer_train(quant, vectors, count);
    assert(status == JW_SUCCESS);
    
    /* 测试量化/反量化精度 */
    jw_float32_t error_sum = 0.0f;
    for (jw_size_t i = 0; i < 10; i++) {
        jw_uint8_t qvec[128];
        jw_float32_t restored[128];
        
        jw_sq_quantize(quant, &vectors[i * dim], qvec);
        jw_sq_dequantize(quant, qvec, restored);
        
        /* 计算误差 */
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t err = fabsf(vectors[i * dim + d] - restored[d]);
            error_sum += err;
        }
    }
    
    jw_float32_t avg_error = error_sum / (10 * dim);
    printf("Average quantization error: %.6f\n", avg_error);
    assert(avg_error < 0.01f);  /* 平均误差应小于 1% */
    
    /* 测试距离计算 */
    jw_float32_t dist_quant = jw_sq_distance(quant, qvec, &vectors[0], JW_METRIC_L2);
    jw_float32_t dist_orig = jw_vec_l2_distance(&vectors[0], restored, dim);
    printf("Distance comparison: quant=%.4f, orig=%.4f, diff=%.4f\n",
           dist_quant, dist_orig, fabsf(dist_quant - dist_orig));
    
    free(vectors);
    jw_pool_destroy(pool);
}
```

### 1.7 注意事项

1. **训练数据量**：训练量化器至少需要 1000 个向量，数据越多 mins/maxs 越准确
2. **维度顺序**：确保 mins/maxs/scales 数组的维度顺序与向量维度一致
3. **边界处理**：量化时用 `fmaxf/fminf` 限制范围，避免越界
4. **内存对齐**：如果追求 SIMD 性能，qvec 数组建议 16 字节对齐

---

## 任务二：索引序列化/反序列化

### 2.1 目标

实现索引的保存和加载，避免每次启动时重建索引。

### 2.2 需要修改的文件

| 文件 | 操作 |
|-----|------|
| `include/jw_index.h` | 修改：完善 save/load 接口声明 |
| `src/jw_index.c` | 修改：实现序列化逻辑 |
| `include/jw_storage.h` | 修改：添加索引文件格式定义 |

### 2.3 文件格式设计

```
索引文件格式（小端序）：

+------------------+
| File Header      |  64 bytes
+------------------+
| Index Config     |  128 bytes
+------------------+
| Quantizer Data   |  (if SQ enabled)
|   - mins[dim]    |
|   - maxs[dim]    |
|   - scales[dim]  |
+------------------+
| IVF Data         |  (if IVF index)
|   - centroids    |
|   - inverted lists|
+------------------+
| HNSW Data        |  (if HNSW index)
|   - levels       |
|   - adjacency    |
+------------------+
| Vector Data      |
|   - raw or quantized
+------------------+
```

### 2.4 文件头定义

添加到 `jw_storage.h`：

```c
/*
 * 索引文件头（固定大小，64 bytes）
 */
#pragma pack(push, 1)
typedef struct jw_index_file_header {
    jw_uint32_t magic;              /* 魔数：0x4A574958 ("JWIX") */
    jw_uint32_t version;            /* 格式版本 */
    jw_uint32_t index_type;         /* 索引类型 */
    jw_uint32_t metric_type;        /* 距离度量类型 */
    jw_uint32_t dim;                /* 向量维度 */
    jw_uint32_t ntotal;             /* 向量总数 */
    jw_uint32_t flags;              /* 标志位 */
    jw_uint32_t reserved[7];        /* 保留字段 */
} jw_index_file_header_t;
#pragma pack(pop)

#define JW_INDEX_FILE_MAGIC    0x4A574958
#define JW_INDEX_FILE_VERSION  0x00010000

/* 标志位定义 */
#define JW_INDEX_FLAG_SQ_QUANTIZED   0x00000001
#define JW_INDEX_FLAG_PQ_QUANTIZED   0x00000002
#define JW_INDEX_FLAG_HAS_METADATA   0x00000004
```

### 2.5 实现框架

```c
// src/jw_index.c

JW_API jw_status_t jw_index_save(const jw_index_t *index, const char *filename)
{
    if (index == NULL || filename == NULL) {
        return JW_INVALID_PARAM;
    }
    
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        return JW_FILE_NOT_FOUND;
    }
    
    jw_status_t status = JW_SUCCESS;
    
    /* 1. 写文件头 */
    jw_index_file_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = jw_htole32(JW_INDEX_FILE_MAGIC);
    header.version = jw_htole32(JW_INDEX_FILE_VERSION);
    header.index_type = jw_htole32(index->type);
    header.metric_type = jw_htole32(index->metric);
    header.dim = jw_htole32(index->dim);
    header.ntotal = jw_htole32((jw_uint32_t)index->ntotal);
    
    if (index->sq_quantizer != NULL) {
        header.flags |= JW_INDEX_FLAG_SQ_QUANTIZED;
    }
    
    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        status = JW_IO_ERROR;
        goto cleanup;
    }
    
    /* 2. 写量化器数据（如果有） */
    if (index->sq_quantizer != NULL) {
        if (fwrite(index->sq_quantizer->mins, sizeof(jw_float32_t), index->dim, fp) != index->dim ||
            fwrite(index->sq_quantizer->maxs, sizeof(jw_float32_t), index->dim, fp) != index->dim ||
            fwrite(index->sq_quantizer->scales, sizeof(jw_float32_t), index->dim, fp) != index->dim) {
            status = JW_IO_ERROR;
            goto cleanup;
        }
    }
    
    /* 3. 根据索引类型写具体数据 */
    switch (index->type) {
        case JW_INDEX_IVF:
            status = jw_ivf_save(index, fp);
            break;
        case JW_INDEX_HNSW:
            status = jw_hnsw_save(index, fp);
            break;
        case JW_INDEX_NONE:
            status = jw_flat_save(index, fp);
            break;
        default:
            status = JW_NOT_SUPPORTED;
    }
    
cleanup:
    fclose(fp);
    return status;
}

JW_API jw_index_t *jw_index_load(jw_pool_t *pool, const char *filename)
{
    if (pool == NULL || filename == NULL) {
        return NULL;
    }
    
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }
    
    /* 1. 读文件头 */
    jw_index_file_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }
    
    /* 验证魔数和版本 */
    if (jw_le32toh(header.magic) != JW_INDEX_FILE_MAGIC) {
        fclose(fp);
        return NULL;
    }
    
    /* 2. 创建索引结构 */
    jw_index_config_t config;
    memset(&config, 0, sizeof(config));
    config.type = jw_le32toh(header.index_type);
    config.metric = jw_le32toh(header.metric_type);
    config.dim = jw_le32toh(header.dim);
    
    jw_index_t *index = jw_index_create(pool, &config);
    if (index == NULL) {
        fclose(fp);
        return NULL;
    }
    
    index->ntotal = jw_le32toh(header.ntotal);
    
    /* 3. 读量化器数据（如果有） */
    if (header.flags & JW_INDEX_FLAG_SQ_QUANTIZED) {
        index->sq_quantizer = jw_sq_quantizer_create(pool, config.dim);
        if (fread(index->sq_quantizer->mins, sizeof(jw_float32_t), config.dim, fp) != config.dim ||
            fread(index->sq_quantizer->maxs, sizeof(jw_float32_t), config.dim, fp) != config.dim ||
            fread(index->sq_quantizer->scales, sizeof(jw_float32_t), config.dim, fp) != config.dim) {
            fclose(fp);
            return NULL;
        }
    }
    
    /* 4. 根据索引类型读具体数据 */
    jw_status_t status = JW_SUCCESS;
    switch (index->type) {
        case JW_INDEX_IVF:
            status = jw_ivf_load(index, fp);
            break;
        case JW_INDEX_HNSW:
            status = jw_hnsw_load(index, fp);
            break;
        case JW_INDEX_NONE:
            status = jw_flat_load(index, fp);
            break;
    }
    
    fclose(fp);
    
    if (status != JW_SUCCESS) {
        return NULL;
    }
    
    return index;
}
```

### 2.6 HNSW 序列化示例

```c
static jw_status_t jw_hnsw_save(const jw_index_t *index, FILE *fp)
{
    jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index;
    
    /* 写 HNSW 配置 */
    jw_uint32_t config[5];
    config[0] = jw_htole32(hnsw->M);
    config[1] = jw_htole32(hnsw->max_level);
    config[2] = jw_htole32(hnsw->ef_construction);
    config[3] = jw_htole32(hnsw->ef_search);
    config[4] = jw_htole32(hnsw->entry_node);
    
    if (fwrite(config, sizeof(jw_uint32_t), 5, fp) != 5) {
        return JW_IO_ERROR;
    }
    
    /* 写每个节点的层级 */
    for (jw_size_t i = 0; i < index->ntotal; i++) {
        jw_uint8_t level = hnsw->nodes[i].level;
        if (fwrite(&level, 1, 1, fp) != 1) {
            return JW_IO_ERROR;
        }
    }
    
    /* 写邻接表 */
    for (jw_size_t i = 0; i < index->ntotal; i++) {
        for (jw_uint32_t l = 0; l <= hnsw->nodes[i].level; l++) {
            jw_uint32_t neighbor_count = hnsw->nodes[i].neighbors[l].count;
            if (fwrite(&neighbor_count, sizeof(jw_uint32_t), 1, fp) != 1) {
                return JW_IO_ERROR;
            }
            if (fwrite(hnsw->nodes[i].neighbors[l].vids, 
                       sizeof(jw_vid_t), neighbor_count, fp) != neighbor_count) {
                return JW_IO_ERROR;
            }
        }
    }
    
    /* 写向量数据 */
    if (index->sq_quantizer != NULL) {
        fwrite(index->quantized_vectors, index->dim, index->ntotal, fp);
    } else {
        fwrite(index->vectors, sizeof(jw_float32_t), index->dim * index->ntotal, fp);
    }
    
    return JW_SUCCESS;
}
```

### 2.7 测试验证

```c
void test_index_serialization(void)
{
    /* 创建并填充索引 */
    jw_pool_t *pool = jw_pool_create(10 * 1024 * 1024);
    jw_index_config_t config = {
        .type = JW_INDEX_HNSW,
        .metric = JW_METRIC_L2,
        .dim = 128,
        .params.hnsw = { .M = 16, .ef_construction = 100 }
    };
    
    jw_index_t *index = jw_index_create(pool, &config);
    
    /* 插入测试向量 */
    jw_float32_t vec[128];
    for (int i = 0; i < 1000; i++) {
        for (int d = 0; d < 128; d++) {
            vec[d] = (jw_float32_t)rand() / RAND_MAX;
        }
        jw_index_add(index, vec);
    }
    
    /* 保存 */
    jw_status_t status = jw_index_save(index, "test_index.bin");
    assert(status == JW_SUCCESS);
    
    /* 加载 */
    jw_pool_t *pool2 = jw_pool_create(10 * 1024 * 1024);
    jw_index_t *loaded = jw_index_load(pool2, "test_index.bin");
    assert(loaded != NULL);
    assert(loaded->ntotal == 1000);
    assert(loaded->dim == 128);
    
    /* 验证搜索结果一致 */
    jw_search_result_t results1[10], results2[10];
    jw_index_search(index, vec, 10, results1);
    jw_index_search(loaded, vec, 10, results2);
    
    for (int i = 0; i < 10; i++) {
        assert(results1[i].vid == results2[i].vid);
    }
    
    printf("Serialization test passed!\n");
}
```

### 2.8 注意事项

1. **字节序**：所有多字节字段使用 `jw_htole32/jw_le32toh` 转换
2. **版本兼容**：保存时写版本号，加载时检查兼容性
3. **文件校验**：考虑在文件末尾加 CRC32 校验
4. **增量更新**：当前设计是全量保存，增量更新可在后续版本实现

---

## 任务三：MMAP 模式完善

### 3.1 目标

实现向量数据的内存映射，让大数据集在移动端也能运行。

### 3.2 原理对比

| 模式 | 内存占用 | 启动速度 | 适用场景 |
|-----|---------|---------|---------|
| 内存模式 | 全部向量 | 快（已加载） | 小数据集 |
| MMAP 模式 | 仅索引结构 | 快（延迟加载） | 大数据集 |
| 磁盘模式 | 最小 | 慢 | 超大数据集 |

### 3.3 需要修改的文件

| 文件 | 操作 |
|-----|------|
| `include/jw_storage.h` | 修改：完善 MMAP 接口 |
| `src/jw_storage.c` | 修改：实现 MMAP 逻辑 |
| `include/jw_collection.h` | 修改：添加 MMAP 配置选项 |

### 3.4 MMAP 接口设计

```c
/* 添加到 jw_storage.h */

/*
 * MMAP 存储句柄
 */
typedef struct jw_mmap_handle {
    void *base;              /* 映射基地址 */
    jw_size_t size;          /* 映射大小 */
    int fd;                  /* 文件描述符 */
#ifdef JW_WIN32
    HANDLE hFile;            /* Windows 文件句柄 */
    HANDLE hMap;             /* Windows 映射句柄 */
#endif
} jw_mmap_handle_t;

/*
 * 创建 MMAP 存储
 * 
 * @param path     文件路径
 * @param size     映射大小（0 表示自动获取文件大小）
 * @param readonly 是否只读
 * @return MMAP 句柄，失败返回 NULL
 */
JW_API jw_mmap_handle_t *jw_mmap_create(
    const char *path,
    jw_size_t size,
    jw_bool_t readonly
);

/*
 * 获取映射指针
 */
JW_API void *jw_mmap_ptr(const jw_mmap_handle_t *handle);

/*
 * 获取映射大小
 */
JW_API jw_size_t jw_mmap_size(const jw_mmap_handle_t *handle);

/*
 * 刷新到磁盘
 */
JW_API jw_status_t jw_mmap_flush(jw_mmap_handle_t *handle);

/*
 * 销毁 MMAP
 */
JW_API void jw_mmap_destroy(jw_mmap_handle_t *handle);

/*
 * 建议内核预读
 * 
 * @param handle   MMAP 句柄
 * @param offset   偏移量
 * @param len      长度
 * @param will_need 预期会访问
 */
JW_API jw_status_t jw_mmap_advise(
    jw_mmap_handle_t *handle,
    jw_size_t offset,
    jw_size_t len,
    jw_bool_t will_need
);
```

### 3.5 MMAP 实现

```c
// src/jw_storage.c

#ifdef JW_WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

JW_API jw_mmap_handle_t *jw_mmap_create(
    const char *path,
    jw_size_t size,
    jw_bool_t readonly)
{
    if (path == NULL) {
        return NULL;
    }
    
    jw_mmap_handle_t *handle = malloc(sizeof(jw_mmap_handle_t));
    if (handle == NULL) {
        return NULL;
    }
    memset(handle, 0, sizeof(jw_mmap_handle_t));
    
#ifdef JW_WIN32
    /* Windows 实现 */
    DWORD access = readonly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
    DWORD create = OPEN_EXISTING;
    
    handle->hFile = CreateFileA(path, access, FILE_SHARE_READ, NULL, 
                                create, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle->hFile == INVALID_HANDLE_VALUE) {
        free(handle);
        return NULL;
    }
    
    /* 获取文件大小 */
    if (size == 0) {
        LARGE_INTEGER fileSize;
        GetFileSizeEx(handle->hFile, &fileSize);
        size = (jw_size_t)fileSize.QuadPart;
    }
    
    /* 创建映射 */
    DWORD protect = readonly ? PAGE_READONLY : PAGE_READWRITE;
    handle->hMap = CreateFileMapping(handle->hFile, NULL, protect, 0, size, NULL);
    if (handle->hMap == NULL) {
        CloseHandle(handle->hFile);
        free(handle);
        return NULL;
    }
    
    /* 映射到内存 */
    DWORD mapAccess = readonly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
    handle->base = MapViewOfFile(handle->hMap, mapAccess, 0, 0, size);
    if (handle->base == NULL) {
        CloseHandle(handle->hMap);
        CloseHandle(handle->hFile);
        free(handle);
        return NULL;
    }
    
#else
    /* POSIX 实现 */
    int flags = readonly ? O_RDONLY : O_RDWR;
    handle->fd = open(path, flags);
    if (handle->fd < 0) {
        free(handle);
        return NULL;
    }
    
    /* 获取文件大小 */
    if (size == 0) {
        struct stat st;
        fstat(handle->fd, &st);
        size = (jw_size_t)st.st_size;
    }
    
    /* 映射 */
    int prot = readonly ? PROT_READ : (PROT_READ | PROT_WRITE);
    handle->base = mmap(NULL, size, prot, MAP_SHARED, handle->fd, 0);
    if (handle->base == MAP_FAILED) {
        close(handle->fd);
        free(handle);
        return NULL;
    }
    
#endif
    
    handle->size = size;
    return handle;
}

JW_API void *jw_mmap_ptr(const jw_mmap_handle_t *handle)
{
    return handle ? handle->base : NULL;
}

JW_API jw_size_t jw_mmap_size(const jw_mmap_handle_t *handle)
{
    return handle ? handle->size : 0;
}

JW_API jw_status_t jw_mmap_flush(jw_mmap_handle_t *handle)
{
    if (handle == NULL || handle->base == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    if (!FlushViewOfFile(handle->base, handle->size)) {
        return JW_IO_ERROR;
    }
#else
    if (msync(handle->base, handle->size, MS_SYNC) != 0) {
        return JW_IO_ERROR;
    }
#endif
    
    return JW_SUCCESS;
}

JW_API void jw_mmap_destroy(jw_mmap_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }
    
    if (handle->base != NULL) {
#ifdef JW_WIN32
        UnmapViewOfFile(handle->base);
        CloseHandle(handle->hMap);
        CloseHandle(handle->hFile);
#else
        munmap(handle->base, handle->size);
        close(handle->fd);
#endif
    }
    
    free(handle);
}

JW_API jw_status_t jw_mmap_advise(
    jw_mmap_handle_t *handle,
    jw_size_t offset,
    jw_size_t len,
    jw_bool_t will_need)
{
#ifndef JW_WIN32
    int advice = will_need ? MADV_WILLNEED : MADV_DONTNEED;
    if (madvise((char *)handle->base + offset, len, advice) != 0) {
        return JW_IO_ERROR;
    }
#endif
    return JW_SUCCESS;
}
```

### 3.6 集成到 Collection

修改 `jw_collection.h`：

```c
typedef struct jw_collection_config {
    /* ... 原有字段 ... */
    
    /* 新增：MMAP 配置 */
    jw_bool_t use_mmap;              /* 是否使用 MMAP */
    const char *mmap_path;           /* MMAP 文件路径 */
    jw_bool_t mmap_readonly;         /* 是否只读 */
} jw_collection_config_t;
```

修改 `jw_collection.c`：

```c
typedef struct jw_collection {
    /* ... 原有字段 ... */
    
    /* MMAP 相关 */
    jw_mmap_handle_t *vector_mmap;   /* 向量数据 MMAP */
    jw_uint8_t *vector_base;         /* 向量基地址 */
    jw_size_t vector_file_size;      /* 文件大小 */
} jw_collection_t;

/* 打开 Collection 时初始化 MMAP */
JW_API jw_status_t jw_collection_open_mmap(jw_collection_t *coll)
{
    if (!coll->config.use_mmap) {
        return JW_SUCCESS;
    }
    
    coll->vector_mmap = jw_mmap_create(
        coll->config.mmap_path,
        0,  /* 自动获取大小 */
        coll->config.mmap_readonly
    );
    
    if (coll->vector_mmap == NULL) {
        return JW_FILE_NOT_FOUND;
    }
    
    coll->vector_base = jw_mmap_ptr(coll->vector_mmap);
    coll->vector_file_size = jw_mmap_size(coll->vector_mmap);
    
    return JW_SUCCESS;
}

/* 获取向量时从 MMAP 读取 */
JW_API jw_status_t jw_collection_get(
    jw_collection_t *coll,
    jw_vid_t vid,
    jw_record_t *record)
{
    if (coll->vector_mmap != NULL) {
        /* MMAP 模式：直接返回指针，无需拷贝 */
        jw_size_t offset = vid * coll->dim * sizeof(jw_float32_t);
        record->vec = (jw_float32_t *)(coll->vector_base + offset);
    } else {
        /* 内存模式：原有逻辑 */
    }
    
    return JW_SUCCESS;
}
```

### 3.7 测试验证

```c
void test_mmap_mode(void)
{
    /* 准备测试数据文件 */
    const char *test_file = "test_vectors.bin";
    jw_dim_t dim = 128;
    jw_size_t count = 100000;  /* 10万向量 */
    
    /* 生成测试文件 */
    FILE *fp = fopen(test_file, "wb");
    for (jw_size_t i = 0; i < count * dim; i++) {
        jw_float32_t v = (jw_float32_t)rand() / RAND_MAX;
        fwrite(&v, sizeof(jw_float32_t), 1, fp);
    }
    fclose(fp);
    
    /* 使用 MMAP 模式打开 */
    jw_mmap_handle_t *mmap = jw_mmap_create(test_file, 0, JW_TRUE);
    assert(mmap != NULL);
    
    /* 验证数据 */
    jw_float32_t *vecs = jw_mmap_ptr(mmap);
    printf("First vector[0]: %.4f\n", vecs[0]);
    
    /* 检查内存占用 */
    printf("MMAP size: %.2f MB\n", jw_mmap_size(mmap) / 1024.0 / 1024.0);
    printf("Actual memory: ~0 MB (lazy loaded by OS)\n");
    
    jw_mmap_destroy(mmap);
}
```

### 3.8 注意事项

1. **只读映射**：移动端建议使用只读映射，避免数据损坏
2. **页面错误**：首次访问向量时会触发页面错误，可在后台预热
3. **文件对齐**：向量数据最好按页大小（4KB）对齐
4. **Android 限制**：Android 上 MMAP 文件大小有限制（约 1GB）

---

## 验收标准

完成三项任务后，应达到以下标准：

| 功能 | 验收标准 |
|-----|---------|
| SQ 量化 | 内存占用减少 4x，平均误差 < 1%，搜索召回率 > 95% |
| 索引序列化 | 索引可保存/加载，加载后搜索结果一致 |
| MMAP 模式 | 100万向量内存占用 < 100MB，首次查询延迟 < 100ms |

---

## 后续任务（P1）

完成 P0 后，继续以下任务：

1. **NEON 距离计算**：ARM SIMD 优化
2. **平台配置预设**：`jw_preset_mobile_low()` 等
3. **内存监控 API**：`jw_collection_memory_used()`

---

如有问题，请随时沟通。
