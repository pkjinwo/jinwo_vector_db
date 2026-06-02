/*
 * jw_index.h - JinWo VecDB 索引结构
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
 * 索引设计说明:
 * 
 * 本文件实现了向量数据库的核心索引算法:
 *   1. IVF (Inverted File Index) - 倒排索引，适合大规模数据集
 *   2. HNSW (Hierarchical Navigable Small World) - 层级导航小世界图
 * 
 * 索引选择建议:
 *   - IVF: 内存占用小，适合超大规模数据 (千万级以上)
 *   - HNSW: 查询速度快，精度高，适合中小规模数据 (百万级)
 * 
 * 性能优化:
 *   - SIMD加速距离计算
 *   - 支持PQ/SQ量化减少内存占用
 *   - 批量建索引和查询
 * 
 * 版本: 0.1.30
 * 作者: 灵活就业码农
 */

#ifndef JW_INDEX_H
#define JW_INDEX_H

#include "jw_types.h"
#include "jw_arena.h"
#include "jw_lock.h"
#include "jw_quant.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 索引类型定义
 * =============================================================================
 */



/*
 * =============================================================================
 * 索引配置参数
 * =============================================================================
 */

/**
 * IVF索引配置参数
 */
typedef struct jw_ivf_config {
    jw_uint32_t nlist;          /* 聚类中心数量 (倒排列表数) */
    jw_uint32_t nprobe;         /* 查询时探测的聚类数 */
    jw_uint32_t max_iter;       /* K-means最大迭代次数 */
    jw_float32_t tolerance;     /* K-means收敛阈值 */
    jw_uint32_t seed;           /* 随机种子 */
} jw_ivf_config_t;

/**
 * HNSW索引配置参数
 * 
 * 算法原理:
 *   - M: 每个节点的最大连接数，影响图的稠密度
 *   - ef_construction: 建索引时的搜索宽度，影响建索引质量
 *   - ef_search: 查询时的搜索宽度，影响查询精度
 */
typedef struct jw_hnsw_config {
    jw_uint32_t M;                  /* 每层最大连接数 (推荐: 16-64) */
    jw_uint32_t ef_construction;    /* 建索引搜索宽度 (推荐: 100-400) */
    jw_uint32_t ef_search;          /* 查询搜索宽度 (推荐: 50-200) */
    jw_uint32_t max_level;          /* 最大层级 (0表示自动计算) */
    jw_uint32_t seed;               /* 随机种子 */
    jw_float32_t level_mult;        /* 层级乘数因子 (默认 1/ln(M)) */
} jw_hnsw_config_t;

/**
 * PQ量化配置参数
 */
typedef struct jw_pq_config {
    jw_uint32_t nsub;           /* 子向量数量 (必须能被维度整除) */
    jw_uint32_t nbits;          /* 每个子向量的编码位数 (通常为8) */
    jw_uint32_t max_iter;       /* K-means迭代次数 */
} jw_pq_config_t;

/**
 * 通用索引配置
 */
typedef struct jw_index_config {
    jw_index_type_t type;       /* 索引类型 */
    jw_metric_t metric;    /* 距离度量 */
    jw_quant_type_t quant;      /* 量化类型 */
    jw_dim_t dim;               /* 向量维度 */
    jw_size_t capacity;         /* 预估容量 */
    
    union {
        jw_ivf_config_t ivf;    /* IVF配置 */
        jw_hnsw_config_t hnsw;  /* HNSW配置 */
    } params;
    
    jw_pq_config_t pq;          /* PQ量化配置 (可选) */
} jw_index_config_t;

/*
 * =============================================================================
 * 索引基础结构
 * =============================================================================
 */

/**
 * 向量ID类型
 *   使用64位，支持超过40亿向量
 */
typedef jw_uint64_t jw_vid_t;



/**
 * 搜索结果数组
 */
