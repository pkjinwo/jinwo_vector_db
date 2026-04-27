# JinWo VecDB 架构设计文档

**版本**: v1.0.0
**生成时间**: 2026-04-26
**文档类型**: 对外发布

---

## 一、架构概述

### 1.1 项目简介

JinWo VecDB 是一款专为嵌入式系统和移动端设计的轻量级向量数据库，支持高效的向量存储、索引和检索。项目采用纯C语言开发，具有以下特点：

- **轻量级**: 最小依赖，适合嵌入式和移动端部署
- **高性能**: 支持SIMD加速，提供高效的向量操作
- **跨平台**: 支持Linux、Android、iOS、Windows等主流平台
- **易集成**: 提供简洁的C API和多种语言绑定

### 1.2 核心架构

JinWo VecDB 采用分层架构设计，主要分为以下层次：

| 层次 | 描述 | 主要模块 |
|------|------|----------|
| API 层 | 对外接口层 | jw_vecdb.h |
| 业务逻辑层 | 核心业务处理 | Collection, Index, Vector |
| 存储层 | 数据持久化 | Storage, File |
| 基础设施层 | 基础工具 | Arena, Lock, Hash |

### 1.3 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        API 层                               │
│  jw_vecdb_open/close, jw_collection_*, jw_vector_*          │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     业务逻辑层                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  Collection │  │   Index     │  │   Vector    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                       存储层                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  Storage    │  │    File     │  │   Config    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     基础设施层                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │  Arena  │ │  Lock   │ │  Hash   │ │  Math   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
└─────────────────────────────────────────────────────────────┘
```

---

## 二、核心模块设计

### 2.1 VecDB 主模块

**文件**: `src/jw_vecdb.c`, `include/jw_vecdb.h`

**功能**:
- 数据库实例管理
- 全局配置管理
- 集合管理入口

**关键数据结构**:

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

**API 接口**:

| 函数 | 描述 |
|------|------|
| `jw_vecdb_open` | 打开数据库 |
| `jw_vecdb_close` | 关闭数据库 |
| `jw_vecdb_create_collection` | 创建集合 |
| `jw_vecdb_drop_collection` | 删除集合 |
| `jw_vecdb_list_collections` | 列举集合 |

### 2.2 Collection 模块

**文件**: `src/jw_collection.c`, `include/jw_collection.h`

**功能**:
- 集合管理
- 向量索引管理
- 集合级配置

**关键数据结构**:

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

**API 接口**:

| 函数 | 描述 |
|------|------|
| `jw_collection_create` | 创建集合 |
| `jw_collection_open` | 打开集合 |
| `jw_collection_close` | 关闭集合 |
| `jw_collection_insert` | 插入向量 |
| `jw_collection_search` | 搜索向量 |

### 2.3 Index 模块

**文件**: `src/jw_index.c`, `include/jw_index.h`

**功能**:
- 向量索引构建
- 相似度搜索
- 索引优化

**索引算法**:

| 算法 | 描述 | 适用场景 |
|------|------|----------|
| 暴力搜索 | 线性扫描所有向量 | 小规模数据集 |
| IVF | 倒排文件索引 | 中等规模数据集 |
| PQ | 产品量化 | 大规模数据集，内存受限 |

**关键数据结构**:

```c
typedef struct {
    IndexType type;
    size_t dimension;
    size_t vector_count;
    void* index_data;
    Quantizer* quantizer;
} Index;
```

### 2.4 Vector 模块

**文件**: `src/jw_vector.c`, `include/jw_vector.h`

**功能**:
- 向量数据管理
- 向量运算
- 距离计算

**关键数据结构**:

```c
typedef struct {
    uint64_t id;
    float* data;
    size_t dimension;
    uint32_t flags;
} Vector;
```

**距离度量**:

| 度量 | 公式 | 适用场景 |
|------|------|----------|
| L2 | √(Σ(a-b)²) | 图像、音频特征 |
| 余弦 | a·b/(|a||b|) | 文本嵌入 |
| 点积 | Σab | 推荐系统 |

### 2.5 Storage 模块

**文件**: `src/jw_storage.c`, `include/jw_storage.h`

**功能**:
- 数据持久化
- 内存映射
- 数据恢复

**存储格式**:

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

### 2.6 Arena 内存池模块

**文件**: `src/jw_arena.c`, `include/jw_arena.h`

**功能**:
- 内存分配管理
- 内存池实现
- 资源追踪

**关键特性**:

| 特性 | 描述 |
|------|------|
| 线性分配 | 简单高效的内存分配 |
| 批量释放 | 一次性释放所有内存 |
| 内存追踪 | 记录内存使用情况 |

---

## 三、数据流设计

### 3.1 向量插入流程

```
用户请求
    │
    ▼
