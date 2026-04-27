/*
 * jw_storage.h - JinWo VecDB 存储抽象层
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * =============================================================================
 * 
 * 存储层设计说明:
 * 
 * 存储抽象层负责向量数据的持久化和读取，支持多种后端:
 *   1. 内存存储 (默认，适合临时数据)
 *   2. 文件存储 (单机持久化)
 *   3. MMAP存储 (内存映射，高效读取)
 *   4. 自定义存储 (用户实现接口)
 * 
 * 设计理念:
 *   - 统一的存储接口，底层可替换
 *   - 支持增量写入和随机读取
 *   - 支持压缩和校验
 *   - 跨平台兼容
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_STORAGE_H
#define JW_STORAGE_H

#include "jw_types.h"
#include "jw_arena.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 存储类型定义
 * =============================================================================
 */

/**
 * 存储后端类型
 */
typedef enum jw_storage_type {
    JW_STORAGE_TYPE_MEMORY = 0,      /* 内存存储 */
    JW_STORAGE_TYPE_FILE,            /* 文件存储 */
    JW_STORAGE_TYPE_MMAP,            /* 内存映射文件 */
    JW_STORAGE_TYPE_CUSTOM,          /* 自定义存储 */
} jw_storage_type_t;

/**
 * 存储打开模式
 */
typedef enum jw_storage_open_mode {
    JW_STORAGE_READ = 0,        /* 只读 */
    JW_STORAGE_WRITE,           /* 只写 */
    JW_STORAGE_READWRITE,       /* 读写 */
    JW_STORAGE_APPEND,          /* 追加 */
    JW_STORAGE_CREATE,          /* 创建 */
    JW_STORAGE_TRUNCATE,        /* 截断 */
    JW_STORAGE_EXCL,            /* 排他 */
} jw_storage_open_mode_t;

/**
 * 压缩算法
 */
typedef enum jw_compression {
    JW_COMPRESS_NONE = 0,       /* 不压缩 */
    JW_COMPRESS_ZLIB,           /* ZLIB压缩 */
    JW_COMPRESS_LZ4,            /* LZ4快速压缩 */
    JW_COMPRESS_ZSTD,           /* Zstandard压缩 */
    JW_COMPRESS_SNAPPY,         /* Snappy压缩 */
} jw_compression_t;

/**
 * 校验算法
 */
typedef enum jw_checksum {
    JW_CHECKSUM_NONE = 0,       /* 不校验 */
    JW_CHECKSUM_CRC32,          /* CRC32 */
    JW_CHECKSUM_MD5,            /* MD5 */
    JW_CHECKSUM_SHA256,         /* SHA256 */
} jw_checksum_t;

/*
 * =============================================================================
 * 存储配置
 * =============================================================================
 */

/**
 * 存储配置参数
 */
typedef struct jw_storage_config {
    jw_storage_type_t type;         /* 存储类型 */
    jw_storage_open_mode_t mode;         /* 打开模式 */
    jw_str_t path;                  /* 文件路径 (文件存储时) */

    /* 性能参数 */
    jw_size_t block_size;           /* 块大小 (默认4KB) */
    jw_size_t cache_size;           /* 缓存大小 (默认64MB) */
    jw_size_t buffer_size;          /* 缓冲区大小 */

    /* 压缩与校验 */
    jw_compression_t compression;   /* 压缩算法 */
    jw_checksum_t checksum;         /* 校验算法 */

    /* 文件存储选项 */
    jw_bool_t sync_on_write;        /* 写入后同步 */
    jw_bool_t direct_io;            /* 直接IO (绕过页缓存) */
    jw_bool_t auto_compact;         /* 自动压缩整理 */

    /* MMAP选项 */
    jw_bool_t mmap_populate;        /* 预加载映射 */
    jw_bool_t mmap_huge_pages;      /* 大页支持 */
} jw_storage_config_t;

/*
 * =============================================================================
 * 文件格式定义
 * =============================================================================
 */

/**
 * 存储文件魔数
 */
#define JW_STORAGE_MAGIC        0x4A575644  /* "JWVD" */
#define JW_STORAGE_VERSION      0x00010000  /* v1.0 */