typedef struct jw_search_results {
    jw_search_result_t *items;  /* 结果数组 */
    jw_size_t count;            /* 结果数量 */
    jw_size_t capacity;         /* 数组容量 */
} jw_search_results_t;

/*
 * =============================================================================
 * IVF索引结构
 * =============================================================================
 */

/**
 * 倒排列表项
 */
typedef struct jw_ivf_entry {
    jw_vid_t vid;               /* 向量ID */
    jw_vec_t vec;               /* 向量数据 (可选，量化后可能为NULL) */
    jw_uint8_t *code;           /* 量化编码 (PQ时使用) */
} jw_ivf_entry_t;

/**
 * 倒排列表
 */
typedef struct jw_ivf_list {
    jw_vec_t centroid;          /* 聚类中心向量 */
    jw_ivf_entry_t *entries;    /* 向量条目数组 */
    jw_size_t count;            /* 当前数量 */
    jw_size_t capacity;         /* 容量 */
    jw_rwlock_t *lock;           /* 读写锁 */
} jw_ivf_list_t;

/**
 * IVF索引结构
 */
typedef struct jw_ivf_index {
    jw_index_type_t type;       /* 索引类型 */
    jw_metric_t metric;    /* 距离度量 */
    jw_dim_t dim;               /* 向量维度 */
    jw_size_t ntotal;           /* 总向量数 */
    
    /* IVF配置 */
    jw_ivf_config_t config;
    
    /* 聚类中心 */
    jw_ivf_list_t *lists;       /* 倒排列表数组 */
    jw_uint32_t nlist;          /* 倒排列表数量 */
    
    /* 量化器 */
    jw_quant_type_t quant_type;
    jw_pq_config_t pq_config;
    jw_pq_quantizer_t *pq;      /* PQ量化器 */
    jw_sq_quantizer_t *sq;      /* SQ量化器 */
    jw_float32_t *scales;       /* SQ缩放因子数组 */
    
    /* 内存管理 */
    jw_arena_t *arena;
    jw_mutex_t *lock;
    
    /* 状态 */
    jw_bool_t trained;          /* 是否已完成训练 */
} jw_ivf_index_t;

/*
 * =============================================================================
 * HNSW索引结构
 * =============================================================================
 */

/**
 * HNSW节点
 */
typedef struct jw_hnsw_node {
    jw_vid_t vid;                   /* 向量ID */
    jw_vec_t vec;                   /* 向量数据 */
    jw_uint8_t *code;               /* 量化编码 (可选) */
    jw_uint32_t level;              /* 节点层级 */
    jw_uint32_t max_level;          /* 最大可达层级 */
    jw_bool_t deleted;              /* 是否已删除 */
    
    /* 连接表: 每层一个连接数组 */
    jw_vid_t **links;               /* links[level][i] = 邻居ID */
    jw_uint32_t *link_counts;       /* 每层连接数 */
    jw_uint32_t max_M;              /* 最大连接数 */
    jw_uint32_t max_M0;             /* 第0层最大连接数 */
} jw_hnsw_node_t;

/**
 * HNSW层级
 */
typedef struct jw_hnsw_level {
    jw_hnsw_node_t **nodes;         /* 该层节点指针 */
    jw_size_t count;                /* 节点数量 */
} jw_hnsw_level_t;

/**
 * HNSW索引结构
 */
typedef struct jw_hnsw_index {
    jw_index_type_t type;           /* 索引类型 */
    jw_metric_t metric;        /* 距离度量 */
    jw_dim_t dim;                   /* 向量维度 */
    jw_size_t ntotal;               /* 总向量数 */
    
    /* HNSW配置 */
    jw_hnsw_config_t config;
    
    /* 节点存储 */
    jw_hnsw_node_t **nodes;         /* 所有节点数组 (按vid索引) */
    jw_size_t capacity;             /* 节点容量 */
    
    /* 层级结构 */
    jw_hnsw_level_t *levels;        /* 层级数组 */
    jw_uint32_t max_level;          /* 当前最大层级 */
    jw_vid_t entry_point;           /* 入口点节点ID */
    
    /* 量化 */
    jw_quant_type_t quant_type;
    jw_pq_config_t pq_config;
    jw_pq_quantizer_t *pq;      /* PQ量化器 */
    jw_sq_quantizer_t *sq;      /* SQ量化器 */
    
    /* 内存管理 */
    jw_arena_t *arena;
    jw_rwlock_t *lock;
    
    /* 随机数生成器状态 */
    jw_uint64_t rng_state;
} jw_hnsw_index_t;

