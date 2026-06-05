/*
 * jw_collection.h - JinWo VecDB 向量集合
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
 * 向量集合说明:
 * 
 * Collection是向量数据库的核心概念，类似于关系数据库中的表。
 * 每个Collection包含:
 *   - 向量数据存储
 *   - 索引结构
 *   - 元数据管理
 *   - 配置参数
 * 
 * 设计理念:
 *   - 类似MongoDB的文档集合概念
 *   - 支持动态添加/删除向量
 *   - 支持多种索引类型
 *   - 支持元数据过滤
 * 
 * 版本: 0.1.32
 * 作者: 灵活就业码农
 */

#ifndef JW_COLLECTION_H
#define JW_COLLECTION_H

#include "jw_types.h"
#include "jw_arena.h"
#include "jw_vector.h"
#include "jw_index.h"
#include "jw_lock.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 元数据定义
 * =============================================================================
 */

/**
 * 元数据字段类型
 */
typedef enum jw_meta_type {
    JW_META_NONE = 0,
    JW_META_INT32,              /* 32位整数 */
    JW_META_INT64,              /* 64位整数 */
    JW_META_FLOAT,              /* 单精度浮点 */
    JW_META_DOUBLE,             /* 双精度浮点 */
    JW_META_STRING,             /* 字符串 */
    JW_META_BOOL,               /* 布尔值 */
    JW_META_JSON,               /* JSON字符串 */
} jw_meta_type_t;

/**
 * 元数据字段值 (联合体)
 */
typedef union jw_meta_value {
    jw_int32_t i32;
    jw_int64_t i64;
    jw_float32_t f32;
    jw_float64_t f64;
    jw_str_t str;
    jw_bool_t b;
} jw_meta_value_t;

/**
 * 元数据字段
 */
typedef struct jw_meta_field {
    jw_str_t name;              /* 字段名 */
    jw_meta_type_t type;        /* 字段类型 */
    jw_meta_value_t value;      /* 字段值 */
} jw_meta_field_t;

/**
 * 向量记录 (包含向量和元数据)
 */
typedef struct jw_record {
    jw_vid_t vid;               /* 向量ID */
    jw_vec_t vec;               /* 向量数据 */
    jw_meta_field_t *fields;    /* 元数据字段数组 */
    jw_size_t field_count;      /* 字段数量 */
} jw_record_t;

/*
 * =============================================================================
 * 过滤条件
 * =============================================================================
 */

/**
 * 比较操作符
 */
typedef enum jw_compare_op {
    JW_CMP_EQ = 0,              /* 等于 */
    JW_CMP_NE,                  /* 不等于 */
    JW_CMP_GT,                  /* 大于 */
    JW_CMP_GE,                  /* 大于等于 */
    JW_CMP_LT,                  /* 小于 */
    JW_CMP_LE,                  /* 小于等于 */
    JW_CMP_IN,                  /* 在列表中 */
    JW_CMP_NOT_IN,              /* 不在列表中 */
    JW_CMP_LIKE,                /* 模糊匹配 */
    JW_CMP_REGEX,               /* 正则匹配 */
} jw_compare_op_t;

/**
 * 逻辑操作符
 */
typedef enum jw_logic_op {
    JW_LOGIC_AND = 0,
    JW_LOGIC_OR,
    JW_LOGIC_NOT,
} jw_logic_op_t;

/**
 * 过滤条件 (支持嵌套)
 */
typedef struct jw_filter {
    jw_logic_op_t logic;        /* 逻辑操作符 */
    
    /* 简单条件 */
    const char *field;          /* 字段名 */
    jw_compare_op_t cmp;        /* 比较操作符 */
    jw_meta_value_t value;      /* 比较值 */
    
    /* 嵌套条件 */
    struct jw_filter **children; /* 子条件数组 */
    jw_size_t child_count;       /* 子条件数量 */
} jw_filter;

/*
 * =============================================================================
 * Collection配置
 * =============================================================================
 */

/* Collection配置参数 - 已在 jw_types.h 中定义 */

/*
 * =============================================================================
 * Collection结构
 * =============================================================================
 */

/**
 * 向量集合结构体
 */
typedef struct jw_collection_t {
    /* 基本信息 */
    char *name;                 /* 集合名称 */
    jw_dim_t dim;               /* 向量维度 */
    jw_metric_t metric;    /* 距离度量 */
    
    /* 向量存储 */
    jw_record_t *records;       /* 记录数组 */
    jw_size_t count;            /* 当前记录数 */
    jw_size_t capacity;         /* 容量 */
    jw_vid_t next_vid;          /* 下一个可用的ID */
    
    /* 索引 */
    jw_index_t *index;          /* 索引指针 */
    jw_bool_t index_enabled;    /* 是否启用索引 */
    jw_size_t index_threshold;  /* 建索引阈值 */
    
    /* 元数据 */
    jw_meta_field_t *schema;    /* 元数据模式 */
    jw_size_t schema_count;     /* 模式字段数 */
    
    /* 配置 */
    jw_collection_config_t config;
    
    /* 内存管理 */
    jw_arena_t *arena;
    jw_bool_t owns_arena;       /* 是否拥有arena（外部传入的arena不可销毁） */
    
    /* 并发控制 */
    jw_rwlock_t *lock;
    
    /* 状态 */
    jw_bool_t created;
    jw_uint64_t create_time;
    jw_uint64_t update_time;
} jw_collection_t;