/**
 * 文件格式标志位
 */
#define JW_STORAGE_FLAG_LITTLE_ENDIAN  0x00000001U  /* 小端序存储 */
#define JW_STORAGE_FLAG_BIG_ENDIAN     0x00000002U  /* 大端序存储 */
#define JW_STORAGE_FLAG_COMPRESSED     0x00000004U  /* 数据已压缩 */
#define JW_STORAGE_FLAG_HAS_INDEX      0x00000008U  /* 包含索引 */

/* 当前平台字节序标志 */
#if JW_LITTLE_ENDIAN
    #define JW_STORAGE_NATIVE_ENDIAN JW_STORAGE_FLAG_LITTLE_ENDIAN
#else
    #define JW_STORAGE_NATIVE_ENDIAN JW_STORAGE_FLAG_BIG_ENDIAN
#endif

/*
 * =============================================================================
 * 固定大小存储结构体 (用于文件持久化)
 * =============================================================================
 * 
 * 说明: 这些结构体使用 packed 属性确保跨平台布局一致
 *       所有写入文件的整数都使用小端序
 */

#pragma pack(push, 1)

/**
 * 文件头结构 (固定大小: 176 bytes)
 */
typedef struct jw_storage_header_fixed {
    jw_uint32_t magic;              /* 4: 魔数 JW_STORAGE_MAGIC */
    jw_uint32_t version;            /* 4: 版本号 */
    jw_uint64_t create_time;        /* 8: 创建时间 (微秒) */
    jw_uint64_t update_time;        /* 8: 更新时间 (微秒) */
    
    /* 数据区信息 */
    jw_uint64_t data_offset;        /* 8: 数据区偏移 */
    jw_uint64_t data_size;          /* 8: 数据区大小 */
    jw_uint64_t index_offset;       /* 8: 索引区偏移 */
    jw_uint64_t index_size;         /* 8: 索引区大小 */
    
    /* 元数据 */
    jw_uint32_t flags;              /* 4: 标志位 (字节序标志等) */
    jw_uint32_t checksum;           /* 4: 文件CRC32校验 */
    
    /* 预留 */
    jw_uint8_t reserved[120];       /* 120: 保留空间 */
} jw_storage_header_fixed_t;        /* 总计: 176 bytes */

/* 静态断言确保大小正确 */
/*
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static_assert(sizeof(jw_storage_header_fixed_t) == 176, 
               "jw_storage_header_fixed_t size mismatch");
#else
_Static_assert(sizeof(jw_storage_header_fixed_t) == 176, 
               "jw_storage_header_fixed_t size mismatch");
#endif
*/

/**
 * Collection 头结构 (固定大小: 256 bytes)
 */
typedef struct jw_collection_header_fixed {
    jw_uint32_t magic;              /* 4: 魔数 */
    jw_uint32_t version;            /* 4: 版本号 */
    jw_uint64_t create_time;        /* 8: 创建时间 */
    jw_uint64_t update_time;        /* 8: 更新时间 */
    
    /* Collection 基本信息 */
    char name[64];                  /* 64: Collection 名称 */
    jw_uint32_t dim;                /* 4: 向量维度 */
    jw_uint32_t metric;             /* 4: 距离度量类型 */
    jw_uint32_t index_type;         /* 4: 索引类型 */
    jw_uint32_t vector_count;       /* 4: 向量数量 */
    jw_uint64_t next_vid;           /* 8: 下一个向量ID */
    
    /* 索引配置 */
    jw_uint32_t hnsw_m;             /* 4: HNSW M参数 */
    jw_uint32_t hnsw_ef_construction; /* 4: HNSW ef_construction */
    jw_uint32_t hnsw_ef_search;     /* 4: HNSW ef_search */
    jw_uint32_t ivf_nlist;          /* 4: IVF 聚类数 */
    jw_uint32_t ivf_nprobe;         /* 4: IVF 探测数 */
    
    /* 数据位置 */
    jw_uint64_t vectors_offset;     /* 8: 向量数据偏移 */
    jw_uint64_t vectors_size;       /* 8: 向量数据大小 */
    jw_uint64_t index_offset;       /* 8: 索引数据偏移 */
    jw_uint64_t index_size;         /* 8: 索引数据大小 */
    jw_uint64_t meta_offset;        /* 8: 元数据偏移 */
    jw_uint64_t meta_size;          /* 8: 元数据大小 */
    
    /* 校验 */
    jw_uint32_t checksum;           /* 4: CRC32校验 */
    
    /* 预留 */
    jw_uint8_t reserved[8];         /* 8: 保留 */
} jw_collection_header_fixed_t;     /* 总计: 256 bytes */