jw_collection_insert()
    │
    ▼
参数验证 ─────────────────────┐
    │                         │
    ▼                         ▼
Arena 分配内存            返回错误
    │
    ▼
Vector 创建
    │
    ▼
Index 更新 ────► 异步索引构建
    │
    ▼
Storage 持久化 ────► 后台写回
    │
    ▼
返回向量ID
```

### 3.2 向量搜索流程

```
用户请求
    │
    ▼
jw_collection_search()
    │
    ▼
参数验证 ─────────────────────┐
    │                         │
    ▼                         ▼
Index 查询                返回错误
    │
    ▼
距离计算 (可选SIMD加速)
    │
    ▼
结果排序
    │
    ▼
返回Top-K结果
```

---

## 四、内存管理设计

### 4.1 内存架构

| 组件 | 内存类型 | 说明 |
|------|----------|------|
| Arena | 堆内存 | 主要内存池 |
| Vector数据 | 堆内存 | 向量数据存储 |
| Index结构 | 堆内存 | 索引数据结构 |
| MMAP区域 | 虚拟内存 | 内存映射文件 |

### 4.2 内存分配策略

| 操作 | 分配策略 | 释放策略 |
|------|----------|----------|
| 小对象 (<1KB) | Arena线性分配 | Arena统一释放 |
| 中对象 (1KB-1MB) | 单独malloc | 单独free |
| 大对象 (>1MB) | mmap | munmap |

### 4.3 内存安全机制

| 机制 | 描述 | 实现 |
|------|------|------|
| 边界检查 | 数组边界验证 | assert/if检查 |
| 内存初始化 | 分配时清零 | memset |
| 双重释放检测 | 检测重复free | 标志位 |
| 内存泄漏检测 | 追踪分配释放 | 日志/工具 |

---

## 五、并发设计

### 5.1 线程模型

```
┌─────────────────┐
│   主线程        │
│  (初始化/关闭)  │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│   工作线程1     │────▶│   工作线程2     │
│  (向量插入)     │     │  (向量搜索)     │
└─────────────────┘     └─────────────────┘
         │                       │
         └───────────┬───────────┘
                     ▼
         ┌─────────────────┐
         │   锁管理器      │
         │  (读写锁/自旋锁)│
         └─────────────────┘
                     │
         ┌───────────┴───────────┐
         ▼                       ▼