/*
 * =============================================================================
 * Collection API
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 * Collection生命周期
 * -----------------------------------------------------------------------------
 */

/**
 * 创建Collection
 * 
 * @param arena   内存池
 * @param config 配置参数
 * @return Collection指针，失败返回NULL
 */
JW_API jw_collection_t *jw_collection_create(jw_arena_t *arena,
                                              const jw_collection_config_t *config);

/**
 * 销毁Collection
 * 
 * @param coll Collection指针
 */
JW_API void jw_collection_destroy(jw_collection_t *coll);

/**
 * 获取Collection名称
 * 
 * @param coll Collection指针
 * @return 集合名称
 */
JW_API const char *jw_collection_get_name(const jw_collection_t *coll);

/**
 * 获取Collection统计信息
 */
typedef struct jw_collection_stats {
    jw_size_t count;            /* 向量数量 */
    jw_size_t capacity;         /* 容量 */
    jw_size_t memory_used;      /* 内存使用量 */
    jw_dim_t dim;               /* 向量维度 */
    jw_index_type_t index_type; /* 索引类型 */
    jw_bool_t index_ready;      /* 索引是否就绪 */
} jw_collection_stats_t;

JW_API jw_status_t jw_collection_get_stats(const jw_collection_t *coll,
                                            jw_collection_stats_t *stats);

/*
 * -----------------------------------------------------------------------------
 * 向量插入
 * -----------------------------------------------------------------------------
 */

/**
 * 插入单个向量
 * 
 * @param coll   Collection指针
 * @param vec    向量数据
 * @param vid    输出向量ID (可为NULL)
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_collection_insert(jw_collection_t *coll,
                                         jw_cvec_t vec,
                                         jw_vid_t *vid);

/**
 * 插入向量带元数据
 * 
 * @param coll        Collection指针
 * @param vec         向量数据
 * @param fields      元数据字段数组
 * @param field_count 字段数量
 * @param vid         输出向量ID
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_insert_with_meta(jw_collection_t *coll,
                                                   jw_cvec_t vec,
                                                   const jw_meta_field_t *fields,
                                                   jw_size_t field_count,
                                                   jw_vid_t *vid);

/**
 * 插入向量记录
 * 
 * @param coll   Collection指针
 * @param record 向量记录
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_insert_record(jw_collection_t *coll,
                                                const jw_record_t *record);

/**
 * 批量插入向量
 * 
 * @param coll    Collection指针
 * @param vectors 向量数组
 * @param count   向量数量
 * @param vids    输出ID数组 (可为NULL)
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_insert_batch(jw_collection_t *coll,
                                               jw_cvec_t vectors,
                                               jw_size_t count,
                                               jw_vid_t *vids);

/**
 * 插入或更新向量
 * 
 * @param coll Collection指针
 * @param vid  向量ID
 * @param vec  向量数据
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_upsert(jw_collection_t *coll,
                                         jw_vid_t vid,
                                         jw_cvec_t vec);

/*
 * -----------------------------------------------------------------------------
 * 向量删除
 * -----------------------------------------------------------------------------
 */

/**
 * 删除向量
 * 
 * @param coll Collection指针
 * @param vid  向量ID
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_delete(jw_collection_t *coll, jw_vid_t vid);

/**
 * 批量删除向量
 * 
 * @param coll  Collection指针
 * @param vids  向量ID数组
 * @param count 向量数量
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_delete_batch(jw_collection_t *coll,
                                               const jw_vid_t *vids,
                                               jw_size_t count);

/**
 * 根据过滤条件删除向量
 * 
 * @param coll   Collection指针
 * @param filter 过滤条件
 * @return 删除的向量数量，-1表示失败
 */
JW_API jw_ssize_t jw_collection_delete_by_filter(jw_collection_t *coll,
                                                  const jw_filter *filter);

/**
 * 清空Collection
 * 
 * @param coll Collection指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_clear(jw_collection_t *coll);

/*
 * -----------------------------------------------------------------------------
 * 向量查询
 * -----------------------------------------------------------------------------
 */