/*
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static_assert(sizeof(jw_collection_header_fixed_t) == 256,
               "jw_collection_header_fixed_t size mismatch");
#else
_Static_assert(sizeof(jw_collection_header_fixed_t) == 256,
               "jw_collection_header_fixed_t size mismatch");
#endif
*/

/**
 * 向量记录结构 (固定大小部分: 32 bytes + 向量数据)
 */
typedef struct jw_vector_record_fixed {
    jw_uint64_t vid;                /* 8: 向量ID */
    jw_uint32_t dim;                /* 4: 向量维度 */
    jw_uint32_t flags;              /* 4: 标志位 */
    jw_uint64_t meta_offset;        /* 8: 元数据偏移 (0表示无元数据) */
    jw_uint32_t meta_size;          /* 4: 元数据大小 */
    jw_uint32_t checksum;           /* 4: CRC32校验 */
    /* 后面跟着 dim 个 float32 向量数据 */
} jw_vector_record_fixed_t;         /* 总计: 32 bytes */

/*
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static_assert(sizeof(jw_vector_record_fixed_t) == 32,
               "jw_vector_record_fixed_t size mismatch");
#else
_Static_assert(sizeof(jw_vector_record_fixed_t) == 32,
               "jw_vector_record_fixed_t size mismatch");
#endif
*/

/**
 * 数据块头 (固定大小: 32 bytes)
 */
typedef struct jw_data_block_fixed {
    jw_uint64_t block_id;           /* 8: 块ID */
    jw_uint64_t block_size;         /* 8: 数据大小 (不含头) */
    jw_uint32_t data_type;          /* 4: 数据类型 */
    jw_uint32_t flags;              /* 4: 标志 */
    jw_uint32_t checksum;           /* 4: CRC32校验 */
    jw_uint8_t compression;         /* 1: 压缩类型 */
    jw_uint8_t reserved[3];         /* 3: 保留 */
} jw_data_block_fixed_t;            /* 总计: 32 bytes */

/*
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static_assert(sizeof(jw_data_block_fixed_t) == 32,
               "jw_data_block_fixed_t size mismatch");
#else
_Static_assert(sizeof(jw_data_block_fixed_t) == 32,
               "jw_data_block_fixed_t size mismatch");
#endif
*/

#pragma pack(pop)

/* 数据类型枚举 */
typedef enum jw_data_type {
    JW_DATA_VECTORS = 1,            /* 向量数据 */
    JW_DATA_INDEX_IVF = 2,          /* IVF索引 */
    JW_DATA_INDEX_HNSW = 3,         /* HNSW索引 */
    JW_DATA_METADATA = 4,           /* 元数据 */
    JW_DATA_SCHEMA = 5,             /* 模式定义 */
} jw_data_type_t;

/*
 * =============================================================================
 * 运行时存储结构体 (内存中使用，可以扩展)
 * =============================================================================
 */

/**
 * 文件头结构 (运行时)
 */
typedef struct jw_storage_header {
    jw_uint32_t magic;              /* 魔数 JW_STORAGE_MAGIC */
    jw_uint32_t version;            /* 版本号 */
    jw_uint64_t create_time;        /* 创建时间 */
    jw_uint64_t update_time;        /* 更新时间 */
    
    /* 数据区信息 */
    jw_uint64_t data_offset;        /* 数据区偏移 */
    jw_uint64_t data_size;          /* 数据区大小 */
    jw_uint64_t index_offset;       /* 索引区偏移 */
    jw_uint64_t index_size;         /* 索引区大小 */
    
    /* 元数据 */
    jw_uint32_t flags;              /* 标志位 */
    jw_uint32_t checksum;           /* 文件校验和 */
    
    /* 预留 */
    jw_uint8_t reserved[128];
} jw_storage_header_t;

