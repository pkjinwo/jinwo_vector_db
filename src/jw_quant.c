/*
 * jw_quant.c - JinWo VecDB 向量量化实现
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
 * 量化实现详解:
 * 
 * 本文件实现了两种向量量化方法:
 * 
 * 1. SQ (Scalar Quantization) - 标量量化
 *    原理: 将每个维度独立量化到8位整数
 *    压缩比: 4x (float32 -> uint8)
 *    精度损失: 约0.1%-0.5%
 * 
 * 2. PQ (Product Quantization) - 乘积量化
 *    原理: 将向量分成多个子向量，每个子向量独立量化
 *    压缩比: 8-32x (取决于子向量数量)
 *    精度损失: 约1%-5%
 * 
 * 量化的核心是在内存使用和查询精度之间找到平衡。
 * 
 * =============================================================================
 */

#include "jw_quant.h"
#include "jw_math.h"
#include "jw_vector.h"
#include <stdio.h>
#include <string.h>

/* PQ 码本 K-means 迭代次数 (Lloyd 算法) */
#define JW_PQ_KMEANS_ITERS 25

/*
 * =============================================================================
 * SQ (标量量化) 实现
 * =============================================================================
 */

/**
 * 创建 SQ 量化器
 */
JW_API jw_sq_quantizer_t *jw_sq_quantizer_create(jw_arena_t *arena, jw_dim_t dim)
{
    if (arena == NULL || dim == 0) {
        return NULL;
    }
    
    jw_sq_quantizer_t *quant = (jw_sq_quantizer_t *)jw_arena_calloc(arena, 1, sizeof(jw_sq_quantizer_t));
    if (quant == NULL) {
        return NULL;
    }
    
    quant->dim = dim;
    quant->arena = arena;
    
    /* 分配mins、maxs和scales数组 */
    quant->mins = (jw_float32_t *)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    quant->maxs = (jw_float32_t *)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    quant->scales = (jw_float32_t *)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    
    if (quant->mins == NULL || quant->maxs == NULL || quant->scales == NULL) {
        return NULL;
    }
    
    return quant;
}

/**
 * 训练量化器（计算 mins/maxs）
 */