/*
 * =============================================================================
 * 通用索引接口 (抽象接口)
 * =============================================================================
 */

/**
 * 通用索引结构体 (使用联合体实现多态)
 */
typedef struct jw_index_t {
    jw_index_type_t type;       /* 索引类型 */
    void *impl;                 /* 具体实现指针 */
    jw_arena_t *arena;            /* 内存池 */
} jw_index_t;

/*
 * =============================================================================
 * 索引操作API
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 * 索引创建与销毁
 * -----------------------------------------------------------------------------
 */

/**
 * 创建索引
 * 
 * @param arena   内存池 (NULL则自动创建)
 * @param config 索引配置
 * @return 索引指针，失败返回NULL
 */
JW_API jw_index_t *jw_index_create(jw_arena_t *arena, 
                                    const jw_index_config_t *config);

/**
 * 销毁索引
 * 
 * @param index 索引指针
 */
JW_API void jw_index_destroy(jw_index_t *index);

/**
 * 获取索引类型
 * 
 * @param index 索引指针
 * @return 索引类型
 */
JW_API jw_index_type_t jw_index_get_type(const jw_index_t *index);

/**
 * 获取向量维度
 * 
 * @param index 索引指针
 * @return 向量维度
 */
JW_API jw_dim_t jw_index_get_dim(const jw_index_t *index);

/**
 * 获取总向量数
 * 
 * @param index 索引指针
 * @return 向量总数
 */
JW_API jw_size_t jw_index_get_ntotal(const jw_index_t *index);

/*
 * -----------------------------------------------------------------------------
 * 训练与构建
 * -----------------------------------------------------------------------------
 */

/**
 * 训练索引 (对IVF等需要聚类中心的索引)
 * 
 * @param index     索引指针
 * @param vectors   训练向量数组
 * @param count     向量数量
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_train(jw_index_t *index,
                                   jw_cvec_t vectors,
                                   jw_size_t count);

/**
 * 检查索引是否已训练
 * 
 * @param index 索引指针
 * @return JW_TRUE 已训练，JW_FALSE 未训练
 */
JW_API jw_bool_t jw_index_is_trained(const jw_index_t *index);

/*
 * -----------------------------------------------------------------------------
 * 向量添加与删除
 * -----------------------------------------------------------------------------
 */

/**
 * 添加单个向量
 * 
 * @param index 索引指针
 * @param vid   向量ID
 * @param vec   向量数据
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_add(jw_index_t *index,
                                 jw_vid_t vid,
                                 jw_cvec_t vec);

/**
 * 批量添加向量
 * 
 * @param index   索引指针
 * @param vids    向量ID数组
 * @param vectors 向量数据数组
 * @param count   向量数量
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_add_batch(jw_index_t *index,
                                       const jw_vid_t *vids,
                                       jw_cvec_t vectors,
                                       jw_size_t count);

/**
 * 删除向量
 * 
 * @param index 索引指针
 * @param vid   向量ID
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_remove(jw_index_t *index, jw_vid_t vid);

/**
 * 批量删除向量
 * 
 * @param index 索引指针
 * @param vids  向量ID数组
 * @param count 向量数量
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_remove_batch(jw_index_t *index,
                                          const jw_vid_t *vids,
                                          jw_size_t count);

/*
 * -----------------------------------------------------------------------------
 * 向量搜索
 * -----------------------------------------------------------------------------
 */

