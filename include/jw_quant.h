/*
 * jw_quant.h - JinWo VecDB 向量量化接口
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
 * 量化接口设计说明:
 * 
 * 本文件定义了向量量化的核心接口，支持多种量化方法:
 *   1. SQ (Scalar Quantization) - 标量量化
 *   2. PQ (Product Quantization) - 乘积量化
 * 
 * 量化的主要目的是减少向量存储大小，提高内存使用效率，
 * 特别适合移动端和嵌入式设备。
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_QUANT_H
#define JW_QUANT_H

#include "jw_types.h"
#include "jw_arena.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 量化类型定义
 * =============================================================================
 */
#ifndef JW_QUANT_TYPE_DEFINED
typedef enum jw_quant_type {
    JW_QUANT_NONE = 0,    /* 不量化 */
    JW_QUANT_INT8,        /* 8位整数量化 (SQ) */
    JW_QUANT_UINT8,       /* 8位无符号整数量化 (SQ) */
    JW_QUANT_PQ,          /* 乘积量化 */
} jw_quant_type_t;
#define JW_QUANT_TYPE_DEFINED
#endif

/*
 * =============================================================================
 * SQ (标量量化) 接口
 * =============================================================================
 */

/**
 * SQ 量化器结构
 */
typedef struct jw_sq_quantizer {
    jw_float32_t *mins;      /* 每个维度的最小值 [dim] */
    jw_float32_t *maxs;      /* 每个维度的最大值 [dim] */
    jw_float32_t *scales;    /* 每个维度的缩放因子 [dim] */
    jw_dim_t dim;            /* 向量维度 */
    jw_arena_t *arena;       /* 内存池 */
} jw_sq_quantizer_t;

/**
 * 创建 SQ 量化器
 * 
 * @param arena    内存池
 * @param dim     向量维度
 * @return 量化器指针，失败返回 NULL
 */
JW_API jw_sq_quantizer_t *jw_sq_quantizer_create(jw_arena_t *arena, jw_dim_t dim);

/**
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

/**
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

/**
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

/**
 * 批量量化
 */
JW_API jw_status_t jw_sq_quantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst,
    jw_size_t count
);

/**
 * 批量反量化
 */
JW_API jw_status_t jw_sq_dequantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst,
    jw_size_t count
);

/**
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
    jw_metric_t metric
);

/**
 * 销毁量化器
 */
JW_API void jw_sq_quantizer_destroy(jw_sq_quantizer_t *quant);

/*
 * =============================================================================
 * PQ (乘积量化) 接口
 * =============================================================================
 */

/**
 * PQ 量化器结构
 */
typedef struct jw_pq_quantizer {
    jw_dim_t dim;               /* 原始向量维度 */
    jw_uint32_t nsub;           /* 子向量数量 */
    jw_uint32_t nbits;          /* 每个子向量的编码位数 */
    jw_uint32_t k;              /* 每个子空间的聚类数 (2^nbits) */
    jw_vec_t *centroids;        /* 聚类中心 [nsub][k][dim/nsub] */
    jw_dim_t sub_dim;           /* 子向量维度 */
    jw_arena_t *arena;          /* 内存池 */
} jw_pq_quantizer_t;

/**
 * 创建 PQ 量化器
 * 
 * @param arena    内存池
 * @param dim     向量维度
 * @param nsub    子向量数量
 * @param nbits   编码位数
 * @param pq      输出量化器指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_pq_create(
    jw_arena_t *arena,
    jw_dim_t dim,
    jw_uint32_t nsub,
    jw_uint32_t nbits,
    jw_pq_quantizer_t **pq
);

/**
 * 训练 PQ 量化器
 * 
 * @param pq       量化器
 * @param vectors  训练向量
 * @param count    训练向量数量
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_pq_train(
    jw_pq_quantizer_t *pq,
    const jw_float32_t *vectors,
    jw_size_t count
);

/**
 * 量化单个向量
 * 
 * @param pq      量化器
 * @param vec     输入向量
 * @param code    输出编码
 */
JW_API void jw_pq_encode(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *vec,
    jw_uint8_t *code
);

/**
 * 批量量化
 */
JW_API void jw_pq_encode_batch(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *vectors,
    jw_uint8_t *codes,
    jw_size_t count
);

/**
 * 计算查询向量与PQ编码向量的距离
 * 
 * @param pq      量化器
 * @param query   查询向量
 * @param code    PQ编码
 * @param metric  距离度量类型
 * @return 距离值
 */
JW_API jw_float32_t jw_pq_distance(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *query,
    const jw_uint8_t *code,
    jw_metric_t metric
);

/**
 * 批量计算距离
 */
JW_API void jw_pq_distance_batch(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *query,
    const jw_uint8_t *codes,
    jw_float32_t *distances,
    jw_size_t count,
    jw_metric_t metric
);

/**
 * 销毁 PQ 量化器
 */
JW_API void jw_pq_destroy(jw_pq_quantizer_t *pq);

/*
 * =============================================================================
 * 辅助函数
 * =============================================================================
 */

/**
 * 计算量化后的内存大小
 * 
 * @param quant_type 量化类型
 * @param dim        向量维度
 * @param count      向量数量
 * @return 内存大小（字节）
 */
JW_API jw_size_t jw_quant_calculate_size(
    jw_quant_type_t quant_type,
    jw_dim_t dim,
    jw_size_t count
);

/**
 * 获取量化器信息
 * 
 * @param quant_type 量化类型
 * @param dim        向量维度
 * @param nsub       PQ子向量数量
 * @param nbits      编码位数
 * @return 量化信息描述
 */
JW_API const char *jw_quant_get_info(
    jw_quant_type_t quant_type,
    jw_dim_t dim,
    jw_uint32_t nsub,
    jw_uint32_t nbits
);

JW_END_DECL

#endif /* JW_QUANT_H */
