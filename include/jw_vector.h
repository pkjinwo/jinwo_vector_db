/*
 * jw_vector.h - JinWo VecDB 向量操作
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
 * 向量操作说明:
 * 
 * 本文件提供向量数据库所需的核心向量运算功能:
 *   1. 向量创建、复制、销毁
 *   2. 向量算术运算 (加减乘除、点积等)
 *   3. 距离计算 (L2、内积、余弦)
 *   4. 向量归一化
 *   5. 向量量化 (可选)
 * 
 * 性能优化:
 *   - 支持SIMD加速 (SSE/AVX/NEON)
 *   - 批量处理接口
 *   - 内存池友好设计
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_VECTOR_H
#define JW_VECTOR_H

#include "jw_types.h"
#include "jw_math.h"
#include "jw_arena.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 向量基础操作
 * =============================================================================
 */

/**
 * 创建向量
 * 
 * @param arena 内存池 (NULL则使用malloc)
 * @param dim  向量维度
 * @return 向量指针，失败返回NULL
 */
JW_API jw_vec_t jw_vec_create(jw_arena_t *arena, jw_dim_t dim);

/**
 * 创建并初始化向量
 * 
 * @param arena  内存池
 * @param dim   向量维度
 * @param value 初始值
 * @return 向量指针
 */
JW_API jw_vec_t jw_vec_create_with_value(jw_arena_t *arena, 
                                          jw_dim_t dim, 
                                          jw_float32_t value);

/**
 * 复制向量
 * 
 * @param arena 内存池
 * @param src  源向量
 * @param dim  维度
 * @return 新向量指针
 */
JW_API jw_vec_t jw_vec_copy(jw_arena_t *arena, jw_cvec_t src, jw_dim_t dim);

/**
 * 释放向量 (仅当未使用内存池时需要)
 * 
 * @param vec 向量指针
 */
JW_API void jw_vec_free(jw_vec_t vec);

/**
 * 清零向量
 * 
 * @param vec 向量
 * @param dim 维度
 */
JW_API void jw_vec_zero(jw_vec_t vec, jw_dim_t dim);

/**
 * 填充向量
 * 
 * @param vec   向量
 * @param dim   维度
 * @param value 填充值
 */
JW_API void jw_vec_fill(jw_vec_t vec, jw_dim_t dim, jw_float32_t value);

/**
 * 复制向量数据
 * 
 * @param dst 目标向量
 * @param src 源向量
 * @param dim 维度
 */
JW_API void jw_vec_copy_data(jw_vec_t dst, jw_cvec_t src, jw_dim_t dim);

/*
 * =============================================================================
 * 向量算术运算
 * =============================================================================
 */

/**
 * 向量加法: result = a + b
 */
JW_API void jw_vec_add(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim, jw_vec_t result);

/**
 * 向量减法: result = a - b
 */
JW_API void jw_vec_sub(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim, jw_vec_t result);

/**
 * 向量数乘: result = vec * scalar
 */
JW_API void jw_vec_scale(jw_cvec_t vec, jw_dim_t dim, 
                          jw_float32_t scalar, jw_vec_t result);

/**
 * 向量数除: result = vec / scalar
 */
JW_API void jw_vec_div(jw_cvec_t vec, jw_dim_t dim, 
                        jw_float32_t scalar, jw_vec_t result);

/**
 * 向量点积 (内积)
 * 
 * dot = sum(vec[i] * other[i])
 */
JW_API jw_float32_t jw_vec_dot(jw_cvec_t vec, jw_cvec_t other, jw_dim_t dim);

/**
 * 向量外积 (仅3维)
 */
JW_API void jw_vec_cross(jw_cvec_t a, jw_cvec_t b, jw_vec_t result);

/*
 * =============================================================================
 * 向量范数和距离计算
 * =============================================================================
 */

/**
 * 计算向量L2范数 (欧几里得范数)
 * 
 * ||v||_2 = sqrt(sum(v[i]^2))
 */
