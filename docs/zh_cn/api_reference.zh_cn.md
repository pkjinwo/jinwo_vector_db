# JinWo VecDB API 参考文档

**版本**：v1.0.0
**生成时间**：2026-04-26
**文档类型**：对外发布

---

## 1. 核心数据结构

### 1.1 数据库结构

```c
/**
 * 向量数据库结构
 */
typedef struct {
    char* db_path;        // 数据库路径
    bool is_open;         // 是否打开
    Arena* arena;         // 内存池
    Collection* collections;  // 集合列表
    size_t collection_count;  // 集合数量
    size_t max_collections;   // 最大集合数
    pthread_mutex_t lock;     // 互斥锁
} jw_vecdb_t;
```

### 1.2 集合结构

```c
/**
 * 向量集合结构
 */
typedef struct {
    char* name;           // 集合名称
    int dimension;        // 向量维度
    size_t vector_count;  // 向量数量
    VectorEntry* vectors; // 向量条目
    Index* index;         // 索引
    Storage* storage;     // 存储
    pthread_mutex_t lock; // 互斥锁
} jw_collection_t;
```

### 1.3 向量条目

```c
/**
 * 向量条目结构
 */
typedef struct {
    uint64_t id;          // 向量ID
    float* vector;        // 向量数据
    size_t dimension;     // 向量维度
    void* metadata;       // 元数据（可选）
    size_t metadata_size; // 元数据大小
} VectorEntry;
```

---

## 2. 错误码定义

| 错误码 | 值 | 描述 |
|-------|-----|------|
| `JW_OK` | 0 | 操作成功 |
| `JW_ERR_INVALID_PARAM` | -1 | 无效参数 |
| `JW_ERR_OUT_OF_MEMORY` | -2 | 内存不足 |
| `JW_ERR_FILE_SYSTEM` | -3 | 文件系统错误 |
| `JW_ERR_COLLECTION_EXISTS` | -4 | 集合已存在 |
| `JW_ERR_COLLECTION_NOT_FOUND` | -5 | 集合不存在 |
| `JW_ERR_VECTOR_NOT_FOUND` | -6 | 向量不存在 |
| `JW_ERR_INDEX_FAILED` | -7 | 索引创建失败 |
| `JW_ERR_DIMENSION_MISMATCH` | -8 | 维度不匹配 |
| `JW_ERR_INTERNAL` | -9 | 内部错误 |

---

## 3. 数据库操作 API

### 3.1 打开数据库

```c
/**
 * 打开或创建数据库
 * @param db 数据库指针指针
 * @param path 数据库路径
 * @param create 如果不存在是否创建
 * @return 错误码
 */
int jw_vecdb_open(jw_vecdb_t** db, const char* path, bool create);
```

**参数**：
- `db`：输出参数，返回数据库指针
- `path`：数据库文件路径
- `create`：如果数据库不存在是否创建

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

### 3.2 关闭数据库

```c
/**
 * 关闭数据库
 * @param db 数据库指针
 * @return 错误码
 */
int jw_vecdb_close(jw_vecdb_t* db);
```

**参数**：
- `db`：数据库指针

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

---

## 4. 集合操作 API

### 4.1 创建集合

```c
/**
 * 创建集合
 * @param collection 集合指针指针
 * @param db 数据库指针
 * @param name 集合名称
 * @param dimension 向量维度
 * @return 错误码
 */
int jw_collection_create(jw_collection_t** collection, jw_vecdb_t* db, const char* name, int dimension);
```

**参数**：
- `collection`：输出参数，返回集合指针
- `db`：数据库指针
- `name`：集合名称
- `dimension`：向量维度

**返回值**：
- `JW_OK`：成功
- `JW_ERR_COLLECTION_EXISTS`：集合已存在
- 其他错误码：失败

### 4.2 打开集合

```c
/**
 * 打开集合
 * @param collection 集合指针指针
 * @param db 数据库指针
 * @param name 集合名称
 * @return 错误码
 */
int jw_collection_open(jw_collection_t** collection, jw_vecdb_t* db, const char* name);
```

**参数**：
- `collection`：输出参数，返回集合指针
- `db`：数据库指针
- `name`：集合名称