┌─────────────────┐     ┌─────────────────┐
│   Collection1   │     │   Collection2  │
└─────────────────┘     └─────────────────┘
```

### 5.2 锁策略

| 锁类型 | 使用场景 | 粒度 |
|--------|----------|------|
| 读写锁 | 集合访问 | Collection级 |
| 自旋锁 | 短临界区 | Arena级 |
| 原子操作 | 计数器 | 字段级 |

### 5.3 线程安全API

```c
jw_vecdb_open();        // 线程安全
jw_vecdb_close();       // 线程安全
jw_collection_insert(); // 线程安全(单Collection)
jw_collection_search(); // 线程安全(读操作)
jw_collection_delete(); // 线程安全(单Collection)
```

---

## 六、存储设计

### 6.1 持久化格式

**文件结构**:

```
database.jwd/
├── header.db         # 数据库头
├── collection_0.db  # Collection数据
├── collection_1.db  # Collection数据
└── wal.db           # 预写日志
```

**Header格式**:

```c
typedef struct {
    uint32_t magic;        // 魔数 0x4A574442 ("JWDB")
    uint32_t version;      // 版本号
    uint32_t checksum;     // 校验和
    uint64_t created_at;  // 创建时间
    uint64_t modified_at; // 修改时间
    size_t collection_count;
    size_t metadata_size;
} StorageHeader;
```

### 6.2 预写日志(WAL)

| 特性 | 描述 |
|------|------|
| 原子性 | 确保数据一致性 |
| 恢复 | 系统崩溃后恢复 |
| 性能 | 批量写入优化 |

### 6.3 内存映射

| 模式 | 适用场景 | 优点 |
|------|----------|------|
| MAP_SHARED | 生产环境 | 持久化支持 |
| MAP_PRIVATE | 临时数据 | 写入性能 |

---

## 七、扩展性设计

### 7.1 模块扩展

| 扩展点 | 描述 | 实现方式 |
|--------|------|----------|
| 索引算法 | 添加新索引类型 | 实现Index接口 |
| 量化方法 | 添加新量化算法 | 实现Quantizer接口 |
| 距离度量 | 添加新距离函数 | 函数指针注册 |
| 存储后端 | 添加新存储引擎 | 实现Storage接口 |

### 7.2 接口扩展

```c
typedef struct {
    int (*init)(void** ctx);
    int (*insert)(void* ctx, const void* data, size_t size, uint64_t* out_id);
    int (*search)(void* ctx, const void* query, size_t k, uint64_t* out_ids, float* scores);
    int (*destroy)(void* ctx);
} IndexInterface;
```

### 7.3 配置扩展

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

## 八、错误处理设计

### 8.1 错误码体系

| 错误码 | 描述 | 严重程度 |
|--------|------|----------|
| JW_OK | 成功 | - |
| JW_ERROR | 一般错误 | 低 |
| JW_OUT_OF_MEMORY | 内存不足 | 中 |
| JW_INVALID_PARAMETER | 参数错误 | 中 |
| JW_NOT_FOUND | 未找到 | 低 |
| JW_ALREADY_EXISTS | 已存在 | 低 |
| JW_STORAGE_ERROR | 存储错误 | 高 |
| JW_CORRUPTED | 数据损坏 | 高 |

### 8.2 错误处理策略

| 场景 | 处理策略 |
|------|----------|
| 内存分配失败 | 返回错误，清理资源 |
| 存储写入失败 | 重试，日志记录 |
| 数据损坏 | 尝试恢复，备份回滚 |
| 并发冲突 | 重试或返回错误 |

---

## 九、配置管理

### 9.1 运行时配置

```c
typedef struct {
    size_t arena_block_size;     // Arena块大小
    size_t max_memory;           // 最大内存限制
    size_t thread_pool_size;     // 线程池大小
    bool enable_wal;             // 启用预写日志
    bool enable_mmap;            // 启用内存映射
    LogLevel log_level;          // 日志级别
} VecDBConfig;
```

### 9.2 集合配置

```c
typedef struct {
    size_t max_vectors;          // 最大向量数
    size_t dimension;            // 向量维度
    IndexType index_type;        // 索引类型
    size_t index_params;        // 索引参数
    bool enable_compression;     // 启用压缩
} CollectionConfig;
```

---

## 十、版本兼容性

### 10.1 存储格式兼容性

| 版本 | 兼容性 | 说明 |
|------|--------|------|
| v0.x | 不兼容 | 早期版本 |
| v1.x | 向后兼容 | 支持读取旧版本数据 |

### 10.2 API兼容性

| 类型 | 兼容性策略 |
|------|------------|
| 公共API | 稳定，不删除 |
| 内部API | 可变更 |
| 实验性API | 可能变更 |

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