/**
 * 数据块头 (运行时)
 */
typedef struct jw_data_block {
    jw_uint64_t block_id;           /* 块ID */
    jw_uint32_t size;               /* 数据大小 */
    jw_uint32_t flags;              /* 标志 */
    jw_uint32_t checksum;           /* 块校验 */
    jw_uint8_t compression;         /* 压缩类型 */
    jw_uint8_t reserved[7];
} jw_data_block_t;

/*
 * =============================================================================
 * 存储句柄与迭代器
 * =============================================================================
 */

/**
 * 存储句柄 (不透明指针)
 */
typedef struct jw_storage jw_storage_t;

/**
 * 存储迭代器
 */
typedef struct jw_storage_iterator {
    jw_storage_t *storage;          /* 存储句柄 */
    jw_uint64_t current_offset;     /* 当前偏移 */
    jw_uint64_t end_offset;         /* 结束偏移 */
    jw_data_block_t current_block;  /* 当前块信息 */
} jw_storage_iterator_t;

/*
 * =============================================================================
 * 存储操作API
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 * 存储生命周期
 * -----------------------------------------------------------------------------
 */

/**
 * 创建存储
 * 
 * @param arena   内存池
 * @param config 配置参数
 * @return 存储句柄，失败返回NULL
 */
JW_API jw_storage_t *jw_storage_create(jw_arena_t *arena,
                                        const jw_storage_config_t *config);

/**
 * 打开已有存储
 * 
 * @param arena   内存池
 * @param path   存储路径
 * @param mode   打开模式
 * @return 存储句柄
 */
JW_API jw_storage_t *jw_storage_open(jw_arena_t *arena,
                                      const char *path,
                                      jw_storage_open_mode_t mode);

/**
 * 关闭存储
 * 
 * @param storage 存储句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_close(jw_storage_t *storage);

/**
 * 销毁存储 (删除数据)
 * 
 * @param storage 存储句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_destroy(jw_storage_t *storage);

/**
 * 同步存储到磁盘
 * 
 * @param storage 存储句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_sync(jw_storage_t *storage);

/*
 * -----------------------------------------------------------------------------
 * 读写操作
 * -----------------------------------------------------------------------------
 */

/**
 * 写入数据
 * 
 * @param storage 存储句柄
 * @param data    数据指针
 * @param size    数据大小
 * @param offset  输出写入偏移
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_write(jw_storage_t *storage,
                                     const void *data,
                                     jw_size_t size,
                                     jw_uint64_t *offset);

/**
 * 追加写入数据
 * 
 * @param storage 存储句柄
 * @param data    数据指针
 * @param size    数据大小
 * @return 写入偏移，-1表示失败
 */
JW_API jw_ssize_t jw_storage_append(jw_storage_t *storage,
                                     const void *data,
                                     jw_size_t size);

/**
 * 读取数据
 * 
 * @param storage 存储句柄
 * @param offset  读取偏移
 * @param buffer  输出缓冲区
 * @param size    读取大小
 * @return 实际读取大小，-1表示失败
 */
JW_API jw_ssize_t jw_storage_read(jw_storage_t *storage,
                                   jw_uint64_t offset,
                                   void *buffer,
                                   jw_size_t size);

/**
 * 写入向量数据
 * 
 * @param storage 存储句柄
 * @param vid     向量ID
 * @param vec     向量数据
 * @param dim     向量维度
 * @param offset  输出写入偏移
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_write_vector(jw_storage_t *storage,
                                            jw_uint64_t vid,
                                            jw_cvec_t vec,
                                            jw_dim_t dim,
                                            jw_uint64_t *offset);

/**
 * 读取向量数据
 * 
 * @param storage 存储句柄
 * @param offset  读取偏移
 * @param vid     输出向量ID
 * @param vec     输出向量
 * @param dim     向量维度
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_read_vector(jw_storage_t *storage,
                                           jw_uint64_t offset,
                                           jw_uint64_t *vid,
                                           jw_vec_t vec,
                                           jw_dim_t dim);

/*
 * -----------------------------------------------------------------------------
 * 字节序安全读写 (自动转换为小端序存储)
 * -----------------------------------------------------------------------------
 */