/**
 * 搜索最近邻 (KNN)
 * 
 * @param index   索引指针
 * @param query   查询向量
 * @param k       返回结果数量
 * @param results 结果数组 (调用者分配)
 * @return 实际返回的结果数量
 */
JW_API jw_size_t jw_index_search(const jw_index_t *index,
                                  jw_cvec_t query,
                                  jw_size_t k,
                                  jw_search_result_t *results);

/**
 * 批量搜索最近邻
 * 
 * @param index   索引指针
 * @param queries 查询向量数组
 * @param nquery  查询数量
 * @param k       每个查询返回的结果数
 * @param results 结果数组 (nquery * k 大小)
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_search_batch(const jw_index_t *index,
                                          jw_cvec_t queries,
                                          jw_size_t nquery,
                                          jw_size_t k,
                                          jw_search_result_t *results);

/**
 * 范围搜索 (返回距离小于threshold的所有向量)
 * 
 * @param index     索引指针
 * @param query     查询向量
 * @param threshold 距离阈值
 * @param results   结果数组
 * @param capacity  结果数组容量
 * @return 实际返回的结果数量
 */
JW_API jw_size_t jw_index_range_search(const jw_index_t *index,
                                        jw_cvec_t query,
                                        jw_float32_t threshold,
                                        jw_search_result_t *results,
                                        jw_size_t capacity);

/*
 * -----------------------------------------------------------------------------
 * 索引持久化
 * -----------------------------------------------------------------------------
 */

/**
 * 保存索引到文件
 * 
 * @param index    索引指针
 * @param filename 文件路径
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_save(const jw_index_t *index,
                                  const jw_str_t *filename);

/**
 * 从文件加载索引
 * 
 * @param arena     内存池
 * @param filename 文件路径
 * @return 索引指针，失败返回NULL
 */
JW_API jw_index_t *jw_index_load(jw_arena_t *arena, const jw_str_t *filename);

/**
 * 序列化索引到内存缓冲区
 * 
 * @param index   索引指针
 * @param buffer  输出缓冲区指针 (调用者释放)
 * @param size    输出缓冲区大小
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_serialize(const jw_index_t *index,
                                       void **buffer,
                                       jw_size_t *size);

/**
 * 从内存缓冲区反序列化索引
 * 
 * @param arena   内存池
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 索引指针，失败返回NULL
 */
JW_API jw_index_t *jw_index_deserialize(jw_arena_t *arena,
                                         const void *buffer,
                                         jw_size_t size);

/*
 * -----------------------------------------------------------------------------
 * 索引统计与调试
 * -----------------------------------------------------------------------------
 */

/**
 * 索引统计信息
 */
typedef struct jw_index_stats {
    jw_size_t ntotal;           /* 总向量数 */
    jw_size_t memory_used;      /* 内存使用量 (字节) */
    jw_size_t disk_size;        /* 磁盘大小 (字节，序列化后) */
    jw_uint32_t nlist;          /* IVF: 倒排列表数 */
    jw_uint32_t max_level;      /* HNSW: 最大层级 */
    jw_float32_t avg_neighbors; /* HNSW: 平均邻居数 */
} jw_index_stats_t;

/**
 * 获取索引统计信息
 * 
 * @param index 索引指针
 * @param stats 统计信息输出
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_get_stats(const jw_index_t *index,
                                       jw_index_stats_t *stats);

/**
 * 打印索引信息 (调试用)
 * 
 * @param index 索引指针
 */
JW_API void jw_index_print_info(const jw_index_t *index);

/*
 * =============================================================================
 * IVF专用API
 * =============================================================================
 */

/**
 * 创建IVF索引
 * 
 * @param arena   内存池
 * @param dim    向量维度
 * @param config IVF配置
 * @return IVF索引指针
 */
JW_API jw_ivf_index_t *jw_ivf_create(jw_arena_t *arena,
                                      jw_dim_t dim,
                                      const jw_ivf_config_t *config);