/**
 * 根据ID获取向量
 * 
 * @param coll Collection指针
 * @param vid  向量ID
 * @param vec  输出向量 (调用者分配)
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_get(const jw_collection_t *coll,
                                      jw_vid_t vid,
                                      jw_vec_t vec);

/**
 * 根据ID获取完整记录
 * 
 * @param coll    Collection指针
 * @param vid     向量ID
 * @param record  输出记录指针 (内部指针，不释放)
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_get_record(const jw_collection_t *coll,
                                             jw_vid_t vid,
                                             const jw_record_t **record);

/**
 * 批量获取向量
 * 
 * @param coll    Collection指针
 * @param vids    向量ID数组
 * @param count   向量数量
 * @param vectors 输出向量数组
 * @return 成功获取的数量
 */
JW_API jw_size_t jw_collection_get_batch(const jw_collection_t *coll,
                                          const jw_vid_t *vids,
                                          jw_size_t count,
                                          jw_vec_t vectors);

/*
 * -----------------------------------------------------------------------------
 * 向量搜索
 * -----------------------------------------------------------------------------
 */

/**
 * 搜索选项
 */
typedef struct jw_search_options {
    jw_size_t k;                /* 返回结果数 */
    const jw_filter *filter;  /* 过滤条件 */
    jw_bool_t include_vectors;  /* 是否返回向量数据 */
    jw_bool_t include_meta;     /* 是否返回元数据 */
    jw_uint32_t nprobe;         /* IVF探测数 */
    jw_uint32_t ef_search;      /* HNSW搜索宽度 */
} jw_search_options_t;

/**
 * 搜索结果 (扩展版)
 */
typedef struct jw_search_result_ex {
    jw_vid_t vid;               /* 向量ID */
    jw_score_t score;           /* 分数 */
    jw_vec_t vec;               /* 向量数据 (可选) */
    jw_meta_field_t *fields;    /* 元数据 (可选) */
    jw_size_t field_count;      /* 元数据字段数 */
} jw_search_result_ex_t;

/**
 * 向量相似度搜索
 * 
 * @param coll    Collection指针
 * @param query   查询向量
 * @param options 搜索选项
 * @param results 结果数组
 * @return 实际结果数量
 */
JW_API jw_size_t jw_collection_search(const jw_collection_t *coll,
                                       jw_cvec_t query,
                                       const jw_search_options_t *options,
                                       jw_search_result_ex_t *results);

/**
 * 按ID搜索相似向量
 * 
 * @param coll    Collection指针
 * @param vid     参考向量ID
 * @param options 搜索选项
 * @param results 结果数组
 * @return 实际结果数量
 */
JW_API jw_size_t jw_collection_search_by_id(const jw_collection_t *coll,
                                             jw_vid_t vid,
                                             const jw_search_options_t *options,
                                             jw_search_result_ex_t *results);

/**
 * 带过滤条件的搜索
 * 
 * @param coll    Collection指针
 * @param query   查询向量
 * @param filter  过滤条件
 * @param k       返回结果数
 * @param results 结果数组
 * @return 实际结果数量
 */
JW_API jw_size_t jw_collection_search_filtered(const jw_collection_t *coll,
                                                jw_cvec_t query,
                                                const jw_filter *filter,
                                                jw_size_t k,
                                                jw_search_result_ex_t *results);

/*
 * -----------------------------------------------------------------------------
 * 索引管理
 * -----------------------------------------------------------------------------
 */

/**
 * 构建索引
 * 
 * @param coll Collection指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_build_index(jw_collection_t *coll);

/**
 * 重建索引
 * 
 * @param coll Collection指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_rebuild_index(jw_collection_t *coll);

/**
 * 删除索引
 * 
 * @param coll Collection指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_drop_index(jw_collection_t *coll);

/**
 * 检查索引状态
 * 
 * @param coll Collection指针
 * @return JW_TRUE 索引就绪
 */
JW_API jw_bool_t jw_collection_has_index(const jw_collection_t *coll);

/**
 * 更新索引配置
 * 
 * @param coll         Collection指针
 * @param index_config 新的索引配置
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_update_index(jw_collection_t *coll,
                                               const jw_index_config_t *index_config);

/*
 * -----------------------------------------------------------------------------
 * 元数据管理
 * -----------------------------------------------------------------------------
 */

/**
 * 设置向量元数据
 * 
 * @param coll        Collection指针
 * @param vid         向量ID
 * @param fields      元数据字段数组
 * @param field_count 字段数量
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_set_meta(jw_collection_t *coll,
                                           jw_vid_t vid,
                                           const jw_meta_field_t *fields,
                                           jw_size_t field_count);

/**
 * 获取向量元数据
 * 
 * @param coll        Collection指针
 * @param vid         向量ID
 * @param fields      输出字段数组
 * @param field_count 字段数量
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_get_meta(const jw_collection_t *coll,
                                           jw_vid_t vid,
                                           jw_meta_field_t *fields,
                                           jw_size_t *field_count);

/**
 * 删除向量元数据字段
 * 
 * @param coll       Collection指针
 * @param vid        向量ID
 * @param field_name 字段名
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_delete_meta_field(jw_collection_t *coll,
                                                    jw_vid_t vid,
                                                    const char *field_name);

/*
 * ----------------------------------------------------------------------------- * 迭代器
 * ----------------------------------------------------------------------------- */