**返回值**：
- `JW_OK`：成功
- `JW_ERR_COLLECTION_NOT_FOUND`：集合不存在
- 其他错误码：失败

### 4.3 关闭集合

```c
/**
 * 关闭集合
 * @param collection 集合指针
 * @return 错误码
 */
int jw_collection_close(jw_collection_t* collection);
```

**参数**：
- `collection`：集合指针

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

### 4.4 删除集合

```c
/**
 * 删除集合
 * @param db 数据库指针
 * @param name 集合名称
 * @return 错误码
 */
int jw_collection_drop(jw_vecdb_t* db, const char* name);
```

**参数**：
- `db`：数据库指针
- `name`：集合名称

**返回值**：
- `JW_OK`：成功
- `JW_ERR_COLLECTION_NOT_FOUND`：集合不存在
- 其他错误码：失败

### 4.5 列出集合

```c
/**
 * 列出所有集合
 * @param db 数据库指针
 * @param names 输出参数，返回集合名称列表
 * @param count 输出参数，返回集合数量
 * @return 错误码
 */
int jw_collection_list(jw_vecdb_t* db, char*** names, size_t* count);
```

**参数**：
- `db`：数据库指针
- `names`：输出参数，返回集合名称列表
- `count`：输出参数，返回集合数量

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

**注意**：使用完毕后需要释放 `names` 内存

---

## 5. 向量操作 API

### 5.1 插入向量

```c
/**
 * 插入向量
 * @param collection 集合指针
 * @param vector 向量数据
 * @param id 输出参数，返回向量ID
 * @return 错误码
 */
int jw_collection_insert(jw_collection_t* collection, const float* vector, uint64_t* id);
```

**参数**：
- `collection`：集合指针
- `vector`：向量数据
- `id`：输出参数，返回分配的向量ID

**返回值**：
- `JW_OK`：成功
- `JW_ERR_DIMENSION_MISMATCH`：维度不匹配
- 其他错误码：失败

### 5.2 批量插入向量

```c
/**
 * 批量插入向量
 * @param collection 集合指针
 * @param vectors 向量数据数组
 * @param count 向量数量
 * @param ids 输出参数，返回向量ID数组
 * @return 错误码
 */
int jw_collection_batch_insert(jw_collection_t* collection, const float** vectors, size_t count, uint64_t* ids);
```

**参数**：
- `collection`：集合指针
- `vectors`：向量数据数组
- `count`：向量数量
- `ids`：输出参数，返回分配的向量ID数组

**返回值**：
- `JW_OK`：成功
- `JW_ERR_DIMENSION_MISMATCH`：维度不匹配
- 其他错误码：失败

### 5.3 搜索向量

```c
/**
 * 搜索向量
 * @param collection 集合指针
 * @param query 查询向量
 * @param k 搜索结果数量
 * @param results 输出参数，返回搜索结果
 * @param result_count 输出参数，返回实际结果数量
 * @return 错误码
 */
int jw_collection_search(jw_collection_t* collection, const float* query, size_t k, jw_search_result_t** results, size_t* result_count);
```

**参数**：
- `collection`：集合指针
- `query`：查询向量
- `k`：期望返回的结果数量
- `results`：输出参数，返回搜索结果
- `result_count`：输出参数，返回实际结果数量

**返回值**：
- `JW_OK`：成功
- `JW_ERR_DIMENSION_MISMATCH`：维度不匹配
- 其他错误码：失败

**注意**：使用完毕后需要释放 `results` 内存

### 5.4 删除向量

```c
/**
 * 删除向量
 * @param collection 集合指针
 * @param id 向量ID
 * @return 错误码
 */
int jw_collection_delete(jw_collection_t* collection, uint64_t id);
```

**参数**：
- `collection`：集合指针
- `id`：向量ID

**返回值**：
- `JW_OK`：成功
- `JW_ERR_VECTOR_NOT_FOUND`：向量不存在
- 其他错误码：失败

---

## 6. 索引操作 API

### 6.1 创建索引

```c
/**
 * 创建索引
 * @param collection 集合指针
 * @param index_type 索引类型
 * @param params 索引参数
 * @return 错误码
 */
int jw_collection_create_index(jw_collection_t* collection, const char* index_type, const char* params);
```