/**
 * 写入 uint32 (小端序)
 */
JW_API jw_status_t jw_storage_write_u32(jw_storage_t *storage,
                                         jw_uint32_t value,
                                         jw_uint64_t *offset);

/**
 * 读取 uint32 (从小端序转换)
 */
JW_API jw_status_t jw_storage_read_u32(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_uint32_t *value);

/**
 * 写入 uint64 (小端序)
 */
JW_API jw_status_t jw_storage_write_u64(jw_storage_t *storage,
                                         jw_uint64_t value,
                                         jw_uint64_t *offset);

/**
 * 读取 uint64 (从小端序转换)
 */
JW_API jw_status_t jw_storage_read_u64(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_uint64_t *value);

/**
 * 写入 float32 (小端序)
 */
JW_API jw_status_t jw_storage_write_f32(jw_storage_t *storage,
                                         jw_float32_t value,
                                         jw_uint64_t *offset);

/**
 * 读取 float32 (从小端序转换)
 */
JW_API jw_status_t jw_storage_read_f32(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_float32_t *value);

/**
 * 写入 float32 数组 (小端序)
 */
JW_API jw_status_t jw_storage_write_f32_array(jw_storage_t *storage,
                                               const jw_float32_t *values,
                                               jw_size_t count,
                                               jw_uint64_t *offset);

/**
 * 读取 float32 数组 (从小端序转换)
 */
JW_API jw_status_t jw_storage_read_f32_array(jw_storage_t *storage,
                                              jw_uint64_t offset,
                                              jw_float32_t *values,
                                              jw_size_t count);

/**
 * 写入固定大小的文件头
 */
JW_API jw_status_t jw_storage_write_header(jw_storage_t *storage,
                                            const jw_storage_header_fixed_t *header);

/**
 * 读取固定大小的文件头
 */
JW_API jw_status_t jw_storage_read_header(jw_storage_t *storage,
                                           jw_storage_header_fixed_t *header);

/**
 * 写入固定大小的 Collection 头
 */
JW_API jw_status_t jw_storage_write_collection_header(
    jw_storage_t *storage,
    const jw_collection_header_fixed_t *header,
    jw_uint64_t *offset);

/**
 * 读取固定大小的 Collection 头
 */
JW_API jw_status_t jw_storage_read_collection_header(
    jw_storage_t *storage,
    jw_uint64_t offset,
    jw_collection_header_fixed_t *header);

/*
 * -----------------------------------------------------------------------------
 * 块操作
 * -----------------------------------------------------------------------------
 */

/**
 * 分配数据块
 * 
 * @param storage 存储句柄
 * @param size    块大小
 * @return 块偏移，-1表示失败
 */
JW_API jw_ssize_t jw_storage_alloc_block(jw_storage_t *storage,
                                          jw_size_t size);

/**
 * 释放数据块
 * 
 * @param storage   存储句柄
 * @param block_id  块ID
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_free_block(jw_storage_t *storage,
                                          jw_uint64_t block_id);

/**
 * 获取块信息
 * 
 * @param storage  存储句柄
 * @param block_id 块ID
 * @param info     输出块信息
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_get_block_info(jw_storage_t *storage,
                                              jw_uint64_t block_id,
                                              jw_data_block_t *info);

/*
 * -----------------------------------------------------------------------------
 * 迭代器
 * -----------------------------------------------------------------------------
 */

/**
 * 创建存储迭代器
 * 
 * @param storage 存储句柄
 * @return 迭代器指针
 */
JW_API jw_storage_iterator_t *jw_storage_iterator_create(jw_storage_t *storage);

/**
 * 销毁迭代器
 * 
 * @param iter 迭代器
 */
JW_API void jw_storage_iterator_destroy(jw_storage_iterator_t *iter);

/**
 * 迭代到下一个块
 * 
 * @param iter 迭代器
 * @return JW_TRUE 有更多数据
 */