/**
 * 创建迭代器
 * 
 * @param coll   Collection指针
 * @param filter 过滤条件 (可选)
 * @return 迭代器指针
 */
JW_API jw_iterator_t *jw_collection_create_iterator(jw_collection_t *coll, 
                                                   const jw_filter *filter);

/**
 * 销毁迭代器
 * 
 * @param iter 迭代器指针
 */
JW_API void jw_collection_destroy_iterator(jw_iterator_t *iter);

/**
 * 检查迭代器是否有效
 * 
 * @param iter 迭代器指针
 * @return JW_TRUE 有效
 */
JW_API jw_bool_t jw_collection_iterator_valid(const jw_iterator_t *iter);

/**
 * 移动到下一个元素
 * 
 * @param iter 迭代器指针
 * @return JW_TRUE 成功
 */
JW_API jw_bool_t jw_collection_iterator_next(jw_iterator_t *iter);

/**
 * 获取当前元素的ID
 * 
 * @param iter 迭代器指针
 * @return 向量ID
 */
JW_API jw_vid_t jw_collection_iterator_get_id(const jw_iterator_t *iter);

/**
 * 获取当前元素的向量
 * 
 * @param iter 迭代器指针
 * @return 向量数据
 */
JW_API jw_cvec_t jw_collection_iterator_get_vector(const jw_iterator_t *iter);

/**
 * 获取当前元素的记录
 * 
 * @param iter   迭代器指针
 * @param record 输出记录指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_iterator_get_record(const jw_iterator_t *iter, 
                                                    const jw_record_t **record);

/**
 * 遍历所有元素
 * 
 * @param coll      Collection指针
 * @param filter    过滤条件 (可选)
 * @param callback  回调函数
 * @param user_data 用户数据
 * @return 遍历的元素数量
 */
JW_API jw_size_t jw_collection_foreach(jw_collection_t *coll, 
                                       const jw_filter *filter, 
                                       jw_iterator_callback_t callback, 
                                       void *user_data);

/*
 * ----------------------------------------------------------------------------- * 持久化
 * ----------------------------------------------------------------------------- */

/**
 * 保存Collection到文件
 * 
 * @param coll     Collection指针
 * @param filepath 文件路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_collection_save(const jw_collection_t *coll,
                                       const char *filepath);

/**
 * 从文件加载Collection
 * 
 * @param arena     内存池
 * @param filepath 文件路径
 * @return Collection指针
 */
JW_API jw_collection_t *jw_collection_load(jw_arena_t *arena,
                                            const char *filepath);

/*
 * =============================================================================
 * 过滤条件构建辅助宏
 * =============================================================================
 */

/* 创建简单过滤条件 */
#define JW_FILTER_EQ(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_EQ, .value = { .str = (val) } }

#define JW_FILTER_NE(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_NE, .value = { .str = (val) } }

#define JW_FILTER_GT(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_GT, .value = { .i64 = (val) } }

#define JW_FILTER_GE(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_GE, .value = { .i64 = (val) } }

#define JW_FILTER_LT(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_LT, .value = { .i64 = (val) } }

#define JW_FILTER_LE(field, val) \
    &(jw_filter){ .field = (field), .cmp = JW_CMP_LE, .value = { .i64 = (val) } }

/* 创建AND过滤条件 */
#define JW_FILTER_AND(...) \
    &(jw_filter){ \
        .logic = JW_LOGIC_AND, \
        .children = (jw_filter*[]){ __VA_ARGS__ }, \
        .child_count = sizeof((jw_filter*[]){ __VA_ARGS__ }) / sizeof(jw_filter*) \
    }

/* 创建OR过滤条件 */
#define JW_FILTER_OR(...) \
    &(jw_filter){ \
        .logic = JW_LOGIC_OR, \
        .children = (jw_filter*[]){ __VA_ARGS__ }, \
        .child_count = sizeof((jw_filter*[]){ __VA_ARGS__ }) / sizeof(jw_filter*) \
    }

/*
 * =============================================================================
 * 默认配置宏
 * =============================================================================
 */

#define JW_COLLECTION_CONFIG_DEFAULT { \
    .name = {0}, \
    .dimension = 128, \
    .metric = JW_METRIC_L2, \
    .index_type = JW_INDEX_HNSW, \
    .capacity = 1000, \
    .auto_resize = JW_TRUE \
}

JW_END_DECL

#endif /* JW_COLLECTION_H */