JW_API jw_status_t jw_sq_quantizer_train(
    jw_sq_quantizer_t *quant,
    const jw_float32_t *vectors,
    jw_size_t count
)
{
    if (quant == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    /* 初始化：第一个向量作为初始值 */
    for (jw_dim_t d = 0; d < dim; d++) {
        quant->mins[d] = vectors[d];
        quant->maxs[d] = vectors[d];
    }
    
    /* 遍历所有向量，更新 min/max */
    for (jw_size_t i = 1; i < count; i++) {
        const jw_float32_t *vec = vectors + i * dim;
        for (jw_dim_t d = 0; d < dim; d++) {
            if (vec[d] < quant->mins[d]) {
                quant->mins[d] = vec[d];
            }
            if (vec[d] > quant->maxs[d]) {
                quant->maxs[d] = vec[d];
            }
        }
    }
    
    /* 计算缩放因子，避免除零 */
    for (jw_dim_t d = 0; d < dim; d++) {
        jw_float32_t range = quant->maxs[d] - quant->mins[d];
        quant->scales[d] = (range > 1e-6f) ? (255.0f / range) : 1.0f;
    }
    
    return JW_SUCCESS;
}

/**
 * 量化单个向量
 */
JW_API jw_status_t jw_sq_quantize(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst
)
{
    if (quant == NULL || src == NULL || dst == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    for (jw_dim_t d = 0; d < dim; d++) {
        jw_float32_t normalized = (src[d] - quant->mins[d]) * quant->scales[d];
        /* 限制范围 [0, 255] */
        normalized = jw_math_clamp_f32(normalized, 0.0f, 255.0f);
        dst[d] = (jw_uint8_t)(normalized + 0.5f);  /* 四舍五入 */
    }
    
    return JW_SUCCESS;
}

/**
 * 反量化单个向量
 */
JW_API jw_status_t jw_sq_dequantize(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst
)
{
    if (quant == NULL || src == NULL || dst == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    for (jw_dim_t d = 0; d < dim; d++) {
        dst[d] = quant->mins[d] + src[d] / quant->scales[d];
    }
    
    return JW_SUCCESS;
}

/**
 * 批量量化
 */
JW_API jw_status_t jw_sq_quantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_float32_t *src,
    jw_uint8_t *dst,
    jw_size_t count
)
{
    if (quant == NULL || src == NULL || dst == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    for (jw_size_t i = 0; i < count; i++) {
        const jw_float32_t *vec = src + i * dim;
        jw_uint8_t *qvec = dst + i * dim;
        
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t normalized = (vec[d] - quant->mins[d]) * quant->scales[d];
            normalized = jw_math_clamp_f32(normalized, 0.0f, 255.0f);
            qvec[d] = (jw_uint8_t)(normalized + 0.5f);
        }
    }
    
    return JW_SUCCESS;
}

/**
 * 批量反量化
 */
JW_API jw_status_t jw_sq_dequantize_batch(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *src,
    jw_float32_t *dst,
    jw_size_t count
)
{
    if (quant == NULL || src == NULL || dst == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_dim_t dim = quant->dim;
    
    for (jw_size_t i = 0; i < count; i++) {
        const jw_uint8_t *qvec = src + i * dim;
        jw_float32_t *vec = dst + i * dim;
        
        for (jw_dim_t d = 0; d < dim; d++) {
            vec[d] = quant->mins[d] + qvec[d] / quant->scales[d];
        }
    }
    
    return JW_SUCCESS;
}

/**
 * 计算量化向量与浮点向量的距离（无需反量化）
 */
JW_API jw_float32_t jw_sq_distance(
    const jw_sq_quantizer_t *quant,
    const jw_uint8_t *qvec,
    const jw_float32_t *fvec,
    jw_metric_t metric
)
{
    if (quant == NULL || qvec == NULL || fvec == NULL) {
        return JW_FLT_MAX;
    }
    
    jw_float32_t dist = 0.0f;
    jw_dim_t dim = quant->dim;
    
    if (metric == JW_METRIC_L2) {
        /* L2 距离：sum((fvec - dequantize(qvec))^2) */
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            jw_float32_t diff = fvec[d] - restored;
            dist += diff * diff;
        }
        return jw_math_sqrt_f32(dist);
    }
    else if (metric == JW_METRIC_IP) {
        /* 内积：sum(fvec * dequantize(qvec)) */
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            dist += fvec[d] * restored;
        }
        return -dist;  /* 负号使得最大内积对应最小距离 */
    }
    else if (metric == JW_METRIC_COSINE) {
        /* 余弦相似度：需要归一化 */
        jw_float32_t dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (jw_dim_t d = 0; d < dim; d++) {
            jw_float32_t restored = quant->mins[d] + qvec[d] / quant->scales[d];
            dot += fvec[d] * restored;
            norm_a += fvec[d] * fvec[d];
            norm_b += restored * restored;
        }
        if (norm_a == 0.0f || norm_b == 0.0f) {
            return 1.0f;  /* 相似度为0 */
        }
        return 1.0f - dot / (jw_math_sqrt_f32(norm_a) * jw_math_sqrt_f32(norm_b));
    }
    
    return JW_FLT_MAX;
}

/**
 * 销毁量化器
 */
JW_API void jw_sq_quantizer_destroy(jw_sq_quantizer_t *quant)
{
    /* 内存由内存池管理，不需要显式释放 */
    (void)quant;
}

/*
 * =============================================================================
 * PQ (乘积量化) 实现
 * =============================================================================
 */

/**
 * 创建 PQ 量化器
 */
JW_API jw_status_t jw_pq_create(
    jw_arena_t *arena,
    jw_dim_t dim,
    jw_uint32_t nsub,
    jw_uint32_t nbits,
    jw_pq_quantizer_t **pq
)
{
    if (arena == NULL || dim == 0 || nsub == 0 || pq == NULL) {
        return JW_INVALID_PARAM;
    }
    
    if (dim % nsub != 0) {
        return JW_INVALID_PARAM; /* 维度必须能被子向量数量整除 */
    }
    
    jw_pq_quantizer_t *quant = (jw_pq_quantizer_t *)jw_arena_calloc(arena, 1, sizeof(jw_pq_quantizer_t));
    if (quant == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    quant->dim = dim;
    quant->nsub = nsub;
    quant->nbits = nbits;
    quant->k = 1 << nbits;
    quant->sub_dim = dim / nsub;
    quant->arena = arena;
    
    /* 分配聚类中心内存 */
    quant->centroids = (jw_vec_t *)jw_arena_alloc(arena, nsub * sizeof(jw_vec_t));
    if (quant->centroids == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    for (jw_uint32_t i = 0; i < nsub; i++) {
        quant->centroids[i] = (jw_vec_t)jw_arena_alloc(arena, quant->k * quant->sub_dim * sizeof(jw_float32_t));
        if (quant->centroids[i] == NULL) {
            return JW_OUT_OF_MEMORY;
        }
        jw_memset(quant->centroids[i], 0, quant->k * quant->sub_dim * sizeof(jw_float32_t));
    }
    
    *pq = quant;
    return JW_SUCCESS;
}

/**
 * 训练 PQ 量化器
 */
JW_API jw_status_t jw_pq_train(
    jw_pq_quantizer_t *pq,
    const jw_float32_t *vectors,
    jw_size_t count,
    jw_metric_t metric
)
{
    if (pq == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }

    pq->metric = metric;

    const jw_dim_t sd = pq->sub_dim;
    const jw_uint32_t kk = pq->k;
    const size_t vec_bytes = (size_t)count * pq->dim * sizeof(jw_float32_t);

    /* 余弦度量: 先对全向量归一化, 使码本学习"方向"而非"长度"。
       不归一化会导致码本按 L2 聚类、而检索按余弦, 召回崩塌。 */
    jw_cvec_t src = vectors;
    jw_vec_t norm_buf = NULL;
    if (metric == JW_METRIC_COSINE) {
        norm_buf = (jw_vec_t)jw_arena_alloc(pq->arena, vec_bytes);
        if (norm_buf == NULL) {
            return JW_OUT_OF_MEMORY;
        }
        jw_memcpy(norm_buf, vectors, vec_bytes);
        for (jw_size_t j = 0; j < count; j++) {
            jw_vec_t v = norm_buf + j * pq->dim;
            jw_float32_t n = jw_vec_norm_l2_inline(v, pq->dim);
            if (n > JW_FLT_EPSILON) {
                jw_vec_scale(v, pq->dim, 1.0f / n, v);
            }
        }
        src = norm_buf;
    }

    /* 每个子空间独立做 K-means (Lloyd 算法) */
    for (jw_uint32_t i = 0; i < pq->nsub; i++) {
        jw_dim_t start = i * sd;

        /* 提取当前子空间的子向量 (连续存储) */
        jw_vec_t sub_vecs = (jw_vec_t)jw_arena_alloc(pq->arena,
                              (size_t)count * sd * sizeof(jw_float32_t));
        if (sub_vecs == NULL) {
            return JW_OUT_OF_MEMORY;
        }
        for (jw_size_t j = 0; j < count; j++) {
            jw_memcpy(sub_vecs + j * sd, src + j * pq->dim + start,
                      sd * sizeof(jw_float32_t));
        }

        jw_vec_t cen = pq->centroids[i];

        /* 初始化聚类中心: 随机选取样本点 (count<k 时循环复用, 保证有 k 个中心) */
        for (jw_uint32_t k = 0; k < kk; k++) {
            jw_size_t idx = (jw_size_t)(jw_rand() % (unsigned long)count);
            jw_memcpy(cen + k * sd, sub_vecs + idx * sd, sd * sizeof(jw_float32_t));
        }

        /* Lloyd 迭代: 分配 -> 重算中心, 直到收敛或达到迭代上限 */
        jw_uint32_t *assign = (jw_uint32_t *)jw_arena_alloc(pq->arena,
                                  count * sizeof(jw_uint32_t));
        jw_float32_t *sums = (jw_float32_t *)jw_arena_alloc(pq->arena,
                                  (size_t)kk * sd * sizeof(jw_float32_t));
        jw_uint32_t *cnts = (jw_uint32_t *)jw_arena_alloc(pq->arena,
                                  kk * sizeof(jw_uint32_t));
        if (assign == NULL || sums == NULL || cnts == NULL) {
            return JW_OUT_OF_MEMORY;
        }

        for (jw_uint32_t iter = 0; iter < JW_PQ_KMEANS_ITERS; iter++) {
            int changed = 0;

            /* 步骤1: 每个子向量分配到最近的中心 (L2) */
            for (jw_size_t j = 0; j < count; j++) {
                jw_cvec_t v = sub_vecs + j * sd;
                jw_float32_t best = JW_FLT_MAX;
                jw_uint32_t best_k = 0;
                for (jw_uint32_t k = 0; k < kk; k++) {
                    jw_float32_t d = 0.0f;
                    jw_cvec_t c = cen + k * sd;
                    for (jw_dim_t d2 = 0; d2 < sd; d2++) {
                        jw_float32_t diff = v[d2] - c[d2];
                        d += diff * diff;
                    }
                    if (d < best) { best = d; best_k = k; }
                }
                if (assign[j] != best_k) { assign[j] = best_k; changed = 1; }
            }

            if (!changed) {
                break;  /* 已收敛 */
            }

            /* 步骤2: 重算中心 = 各簇均值 */
            jw_memset(sums, 0, (size_t)kk * sd * sizeof(jw_float32_t));
            jw_memset(cnts, 0, kk * sizeof(jw_uint32_t));
            for (jw_size_t j = 0; j < count; j++) {
                jw_uint32_t c = assign[j];
                cnts[c]++;
                for (jw_dim_t d2 = 0; d2 < sd; d2++) {
                    sums[c * sd + d2] += sub_vecs[j * sd + d2];
                }
            }
            for (jw_uint32_t k = 0; k < kk; k++) {
                if (cnts[k] == 0) continue;
                for (jw_dim_t d2 = 0; d2 < sd; d2++) {
                    cen[k * sd + d2] = sums[k * sd + d2] / (jw_float32_t)cnts[k];
                }
            }
        }
    }

    return JW_SUCCESS;
}

/**
 * 量化单个向量
 */
JW_API void jw_pq_encode(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *vec,
    jw_uint8_t *code
)
{
    if (pq == NULL || vec == NULL || code == NULL) {
        return;
    }

    /* 余弦: 编码前先归一化, 与训练时的预处理保持一致 */
    jw_cvec_t src = vec;
    jw_float32_t stack_norm[JW_MAX_VECTOR_DIM];
    jw_vec_t heap_norm = NULL;
    if (pq->metric == JW_METRIC_COSINE) {
        jw_float32_t n = jw_vec_norm_l2_inline(vec, pq->dim);
        if (n > JW_FLT_EPSILON) {
            if (pq->dim <= JW_MAX_VECTOR_DIM) {
                jw_memcpy(stack_norm, vec, pq->dim * sizeof(jw_float32_t));
                jw_vec_scale(stack_norm, pq->dim, 1.0f / n, stack_norm);
                src = stack_norm;
            } else {
                heap_norm = (jw_vec_t)jw_arena_alloc(pq->arena, pq->dim * sizeof(jw_float32_t));
                if (heap_norm != NULL) {
                    jw_memcpy(heap_norm, vec, pq->dim * sizeof(jw_float32_t));
                    jw_vec_scale(heap_norm, pq->dim, 1.0f / n, heap_norm);
                    src = heap_norm;
                }
            }
        }
    }

    for (jw_uint32_t i = 0; i < pq->nsub; i++) {
        jw_dim_t start = i * pq->sub_dim;
        const jw_float32_t *sub_vec = src + start;
        const jw_vec_t centroids = pq->centroids[i];
        
        /* 找到最近的聚类中心 */
        jw_float32_t min_dist = JW_FLT_MAX;
        jw_uint32_t min_idx = 0;
        
        for (jw_uint32_t j = 0; j < pq->k; j++) {
            jw_float32_t dist = 0.0f;
            for (jw_dim_t d = 0; d < pq->sub_dim; d++) {
                jw_float32_t diff = sub_vec[d] - centroids[j * pq->sub_dim + d];
                dist += diff * diff;
            }
            if (dist < min_dist) {
                min_dist = dist;
                min_idx = j;
            }
        }
        
        code[i] = (jw_uint8_t)min_idx;
    }
}

/**
 * 批量量化
 */
JW_API void jw_pq_encode_batch(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *vectors,
    jw_uint8_t *codes,
    jw_size_t count
)
{
    if (pq == NULL || vectors == NULL || codes == NULL || count == 0) {
        return;
    }
    
    for (jw_size_t i = 0; i < count; i++) {
        const jw_float32_t *vec = vectors + i * pq->dim;
        jw_uint8_t *code = codes + i * pq->nsub;
        jw_pq_encode(pq, vec, code);
    }
}

/**
 * 计算查询向量与PQ编码向量的距离
 */
JW_API jw_float32_t jw_pq_distance(
    const jw_pq_quantizer_t *pq,
    const jw_float32_t *query,
    const jw_uint8_t *code,
    jw_metric_t metric
)
{
    if (pq == NULL || query == NULL || code == NULL) {
        return JW_FLT_MAX;
    }
    
    jw_float32_t total_dist = 0.0f;
    
    for (jw_uint32_t i = 0; i < pq->nsub; i++) {
        jw_dim_t start = i * pq->sub_dim;
        const jw_float32_t *sub_query = query + start;
        jw_uint32_t centroid_idx = code[i];
        const jw_vec_t centroid = pq->centroids[i] + centroid_idx * pq->sub_dim;
        
        jw_float32_t dist = 0.0f;
        for (jw_dim_t d = 0; d < pq->sub_dim; d++) {
            jw_float32_t diff = sub_query[d] - centroid[d];
            dist += diff * diff;
        }
        total_dist += dist;
    }
    
    if (metric == JW_METRIC_L2) {
        /* 码本在归一化(COSINE)空间训练时, 查询必须先归一化再算 L2,
           否则"原始坐标"与"单位球面坐标"量纲错乱, 距离值爆炸(可达1e12量级),
           导致 IVF/PQ 检索被巨值主导、真结果被挤掉, 召回崩塌。 */
        if (pq->metric == JW_METRIC_COSINE) {
            jw_float32_t nq = jw_vec_norm_l2_inline(query, pq->dim);
            if (nq <= JW_FLT_EPSILON) {
                return JW_FLT_MAX;
            }
            jw_float32_t d = 0.0f;
            for (jw_uint32_t i = 0; i < pq->nsub; i++) {
                jw_dim_t start = i * pq->sub_dim;
                jw_uint32_t centroid_idx = code[i];
                const jw_vec_t centroid = pq->centroids[i] + centroid_idx * pq->sub_dim;
                for (jw_dim_t dd = 0; dd < pq->sub_dim; dd++) {
                    jw_float32_t diff = query[start + dd] / nq - centroid[dd];
                    d += diff * diff;
                }
            }
            return jw_math_sqrt_f32(d);
        }
        return jw_math_sqrt_f32(total_dist);
    }
    else if (metric == JW_METRIC_IP) {
        /* 内积需要重新计算 */
        jw_float32_t dot = 0.0f;
        for (jw_uint32_t i = 0; i < pq->nsub; i++) {
            jw_dim_t start = i * pq->sub_dim;
            const jw_float32_t *sub_query = query + start;
            jw_uint32_t centroid_idx = code[i];
            const jw_vec_t centroid = pq->centroids[i] + centroid_idx * pq->sub_dim;
            
            for (jw_dim_t d = 0; d < pq->sub_dim; d++) {
                dot += sub_query[d] * centroid[d];
            }
        }
        return -dot;
    }
    else if (metric == JW_METRIC_COSINE) {
        /* 余弦相似度需要归一化 */
        jw_float32_t dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (jw_uint32_t i = 0; i < pq->nsub; i++) {
            jw_dim_t start = i * pq->sub_dim;
            const jw_float32_t *sub_query = query + start;
            jw_uint32_t centroid_idx = code[i];
            const jw_vec_t centroid = pq->centroids[i] + centroid_idx * pq->sub_dim;
            
            for (jw_dim_t d = 0; d < pq->sub_dim; d++) {
                dot += sub_query[d] * centroid[d];
                norm_a += sub_query[d] * sub_query[d];
                norm_b += centroid[d] * centroid[d];
            }
        }
        if (norm_a == 0.0f || norm_b == 0.0f) {
            return 1.0f;
        }
        return 1.0f - dot / (jw_math_sqrt_f32(norm_a) * jw_math_sqrt_f32(norm_b));
    }
    
    return total_dist;
}

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
)
{
    if (pq == NULL || query == NULL || codes == NULL || distances == NULL || count == 0) {
        return;
    }
    
    for (jw_size_t i = 0; i < count; i++) {
        const jw_uint8_t *code = codes + i * pq->nsub;
        distances[i] = jw_pq_distance(pq, query, code, metric);
    }
}

/**
 * 销毁 PQ 量化器
 */
JW_API void jw_pq_destroy(jw_pq_quantizer_t *pq)
{
    /* 内存由内存池管理，不需要显式释放 */
    (void)pq;
}

/*
 * =============================================================================
 * 辅助函数
 * =============================================================================
 */

/**
 * 计算量化后的内存大小
 */
JW_API jw_size_t jw_quant_calculate_size(
    jw_quant_type_t quant_type,
    jw_dim_t dim,
    jw_size_t count
)
{
    switch (quant_type) {
        case JW_QUANT_NONE:
            return dim * count * sizeof(jw_float32_t);
        case JW_QUANT_INT8:
        case JW_QUANT_UINT8:
            return dim * count * sizeof(jw_uint8_t);
        case JW_QUANT_PQ:
            /* PQ的大小取决于子向量数量，这里假设默认nsub=8 */
            return 8 * count * sizeof(jw_uint8_t);
        default:
            return 0;
    }
}

/**
 * 获取量化器信息
 */
JW_API const char *jw_quant_get_info(
    jw_quant_type_t quant_type,
    jw_dim_t dim,
    jw_uint32_t nsub,
    jw_uint32_t nbits
)
{
    static char info[128];
    
    switch (quant_type) {
        case JW_QUANT_NONE:
            snprintf(info, sizeof(info), "No quantization: %.2f MB", 
                     (dim * sizeof(jw_float32_t)) / (1024.0f * 1024.0f));
            break;
        case JW_QUANT_INT8:
        case JW_QUANT_UINT8:
            snprintf(info, sizeof(info), "SQ quantization: %.2f MB (4x compression)", 
                     (dim * sizeof(jw_uint8_t)) / (1024.0f * 1024.0f));
            break;
        case JW_QUANT_PQ:
            snprintf(info, sizeof(info), "PQ quantization: %.2f MB (%.1fx compression)", 
                     (nsub * sizeof(jw_uint8_t)) / (1024.0f * 1024.0f),
                     (dim * sizeof(jw_float32_t)) / (float)(nsub * sizeof(jw_uint8_t)));
            break;
        default:
            strcpy(info, "Unknown quantization");
    }
    
    return info;
}