JW_API jw_float32_t jw_vec_norm_l2(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算向量L1范数 (曼哈顿范数)
 * 
 * ||v||_1 = sum(|v[i]|)
 */
JW_API jw_float32_t jw_vec_norm_l1(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算向量L2范数的平方 (避免sqrt，用于比较)
 */
JW_API jw_float32_t jw_vec_norm_l2_squared(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算L2距离 (欧氏距离)
 * 
 * d(a, b) = ||a - b||_2 = sqrt(sum((a[i] - b[i])^2))
 */
JW_API jw_float32_t jw_vec_distance_l2(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 计算L2距离的平方 (避免sqrt，用于比较)
 */
JW_API jw_float32_t jw_vec_l2_squared(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 计算L1距离 (曼哈顿距离)
 * 
 * d(a, b) = sum(|a[i] - b[i]|)
 */
JW_API jw_float32_t jw_vec_distance_l1(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 计算内积距离 (负内积，用于最小化)
 * 
 * distance = -dot(a, b)
 * 注意: 对于归一化向量，这与余弦距离等价
 */
JW_API jw_float32_t jw_vec_distance_ip(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 计算余弦相似度
 * 
 * cos(a, b) = dot(a, b) / (||a|| * ||b||)
 * 返回值范围: [-1, 1]，值越大越相似
 */
JW_API jw_float32_t jw_vec_cosine_similarity(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 计算余弦距离
 * 
 * d(a, b) = 1 - cos(a, b)
 * 返回值范围: [0, 2]，值越小越相似
 */
JW_API jw_float32_t jw_vec_cosine_distance(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);

/**
 * 通用距离计算函数
 * 
 * @param a      向量a
 * @param b      向量b
 * @param dim    维度
 * @param metric 距离度量类型
 * @return 距离值
 */
JW_API jw_float32_t jw_vec_distance(jw_cvec_t a, jw_cvec_t b, 
                                     jw_dim_t dim, 
                                     jw_metric_t metric);

/*
 * =============================================================================
 * 向量归一化
 * =============================================================================
 */

/**
 * L2归一化 (使向量长度为1)
 * 
 * vec = vec / ||vec||_2
 * 
 * @param vec 向量 (原地修改)
 * @param dim 维度
 * @return JW_SUCCESS 成功，JW_INVALID_PARAM 如果向量为零向量
 */
JW_API jw_status_t jw_vec_normalize_l2(jw_vec_t vec, jw_dim_t dim);
JW_API jw_status_t jw_vec_normalize(jw_vec_t vec, jw_dim_t dim);

/**
 * L1归一化 (使向量元素绝对值之和为1)
 * 
 * vec = vec / ||vec||_1
 */
JW_API jw_status_t jw_vec_normalize_l1(jw_vec_t vec, jw_dim_t dim);

/**
 * 归一化向量并返回新向量
 * 
 * @param arena 内存池
 * @param vec  源向量
 * @param dim  维度
 * @return 归一化后的新向量
 */
JW_API jw_vec_t jw_vec_normalized(jw_arena_t *arena, jw_cvec_t vec, jw_dim_t dim);

/**
 * 检查向量是否已归一化
 * 
 * @param vec      向量
 * @param dim      维度
 * @param epsilon  容差 (默认1e-6)
 * @return JW_TRUE 已归一化
 */
JW_API jw_bool_t jw_vec_is_normalized(jw_cvec_t vec, jw_dim_t dim, 
                                        jw_float32_t epsilon);

/*
 * =============================================================================
 * 向量统计
 * =============================================================================
 */

/**
 * 计算向量均值
 */
JW_API jw_float32_t jw_vec_mean(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算向量方差
 */
JW_API jw_float32_t jw_vec_variance(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算向量标准差
 */
JW_API jw_float32_t jw_vec_stddev(jw_cvec_t vec, jw_dim_t dim);

/**
 * 找到向量中的最大值
 */
JW_API jw_float32_t jw_vec_max(jw_cvec_t vec, jw_dim_t dim);

/**
 * 找到向量中的最小值
 */
JW_API jw_float32_t jw_vec_min(jw_cvec_t vec, jw_dim_t dim);

/**
 * 找到向量中最大值的索引
 */
JW_API jw_dim_t jw_vec_argmax(jw_cvec_t vec, jw_dim_t dim);

/**
 * 找到向量中最小值的索引
 */
JW_API jw_dim_t jw_vec_argmin(jw_cvec_t vec, jw_dim_t dim);

/**
 * 计算向量元素之和
 */
JW_API jw_float32_t jw_vec_sum(jw_cvec_t vec, jw_dim_t dim);

/*
 * =============================================================================
 * 批量向量操作 (高性能)
 * =============================================================================
 */

/**
 * 批量向量结构
 */
typedef struct jw_vec_batch {
    jw_vec_t   *vectors;      /* 向量数组 */
    jw_dim_t    dimension;    /* 向量维度 */
    jw_size_t   count;        /* 向量数量 */
    jw_size_t   capacity;     /* 容量 */
} jw_vec_batch_t;

/**
 * 创建批量向量
 */
JW_API jw_status_t jw_vec_batch_create(jw_arena_t *arena,
                                        jw_dim_t dimension,
                                        jw_size_t count,
                                        jw_vec_batch_t *batch);

/**
 * 批量计算L2距离
 * 
 * @param query    查询向量
 * @param batch    批量向量
 * @param results  输出距离数组 (需预分配count个元素)
 */
JW_API void jw_vec_batch_distance_l2(jw_cvec_t query,
                                      const jw_vec_batch_t *batch,
                                      jw_float32_t *results);

/**
 * 批量计算余弦相似度
 */
JW_API void jw_vec_batch_cosine_similarity(jw_cvec_t query,
                                            const jw_vec_batch_t *batch,
                                            jw_float32_t *results);

/**
 * 批量归一化
 */
JW_API void jw_vec_batch_normalize(jw_vec_batch_t *batch);

/*
 * =============================================================================
 * 向量量化 (可选功能)
 * =============================================================================
 */

/* 量化配置 */
typedef struct jw_quant_config {
    jw_quant_type_t type;       /* 量化类型 */
    jw_float32_t    scale;      /* 量化缩放因子 */
    jw_float32_t    min_val;    /* 最小值 (用于对称量化) */
    jw_float32_t    max_val;    /* 最大值 */
} jw_quant_config_t;

/**
 * 量化向量到int8
 * 
 * @param vec    输入向量
 * @param dim    维度
 * @param output 输出int8数组
 * @param scale  输出缩放因子
 */
JW_API void jw_vec_quantize_int8(jw_cvec_t vec, jw_dim_t dim,
                                  jw_int8_t *output, jw_float32_t *scale);

/**
 * 从int8反量化
 */
JW_API void jw_vec_dequantize_int8(const jw_int8_t *input, jw_dim_t dim,
                                    jw_float32_t scale, jw_vec_t output);

/**
 * 计算int8量化向量的点积
 */
JW_API jw_int32_t jw_vec_dot_int8(const jw_int8_t *a, 
                                   const jw_int8_t *b, 
                                   jw_dim_t dim);

/*
 * =============================================================================
 * SIMD加速支持
 * =============================================================================
 */

/* SIMD支持检测 */
typedef enum jw_simd_level {
    JW_SIMD_NONE = 0,
    JW_SIMD_SSE2,
    JW_SIMD_SSE4_1,
    JW_SIMD_AVX,
    JW_SIMD_AVX2,
    JW_SIMD_AVX512,
    JW_SIMD_NEON              /* ARM NEON */
} jw_simd_level_t;

/**
 * 检测当前CPU支持的SIMD级别
 */
JW_API jw_simd_level_t jw_simd_detect(void);

/**
 * 获取SIMD级别名称
 */
JW_API const char* jw_simd_level_name(jw_simd_level_t level);

/**
 * 启用/禁用SIMD加速
 */
JW_API void jw_simd_set_enabled(jw_bool_t enabled);

/**
 * 检查SIMD是否启用
 */
JW_API jw_bool_t jw_simd_is_enabled(void);

/*
 * =============================================================================
 * 工具函数
 * =============================================================================
 */

/**
 * 打印向量内容 (调试用)
 */
JW_API void jw_vec_print(jw_cvec_t vec, jw_dim_t dim, const char *name);

/**
 * 验证向量数据有效性
 * 
 * 检查是否包含NaN或Inf
 */
JW_API jw_bool_t jw_vec_is_valid(jw_cvec_t vec, jw_dim_t dim);

/**
 * 比较两个向量是否相等
 */
JW_API jw_bool_t jw_vec_equal(jw_cvec_t a, jw_cvec_t b, 
                               jw_dim_t dim, 
                               jw_float32_t epsilon);

/**
 * 生成随机向量
 */
JW_API void jw_vec_random(jw_vec_t vec, jw_dim_t dim);

/**
 * 生成随机归一化向量
 */
JW_API void jw_vec_random_normalized(jw_vec_t vec, jw_dim_t dim);

/*
 * =============================================================================
 * 内联优化版本 (高频使用场景)
 * =============================================================================
 */

/* 内联L2距离计算 */
JW_FORCE_INLINE jw_float32_t jw_vec_distance_l2_inline(
    jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        jw_float32_t diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum; /* 返回平方距离，避免sqrt */
}

/* 内联点积计算 */
JW_FORCE_INLINE jw_float32_t jw_vec_dot_inline(
    jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

/* 内联L2归一化 */
JW_FORCE_INLINE void jw_vec_normalize_l2_inline(jw_vec_t vec, jw_dim_t dim)
{
    jw_float32_t norm = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        norm += vec[i] * vec[i];
    }
    if (norm > 0.0f) {
        norm = 1.0f / jw_math_sqrt_f32(norm);
        for (jw_dim_t i = 0; i < dim; i++) {
            vec[i] *= norm;
        }
    }
}

JW_FORCE_INLINE jw_float32_t jw_vec_distance_l2_squared_inline(
    jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        jw_float32_t diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

JW_FORCE_INLINE jw_float32_t jw_vec_distance_ip_inline(
    jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        sum += a[i] * b[i];
    }
    return -sum;
}

JW_FORCE_INLINE jw_float32_t jw_vec_norm_l2_inline(
    jw_cvec_t vec, jw_dim_t dim)
{
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        sum += vec[i] * vec[i];
    }
    return jw_math_sqrt_f32(sum);
}

JW_END_DECL

#endif /* JW_VECTOR_H */

/*
 * =============================================================================
 * 使用示例
 * =============================================================================
 * 
 * // 示例1: 基本向量操作
 * jw_arena_t *arena;
 * jw_vec_batch_create(NULL, &arena);
 * 
 * // 创建向量
 * jw_dim_t dim = 128;
 * jw_vec_t vec1 = jw_vec_create(arena, dim);
 * jw_vec_t vec2 = jw_vec_create(arena, dim);
 * 
 * // 填充数据
 * jw_vec_fill(vec1, dim, 1.0f);
 * jw_vec_random_normalized(vec2, dim);
 * 
 * // 计算距离
 * jw_float32_t l2_dist = jw_vec_distance_l2(vec1, vec2, dim);
 * jw_float32_t cos_sim = jw_vec_cosine_similarity(vec1, vec2, dim);
 * 
 * // 归一化
 * jw_vec_normalize_l2(vec1, dim);
 * 
 * 
 * // 示例2: 批量搜索
 * jw_vec_batch_t batch;
 * jw_vec_batch_create(arena, 128, 1000, &batch);
 * 
 * // 填充向量...
 * 
 * // 批量计算距离
 * jw_float32_t *distances = JW_POOL_ALLOC_ARRAY(arena, jw_float32_t, batch.count);
 * jw_vec_batch_distance_l2(query_vec, &batch, distances);
 * 
 * 
 * // 示例3: 使用内联版本优化性能
 * for (int i = 0; i < n; i++) {
 *     // 使用内联版本避免函数调用开销
 *     float dist_sq = jw_vec_distance_l2_inline(query, vectors[i], dim);
 *     if (dist_sq < min_dist) {
 *         min_dist = dist_sq;
 *         best_idx = i;
 *     }
 * }
 * 
 * 
 * // 示例4: SIMD加速
 * jw_simd_level_t level = jw_simd_detect();
 * printf("SIMD support: %s\n", jw_simd_level_name(level));
 * // 库函数会自动使用SIMD加速
 */