/**
 * 销毁IVF索引
 * 
 * @param index IVF索引指针
 */
JW_API void jw_ivf_destroy(jw_ivf_index_t *index);

/**
 * 设置探测聚类数 (查询时使用)
 * 
 * @param index  IVF索引指针
 * @param nprobe 探测的聚类数
 */
JW_API void jw_ivf_set_nprobe(jw_ivf_index_t *index, jw_uint32_t nprobe);

/**
 * 获取最近的聚类中心
 * 
 * @param index IVF索引指针
 * @param query 查询向量
 * @param n     返回数量
 * @param ids   输出聚类ID数组
 * @return 实际返回数量
 */
JW_API jw_uint32_t jw_ivf_get_nearest_centroids(const jw_ivf_index_t *index,
                                                 jw_cvec_t query,
                                                 jw_uint32_t n,
                                                 jw_uint32_t *ids);

/*
 * =============================================================================
 * HNSW专用API
 * =============================================================================
 */

/**
 * 创建HNSW索引
 * 
 * @param arena   内存池
 * @param dim    向量维度
 * @param config HNSW配置
 * @return HNSW索引指针
 */
JW_API jw_hnsw_index_t *jw_hnsw_create(jw_arena_t *arena,
                                        jw_dim_t dim,
                                        const jw_hnsw_config_t *config);

/**
 * 销毁HNSW索引
 * 
 * @param index HNSW索引指针
 */
JW_API void jw_hnsw_destroy(jw_hnsw_index_t *index);

/**
 * 设置查询时的搜索宽度
 * 
 * @param index HNSW索引指针
 * @param ef    搜索宽度
 */
JW_API void jw_hnsw_set_ef_search(jw_hnsw_index_t *index, jw_uint32_t ef);

/**
 * 获取节点连接信息
 * 
 * @param index     HNSW索引指针
 * @param vid       节点ID
 * @param level     层级
 * @param neighbors 输出邻居ID数组
 * @param capacity  数组容量
 * @return 实际邻居数量
 */
JW_API jw_uint32_t jw_hnsw_get_neighbors(const jw_hnsw_index_t *index,
                                          jw_vid_t vid,
                                          jw_uint32_t level,
                                          jw_vid_t *neighbors,
                                          jw_uint32_t capacity);

/*
 * =============================================================================
 * 辅助宏定义
 * =============================================================================
 */

/* 默认IVF配置 */
#define JW_IVF_CONFIG_DEFAULT { \
    .nlist = 1024, \
    .nprobe = 16, \
    .max_iter = 100, \
    .tolerance = 1e-6f, \
    .seed = 42 \
}

/* 默认HNSW配置 */
#define JW_HNSW_CONFIG_DEFAULT { \
    .M = 32, \
    .ef_construction = 200, \
    .ef_search = 100, \
    .max_level = 0, \
    .seed = 42, \
    .level_mult = 0.0f \
}

/* 默认PQ配置 */
#define JW_PQ_CONFIG_DEFAULT { \
    .nsub = 8, \
    .nbits = 8, \
    .max_iter = 20 \
}

/* 便捷创建索引宏 */
#define JW_INDEX_CREATE_IVF(arena, dim, nlist) \
    jw_index_create((arena), &(jw_index_config_t){ \
        .type = JW_INDEX_IVF, \
        .metric = JW_METRIC_L2, \
        .dim = (dim), \
        .params.ivf = { .nlist = (nlist), .nprobe = 16 } \
    })

#define JW_INDEX_CREATE_HNSW(arena, dim, M) \
    jw_index_create((arena), &(jw_index_config_t){ \
        .type = JW_INDEX_HNSW, \
        .metric = JW_METRIC_L2, \
        .dim = (dim), \
        .params.hnsw = { .M = (M), .ef_construction = 200 } \
    })

JW_END_DECL

#endif /* JW_INDEX_H */