**参数**：
- `collection`：集合指针
- `index_type`：索引类型（"brute_force", "ivf", "pq"）
- `params`：索引参数，格式为 "key1=value1,key2=value2"

**返回值**：
- `JW_OK`：成功
- `JW_ERR_INDEX_FAILED`：索引创建失败
- 其他错误码：失败

### 6.2 索引参数

| 索引类型 | 参数 | 默认值 | 说明 |
|---------|------|-------|------|
| brute_force | 无 | - | 暴力搜索 |
| ivf | nlist | 100 | 聚类数量 |
| ivf | nprobe | 10 | 搜索时探测的聚类数 |
| pq | M | 8 | 乘积量化的子空间数 |
| pq | nlist | 100 | 聚类数量 |
| pq | nprobe | 10 | 搜索时探测的聚类数 |

---

## 7. 内存管理 API

### 7.1 分配内存

```c
/**
 * 分配内存
 * @param size 内存大小
 * @return 内存指针
 */
void* jw_malloc(size_t size);
```

**参数**：
- `size`：内存大小

**返回值**：
- 成功：内存指针
- 失败：NULL

### 7.2 释放内存

```c
/**
 * 释放内存
 * @param ptr 内存指针
 */
void jw_free(void* ptr);
```

**参数**：
- `ptr`：内存指针

### 7.3 内存池操作

```c
/**
 * 创建内存池
 * @param block_size 块大小
 * @return 内存池指针
 */
Arena* jw_arena_create(size_t block_size);

/**
 * 从内存池分配内存
 * @param arena 内存池指针
 * @param size 内存大小
 * @return 内存指针
 */
void* jw_arena_alloc(Arena* arena, size_t size);

/**
 * 销毁内存池
 * @param arena 内存池指针
 */
void jw_arena_destroy(Arena* arena);
```

---

## 8. 存储操作 API

### 8.1 保存数据

```c
/**
 * 保存集合数据
 * @param collection 集合指针
 * @return 错误码
 */
int jw_collection_save(jw_collection_t* collection);
```

**参数**：
- `collection`：集合指针

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

### 8.2 加载数据

```c
/**
 * 加载集合数据
 * @param collection 集合指针
 * @return 错误码
 */
int jw_collection_load(jw_collection_t* collection);
```

**参数**：
- `collection`：集合指针

**返回值**：
- `JW_OK`：成功
- 其他错误码：失败

---

## 9. 工具函数 API

### 9.1 距离计算

```c
/**
 * 计算L2距离
 * @param a 向量a
 * @param b 向量b
 * @param dimension 向量维度
 * @return 距离值
 */
float jw_distance_l2(const float* a, const float* b, int dimension);

/**
 * 计算余弦距离
 * @param a 向量a
 * @param b 向量b
 * @param dimension 向量维度
 * @return 距离值
 */
float jw_distance_cosine(const float* a, const float* b, int dimension);

/**
 * 计算点积
 * @param a 向量a
 * @param b 向量b
 * @param dimension 向量维度
 * @return 点积值
 */
float jw_distance_dot(const float* a, const float* b, int dimension);
```

### 9.2 向量操作

```c
/**
 * 向量归一化
 * @param vector 向量
 * @param dimension 向量维度
 */
void jw_vector_normalize(float* vector, int dimension);

/**
 * 向量复制
 * @param dest 目标向量
 * @param src 源向量
 * @param dimension 向量维度
 */
void jw_vector_copy(float* dest, const float* src, int dimension);

/**
 * 向量加法
 * @param result 结果向量
 * @param a 向量a
 * @param b 向量b
 * @param dimension 向量维度
 */
void jw_vector_add(float* result, const float* a, const float* b, int dimension);
```

---

## 10. 并发控制 API

### 10.1 锁操作

```c
/**
 * 加锁
 * @param mutex 互斥锁
 */
void jw_mutex_lock(pthread_mutex_t* mutex);

/**
 * 解锁
 * @param mutex 互斥锁
 */
void jw_mutex_unlock(pthread_mutex_t* mutex);

/**
 * 尝试加锁
 * @param mutex 互斥锁
 * @return 0 成功，其他失败
 */
int jw_mutex_trylock(pthread_mutex_t* mutex);
```