JW_API jw_bool_t jw_storage_iterator_next(jw_storage_iterator_t *iter);

/**
 * 获取当前块数据
 * 
 * @param iter   迭代器
 * @param buffer 输出缓冲区
 * @param size   缓冲区大小
 * @return 实际数据大小
 */
JW_API jw_size_t jw_storage_iterator_get_data(jw_storage_iterator_t *iter,
                                               void *buffer,
                                               jw_size_t size);

/*
 * -----------------------------------------------------------------------------
 * 统计与维护
 * -----------------------------------------------------------------------------
 */

/**
 * 存储统计信息
 */
typedef struct jw_storage_stats {
    jw_uint64_t total_size;     /* 总大小 */
    jw_uint64_t used_size;      /* 已用大小 */
    jw_uint64_t free_size;      /* 空闲大小 */
    jw_uint64_t block_count;    /* 块数量 */
    jw_uint64_t fragment_count; /* 碎片数量 */
    jw_float32_t fragmentation; /* 碎片率 */
} jw_storage_stats_t;

/**
 * 获取存储统计信息
 * 
 * @param storage 存储句柄
 * @param stats   统计信息输出
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_get_stats(const jw_storage_t *storage,
                                         jw_storage_stats_t *stats);

/**
 * 压缩整理存储
 * 
 * @param storage 存储句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_compact(jw_storage_t *storage);

/**
 * 校验存储完整性
 * 
 * @param storage 存储句柄
 * @return JW_SUCCESS 校验通过
 */
JW_API jw_status_t jw_storage_verify(jw_storage_t *storage);

/*
 * =============================================================================
 * 自定义存储后端接口
 * =============================================================================
 */

/**
 * 自定义存储操作函数表
 */
typedef struct jw_storage_ops {
    /* 生命周期 */
    jw_status_t (*open)(void **handle, const jw_storage_config_t *config);
    jw_status_t (*close)(void *handle);
    jw_status_t (*destroy)(void *handle);
    jw_status_t (*sync)(void *handle);
    
    /* 读写 */
    jw_ssize_t (*read)(void *handle, jw_uint64_t offset, void *buf, jw_size_t size);
    jw_ssize_t (*write)(void *handle, jw_uint64_t offset, const void *data, jw_size_t size);
    jw_ssize_t (*append)(void *handle, const void *data, jw_size_t size);
    
    /* 块管理 */
    jw_ssize_t (*alloc_block)(void *handle, jw_size_t size);
    jw_status_t (*free_block)(void *handle, jw_uint64_t block_id);
    
    /* 统计 */
    jw_status_t (*get_stats)(void *handle, jw_storage_stats_t *stats);
    jw_status_t (*compact)(void *handle);
    jw_status_t (*verify)(void *handle);
} jw_storage_ops_t;

/**
 * 注册自定义存储后端
 * 
 * @param name 存储类型名称
 * @param ops  操作函数表
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_storage_register_backend(const char *name,
                                                const jw_storage_ops_t *ops);

/*
 * =============================================================================
 * 默认配置宏
 * =============================================================================
 */

#define JW_STORAGE_CONFIG_DEFAULT { \
    .type = JW_STORAGE_TYPE_FILE, \
    .mode = JW_STORAGE_CREATE, \
    .block_size = 4096, \
    .cache_size = 64 * 1024 * 1024, \
    .compression = JW_COMPRESS_NONE, \
    .checksum = JW_CHECKSUM_CRC32, \
    .sync_on_write = JW_FALSE, \
    .direct_io = JW_FALSE, \
    .auto_compact = JW_TRUE \
}

#define JW_STORAGE_CONFIG_MMAP { \
    .type = JW_STORAGE_MMAP, \
    .mode = JW_STORAGE_READWRITE, \
    .block_size = 4096, \
    .cache_size = 256 * 1024 * 1024, \
    .mmap_populate = JW_TRUE \
}

#define JW_STORAGE_CONFIG_MEMORY { \
    .type = JW_STORAGE_MEMORY, \
    .mode = JW_STORAGE_READWRITE, \
    .block_size = 4096 \
}

JW_END_DECL

#endif /* JW_STORAGE_H */