### 10.2 线程池

```c
/**
 * 创建线程池
 * @param size 线程池大小
 * @return 线程池指针
 */
ThreadPool* jw_thread_pool_create(size_t size);

/**
 * 提交任务
 * @param pool 线程池指针
 * @param task 任务函数
 * @param arg 任务参数
 * @return 任务ID
 */
int jw_thread_pool_submit(ThreadPool* pool, void (*task)(void*), void* arg);

/**
 * 销毁线程池
 * @param pool 线程池指针
 */
void jw_thread_pool_destroy(ThreadPool* pool);
```

---

## 11. 配置 API

### 11.1 全局配置

```c
/**
 * 设置全局配置
 * @param key 配置键
 * @param value 配置值
 * @return 错误码
 */
int jw_config_set(const char* key, const char* value);

/**
 * 获取全局配置
 * @param key 配置键
 * @param value 输出参数，返回配置值
 * @return 错误码
 */
int jw_config_get(const char* key, char* value, size_t size);
```

### 11.2 配置项

| 配置项 | 默认值 | 说明 |
|-------|-------|------|
| `max_memory` | 0 | 最大内存限制（0表示无限制） |
| `arena_block_size` | 65536 | 内存池块大小 |
| `enable_compression` | false | 是否启用向量压缩 |
| `enable_wal` | true | 是否启用写前日志 |
| `wal_buffer_size` | 16777216 | WAL缓冲区大小 |
| `enable_mmap` | true | 是否启用内存映射 |

---

## 12. 版本信息 API

### 12.1 获取版本

```c
/**
 * 获取版本号
 * @return 版本字符串
 */
const char* jw_version();

/**
 * 获取版本号组件
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁版本号
 */
void jw_version_components(int* major, int* minor, int* patch);
```

---

## 13. 示例代码

### 13.1 基本用法

```c
#include "jw_vecdb.h"
#include <stdio.h>

int main() {
    // 打开数据库
    jw_vecdb_t* db = NULL;
    int result = jw_vecdb_open(&db, "./vecdb", true);
    if (result != JW_OK) {
        printf("Failed to open database: %d\n", result);
        return 1;
    }
    
    // 创建集合
    jw_collection_t* col = NULL;
    result = jw_collection_create(&col, db, "test", 128);
    if (result != JW_OK) {
        printf("Failed to create collection: %d\n", result);
        jw_vecdb_close(db);
        return 1;
    }
    
    // 插入向量
    float vector[128];
    for (int i = 0; i < 128; i++) {
        vector[i] = (float)i / 128.0f;
    }
    uint64_t id;
    result = jw_collection_insert(col, vector, &id);
    if (result != JW_OK) {
        printf("Failed to insert vector: %d\n", result);
        jw_collection_close(col);
        jw_vecdb_close(db);
        return 1;
    }
    printf("Inserted vector with id: %llu\n", id);
    
    // 搜索向量
    jw_search_result_t* results = NULL;
    size_t result_count;
    result = jw_collection_search(col, vector, 10, &results, &result_count);
    if (result != JW_OK) {
        printf("Failed to search: %d\n", result);
        jw_collection_close(col);
        jw_vecdb_close(db);
        return 1;
    }
    
    printf("Found %zu results:\n", result_count);
    for (size_t i = 0; i < result_count; i++) {
        printf("  %zu: id=%llu, distance=%f\n", i, results[i].id, results[i].distance);
    }
    
    // 清理
    jw_free(results);
    jw_collection_close(col);
    jw_vecdb_close(db);
    
    return 0;
}
```

### 13.2 批量操作

```c
// 批量插入
float* vectors[1000];
uint64_t ids[1000];

for (int i = 0; i < 1000; i++) {
    vectors[i] = (float*)jw_malloc(128 * sizeof(float));
    for (int j = 0; j < 128; j++) {
        vectors[i][j] = (float)(i * 128 + j) / (1000 * 128.0f);
    }
}

result = jw_collection_batch_insert(col, (const float**)vectors, 1000, ids);

// 清理向量内存
for (int i = 0; i < 1000; i++) {
    jw_free(vectors[i]);
}
```

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
