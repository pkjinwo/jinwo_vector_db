/*
 * jw_vector.c - JinWo VecDB 向量操作实现
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
 */

#include "jw_vector.h"
#include "jw_types.h"
#include "jw_string.h"
#include "jw_arena.h"
#include "jw_math.h"
#include "jw_index.h"

/* 前向声明 - 解决循环依赖 */
JW_API void jw_vec_batch_l2_distance(jw_cvec_t query,
                                      jw_cvec_t *vectors,
                                      jw_dim_t dim,
                                      jw_size_t count,
                                      jw_float32_t *distances);

/*
 * =============================================================================
 * SIMD 加速 (可选)
 * =============================================================================
 */

#if defined(JW_X86) || defined(JW_X64)
    #if defined(__SSE2__)
        #include <emmintrin.h>
        #define JW_USE_SSE2 1
    #endif
    #if defined(__AVX__)
        #include <immintrin.h>
        #define JW_USE_AVX 1
    #endif
#endif

#if defined(JW_ARM) || defined(JW_ARM64)
    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
        #include <arm_neon.h>
        #define JW_USE_NEON 1
    #endif
#endif

/*
 * =============================================================================
 * 向量创建与销毁
 * =============================================================================
 */

JW_API jw_vec_t jw_vec_create(jw_arena_t *arena, jw_dim_t dim)
{
    if (dim == 0) {
        return NULL;
    }
    
    jw_size_t size = dim * sizeof(jw_float32_t);
    
    if (arena != NULL) {
        return (jw_vec_t)jw_arena_alloc(arena, size);
    } else {
        return (jw_vec_t)jw_malloc(size);
    }
}

JW_API jw_vec_t jw_vec_create_with_value(jw_arena_t *arena,
                                          jw_dim_t dim,
                                          jw_float32_t value)
{
    jw_vec_t vec = jw_vec_create(arena, dim);
    if (vec == NULL) {
        return NULL;
    }
    
    for (jw_dim_t i = 0; i < dim; i++) {
        vec[i] = value;
    }
    
    return vec;
}

JW_API jw_vec_t jw_vec_copy(jw_arena_t *arena, jw_cvec_t src, jw_dim_t dim)
{
    if (src == NULL || dim == 0) {
        return NULL;
    }
    
    jw_vec_t dst = jw_vec_create(arena, dim);
    if (dst == NULL) {
        return NULL;
    }
    
    jw_memcpy(dst, src, dim * sizeof(jw_float32_t));
    return dst;
}

JW_API void jw_vec_free(jw_vec_t vec)
{
    if (vec != NULL) {
        jw_free(vec);
    }
}

/*
 * =============================================================================
 * 向量量化与反量化
 * =============================================================================
 */

/**
 * 量化向量到int8
 *
 * @param vec    输入向量
 * @param dim    维度
 * @param output 输出int8数组
 * @param scale  输出缩放因子
 */
JW_API void jw_vec_quantize_int8(jw_cvec_t vec, jw_dim_t dim,
                                  jw_int8_t *output, jw_float32_t *scale)
{
    if (vec == NULL || output == NULL || dim == 0) {
        return;
    }

    /* 计算向量的最大值和最小值 */
    jw_float32_t min_val = JW_FLT_MAX;
    jw_float32_t max_val = -JW_FLT_MAX;
    for (jw_dim_t i = 0; i < dim; i++) {
        if (vec[i] < min_val) min_val = vec[i];
        if (vec[i] > max_val) max_val = vec[i];
    }

    /* 计算缩放因子 */
    jw_float32_t range = max_val - min_val;
    if (range < 1e-6) {
        *scale = 1.0f;
        for (jw_dim_t i = 0; i < dim; i++) {
            output[i] = 0;
        }
        return;
    }

    *scale = range / 255.0f;
    jw_float32_t offset = min_val;

    /* 量化到int8 */
    for (jw_dim_t i = 0; i < dim; i++) {
        int32_t quantized = (int32_t)((vec[i] - offset) / *scale + 0.5f);
        /*  clamp to [-128, 127] */
        if (quantized < -128) quantized = -128;
        if (quantized > 127) quantized = 127;
        output[i] = (jw_int8_t)quantized;
    }
}

/**
 * 从int8反量化
 */
JW_API void jw_vec_dequantize_int8(const jw_int8_t *input, jw_dim_t dim,
                                    jw_float32_t scale, jw_vec_t output)
{
    if (input == NULL || output == NULL || dim == 0) {
        return;
    }

    for (jw_dim_t i = 0; i < dim; i++) {
        output[i] = (jw_float32_t)input[i] * scale;
    }
}

/**
 * 计算int8量化向量的点积
 */
JW_API jw_int32_t jw_vec_dot_int8(const jw_int8_t *a, 
                                   const jw_int8_t *b, 
                                   jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return 0;
    }

    jw_int32_t dot = 0;
    for (jw_dim_t i = 0; i < dim; i++) {
        dot += (jw_int32_t)a[i] * (jw_int32_t)b[i];
    }
    return dot;
}

JW_API void jw_vec_zero(jw_vec_t vec, jw_dim_t dim)
{
    if (vec != NULL && dim > 0) {
        jw_memset(vec, 0, dim * sizeof(jw_float32_t));
    }
}

JW_API void jw_vec_fill(jw_vec_t vec, jw_dim_t dim, jw_float32_t value)
{
    if (vec == NULL || dim == 0) {
        return;
    }
    
    for (jw_dim_t i = 0; i < dim; i++) {
        vec[i] = value;
    }
}

/*
 * =============================================================================
 * 基础算术运算
 * =============================================================================
 */

/* 向量加法 */
JW_API void jw_vec_add(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim, jw_vec_t result)
{
    if (result == NULL || a == NULL || b == NULL || dim == 0) {
        return;
    }
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 va = _mm256_loadu_ps(a + i);
            __m256 vb = _mm256_loadu_ps(b + i);
            __m256 vr = _mm256_add_ps(va, vb);
            _mm256_storeu_ps(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = a[i] + b[i];
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t va = vld1q_f32(a + i);
            float32x4_t vb = vld1q_f32(b + i);
            float32x4_t vr = vaddq_f32(va, vb);
            vst1q_f32(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = a[i] + b[i];
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            result[i] = a[i] + b[i];
        }
    }
}

/* 向量减法 */
JW_API void jw_vec_sub(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim, jw_vec_t result)
{
    if (result == NULL || a == NULL || b == NULL || dim == 0) {
        return;
    }
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 va = _mm256_loadu_ps(a + i);
            __m256 vb = _mm256_loadu_ps(b + i);
            __m256 vr = _mm256_sub_ps(va, vb);
            _mm256_storeu_ps(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = a[i] - b[i];
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t va = vld1q_f32(a + i);
            float32x4_t vb = vld1q_f32(b + i);
            float32x4_t vr = vsubq_f32(va, vb);
            vst1q_f32(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = a[i] - b[i];
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            result[i] = a[i] - b[i];
        }
    }
}

/* 向量数乘 */
JW_API void jw_vec_scale(jw_cvec_t vec, jw_dim_t dim, jw_float32_t scalar, jw_vec_t result)
{
    if (vec == NULL || result == NULL || dim == 0) {
        return;
    }
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        __m256 vs = _mm256_set1_ps(scalar);
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 vv = _mm256_loadu_ps(vec + i);
            __m256 vr = _mm256_mul_ps(vv, vs);
            _mm256_storeu_ps(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = vec[i] * scalar;
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        float32x4_t vs = vdupq_n_f32(scalar);
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t vv = vld1q_f32(vec + i);
            float32x4_t vr = vmulq_f32(vv, vs);
            vst1q_f32(result + i, vr);
        }
        for (; i < dim; i++) {
            result[i] = vec[i] * scalar;
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            result[i] = vec[i] * scalar;
        }
    }
}

/* 向量点积 */
JW_API jw_float32_t jw_vec_dot(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return 0.0f;
    }
    
    jw_float32_t sum = 0.0f;
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        __m256 vsum = _mm256_setzero_ps();
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 va = _mm256_loadu_ps(a + i);
            __m256 vb = _mm256_loadu_ps(b + i);
            vsum = _mm256_add_ps(vsum, _mm256_mul_ps(va, vb));
        }
        /* 水平求和 */
        __m128 vlow = _mm256_castps256_ps128(vsum);
        __m128 vhigh = _mm256_extractf128_ps(vsum, 1);
        vlow = _mm_add_ps(vlow, vhigh);
        __m128 shuf = _mm_movehdup_ps(vlow);
        __m128 sums = _mm_add_ps(vlow, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        sum = _mm_cvtss_f32(sums);
        
        for (; i < dim; i++) {
            sum += a[i] * b[i];
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        float32x4_t vsum = vdupq_n_f32(0.0f);
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t va = vld1q_f32(a + i);
            float32x4_t vb = vld1q_f32(b + i);
            vsum = vmlaq_f32(vsum, va, vb);
        }
        float32x2_t vsum2 = vadd_f32(vget_low_f32(vsum), vget_high_f32(vsum));
        vsum2 = vpadd_f32(vsum2, vsum2);
        sum = vget_lane_f32(vsum2, 0);
        
        for (; i < dim; i++) {
            sum += a[i] * b[i];
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            sum += a[i] * b[i];
        }
    }
    
    return sum;
}

/*
 * =============================================================================
 * 距离计算
 * =============================================================================
 */

/* L2 (欧几里得) 距离 */
JW_API jw_float32_t jw_vec_l2_distance(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return JW_FLT_MAX;
    }
    
    jw_float32_t sum = 0.0f;
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        __m256 vsum = _mm256_setzero_ps();
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 va = _mm256_loadu_ps(a + i);
            __m256 vb = _mm256_loadu_ps(b + i);
            __m256 vd = _mm256_sub_ps(va, vb);
            vsum = _mm256_add_ps(vsum, _mm256_mul_ps(vd, vd));
        }
        /* 水平求和 */
        __m128 vlow = _mm256_castps256_ps128(vsum);
        __m128 vhigh = _mm256_extractf128_ps(vsum, 1);
        vlow = _mm_add_ps(vlow, vhigh);
        __m128 shuf = _mm_movehdup_ps(vlow);
        __m128 sums = _mm_add_ps(vlow, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        sum = _mm_cvtss_f32(sums);
        
        for (; i < dim; i++) {
            jw_float32_t d = a[i] - b[i];
            sum += d * d;
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        float32x4_t vsum = vdupq_n_f32(0.0f);
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t va = vld1q_f32(a + i);
            float32x4_t vb = vld1q_f32(b + i);
            float32x4_t vd = vsubq_f32(va, vb);
            vsum = vmlaq_f32(vsum, vd, vd);
        }
        /* 水平求和 */
        float32x2_t vsum2 = vadd_f32(vget_low_f32(vsum), vget_high_f32(vsum));
        vsum2 = vpadd_f32(vsum2, vsum2);
        sum = vget_lane_f32(vsum2, 0);
        
        for (; i < dim; i++) {
            jw_float32_t d = a[i] - b[i];
            sum += d * d;
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            jw_float32_t d = a[i] - b[i];
            sum += d * d;
        }
    }
    
    return jw_math_sqrt_f32(sum);
}

/* L2 平方距离 (避免sqrt，用于排序) */
JW_API jw_float32_t jw_vec_l2_squared(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return JW_FLT_MAX;
    }

    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        jw_float32_t d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

/* L2距离 (别名) */
JW_API jw_float32_t jw_vec_distance_l2(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    return jw_vec_l2_distance(a, b, dim);
}

/* 内积距离 (负内积，用于最近邻搜索) */
JW_API jw_float32_t jw_vec_ip_distance(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    return -jw_vec_dot(a, b, dim);
}

/* 余弦距离 */
JW_API jw_float32_t jw_vec_cosine_distance(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return 1.0f;
    }
    
    jw_float32_t dot = jw_vec_dot_inline(a, b, dim);
    jw_float32_t norm_a = jw_vec_norm_l2_inline(a, dim);
    jw_float32_t norm_b = jw_vec_norm_l2_inline(b, dim);
    
    if (norm_a < JW_FLT_EPSILON || norm_b < JW_FLT_EPSILON) {
        return 1.0f;  /* 零向量 */
    }
    
    return 1.0f - dot / (norm_a * norm_b);
}

/* 余弦相似度 */
JW_API jw_float32_t jw_vec_cosine_similarity(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    return 1.0f - jw_vec_cosine_distance(a, b, dim);
}

/* 通用距离计算 */
JW_API jw_float32_t jw_vec_distance(jw_cvec_t a,
                                     jw_cvec_t b,
                                     jw_dim_t dim,
                                     jw_metric_t metric)
{
    switch (metric) {
        case JW_METRIC_L2:
            return jw_vec_distance_l2_inline(a, b, dim);
        case JW_METRIC_IP:
            return jw_vec_distance_ip_inline(a, b, dim);
        case JW_METRIC_COSINE:
            return jw_vec_cosine_distance(a, b, dim);
        default:
            return jw_vec_distance_l2_inline(a, b, dim);
    }
}

/*
 * =============================================================================
 * 向量范数
 * =============================================================================
 */



/* L2范数平方 */
JW_API jw_float32_t jw_vec_norm_squared(jw_cvec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return 0.0f;
    }
    
    jw_float32_t sum = 0.0f;
    
#if defined(JW_USE_AVX)
    if (jw_is_simd_enabled() && dim >= 8) {
        __m256 vsum = _mm256_setzero_ps();
        jw_dim_t i = 0;
        for (; i + 8 <= dim; i += 8) {
            __m256 vv = _mm256_loadu_ps(vec + i);
            vsum = _mm256_add_ps(vsum, _mm256_mul_ps(vv, vv));
        }
        /* 水平求和 */
        __m128 vlow = _mm256_castps256_ps128(vsum);
        __m128 vhigh = _mm256_extractf128_ps(vsum, 1);
        vlow = _mm_add_ps(vlow, vhigh);
        __m128 shuf = _mm_movehdup_ps(vlow);
        __m128 sums = _mm_add_ps(vlow, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        sum = _mm_cvtss_f32(sums);
        
        for (; i < dim; i++) {
            sum += vec[i] * vec[i];
        }
    } else
#elif defined(JW_USE_NEON)
    if (jw_is_simd_enabled() && dim >= 4) {
        float32x4_t vsum = vdupq_n_f32(0.0f);
        jw_dim_t i = 0;
        for (; i + 4 <= dim; i += 4) {
            float32x4_t vv = vld1q_f32(vec + i);
            vsum = vmlaq_f32(vsum, vv, vv);
        }
        float32x2_t vsum2 = vadd_f32(vget_low_f32(vsum), vget_high_f32(vsum));
        vsum2 = vpadd_f32(vsum2, vsum2);
        sum = vget_lane_f32(vsum2, 0);
        
        for (; i < dim; i++) {
            sum += vec[i] * vec[i];
        }
    } else
#endif
    {
        for (jw_dim_t i = 0; i < dim; i++) {
            sum += vec[i] * vec[i];
        }
    }
    
    return sum;
}

/* L2范数 */
JW_API jw_float32_t jw_vec_norm_l2(jw_cvec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return 0.0f;
    }
    return jw_math_sqrt_f32(jw_vec_norm_squared(vec, dim));
}

/* L2范数平方 (避免sqrt，用于比较) */
JW_API jw_float32_t jw_vec_norm_l2_squared(jw_cvec_t vec, jw_dim_t dim)
{
    return jw_vec_norm_squared(vec, dim);
}

/* L1范数 */
JW_API jw_float32_t jw_vec_l1_norm(jw_cvec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return 0.0f;
    }
    
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        sum += jw_math_abs_f32(vec[i]);
    }
    return sum;
}

/*
 * =============================================================================
 * 向量归一化
 * =============================================================================
 */

/* 归一化向量 (原地) */
JW_API jw_status_t jw_vec_normalize(jw_vec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_float32_t norm = jw_vec_norm_l2_inline(vec, dim);
    if (norm < JW_FLT_EPSILON) {
        return JW_INVALID_PARAM;  /* 零向量 */
    }
    
    jw_vec_scale(vec, dim, 1.0f / norm, vec);
    return JW_SUCCESS;
}

/* 归一化向量 (复制) */
JW_API jw_status_t jw_vec_normalize_to(jw_cvec_t src,
                                        jw_vec_t dst,
                                        jw_dim_t dim)
{
    if (src == NULL || dst == NULL || dim == 0) {
        return JW_INVALID_PARAM;
    }

    jw_memcpy(dst, src, dim * sizeof(jw_float32_t));
    return jw_vec_normalize(dst, dim);
}

/* 归一化向量并返回新向量 */
JW_API jw_vec_t jw_vec_normalized(jw_arena_t *arena, jw_cvec_t vec, jw_dim_t dim)
{
    if (arena == NULL || vec == NULL || dim == 0) {
        return NULL;
    }

    jw_vec_t result = jw_vec_create(arena, dim);
    if (result == NULL) {
        return NULL;
    }

    if (jw_vec_normalize_to(vec, result, dim) != JW_SUCCESS) {
        return NULL;
    }

    return result;
}

/*
 * =============================================================================
 * 向量比较
 * =============================================================================
 */

JW_API jw_bool_t jw_vec_equal(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim, jw_float32_t epsilon)
{
    if (a == NULL || b == NULL || dim == 0) {
        return JW_FALSE;
    }
    
    for (jw_dim_t i = 0; i < dim; i++) {
        if (jw_math_abs_f32(a[i] - b[i]) > epsilon) {
            return JW_FALSE;
        }
    }
    return JW_TRUE;
}

JW_API jw_bool_t jw_vec_almost_equal(jw_cvec_t a,
                                      jw_cvec_t b,
                                      jw_dim_t dim,
                                      jw_float32_t epsilon)
{
    if (a == NULL || b == NULL || dim == 0) {
        return JW_FALSE;
    }
    
    for (jw_dim_t i = 0; i < dim; i++) {
        if (jw_math_abs_f32(a[i] - b[i]) > epsilon) {
            return JW_FALSE;
        }
    }
    return JW_TRUE;
}

/*
 * =============================================================================
 * 批量操作
 * =============================================================================
 */

/* 批量计算距离 */
JW_API void jw_vec_batch_distance(jw_cvec_t query,
                                   jw_cvec_t *vectors,
                                   jw_dim_t dim,
                                   jw_size_t count,
                                   jw_metric_t metric,
                                   jw_float32_t *distances)
{
    if (query == NULL || vectors == NULL || distances == NULL) {
        return;
    }
    
    /* 批量优化：根据不同的距离度量使用不同的实现 */
    switch (metric) {
        case JW_METRIC_L2:
            jw_vec_batch_l2_distance(query, vectors, dim, count, distances);
            break;
        case JW_METRIC_IP:
            for (jw_size_t i = 0; i < count; i++) {
                distances[i] = -jw_vec_dot(query, vectors[i], dim);
            }
            break;
        case JW_METRIC_COSINE:
            for (jw_size_t i = 0; i < count; i++) {
                distances[i] = jw_vec_cosine_distance(query, vectors[i], dim);
            }
            break;
        default:
            jw_vec_batch_l2_distance(query, vectors, dim, count, distances);
            break;
    }
}

/* 批量计算L2距离 */
JW_API void jw_vec_batch_l2_distance(jw_cvec_t query,
                                      jw_cvec_t *vectors,
                                      jw_dim_t dim,
                                      jw_size_t count,
                                      jw_float32_t *distances)
{
    for (jw_size_t i = 0; i < count; i++) {
        distances[i] = jw_vec_l2_distance(query, vectors[i], dim);
    }
}

/* 批量计算余弦相似度 */
JW_API void jw_vec_batch_cosine_similarity(jw_cvec_t query,
                                            const jw_vec_batch_t *batch,
                                            jw_float32_t *results)
{
    if (query == NULL || batch == NULL || results == NULL) {
        return;
    }
    
    for (jw_size_t i = 0; i < batch->count; i++) {
        results[i] = jw_vec_cosine_similarity(query, batch->vectors[i], batch->dimension);
    }
}

/*
 * =============================================================================
 * 向量聚合
 * =============================================================================
 */

/* 向量求和 */
JW_API jw_float32_t jw_vec_sum(jw_cvec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return 0.0f;
    }
    
    jw_float32_t sum = 0.0f;
    for (jw_dim_t i = 0; i < dim; i++) {
        sum += vec[i];
    }
    return sum;
}

/* 向量数组求和 */
JW_API void jw_vec_array_sum(jw_vec_t dst,
                             jw_cvec_t *vectors,
                             jw_dim_t dim,
                             jw_size_t count)
{
    if (dst == NULL || vectors == NULL || count == 0) {
        return;
    }
    
    jw_vec_zero(dst, dim);
    
    for (jw_size_t i = 0; i < count; i++) {
        jw_vec_add(vectors[i], dst, dim, dst);
    }
}

/* 向量元素平均值 */
JW_API jw_float32_t jw_vec_mean(jw_cvec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return 0.0f;
    }
    
    jw_float32_t sum = jw_vec_sum(vec, dim);
    return sum / (jw_float32_t)dim;
}

/* 向量数组平均 */
JW_API void jw_vec_array_mean(jw_vec_t dst,
                              jw_cvec_t *vectors,
                              jw_dim_t dim,
                              jw_size_t count)
{
    if (count == 0) {
        return;
    }
    
    jw_vec_array_sum(dst, vectors, dim, count);
    jw_vec_scale(dst, dim, 1.0f / (jw_float32_t)count, dst);
}

/* 加权平均 */
JW_API void jw_vec_weighted_mean(jw_vec_t dst,
                                  jw_cvec_t *vectors,
                                  const jw_float32_t *weights,
                                  jw_dim_t dim,
                                  jw_size_t count)
{
    if (dst == NULL || vectors == NULL || weights == NULL || count == 0) {
        return;
    }
    
    jw_vec_zero(dst, dim);
    jw_float32_t weight_sum = 0.0f;
    
    for (jw_size_t i = 0; i < count; i++) {
        jw_vec_t temp = jw_vec_create(NULL, dim);
        if (temp) {
            jw_vec_scale(vectors[i], dim, weights[i], temp);
            jw_vec_add(temp, dst, dim, dst);
            jw_vec_free(temp);
        }
        weight_sum += weights[i];
    }
    
    if (weight_sum > JW_FLT_EPSILON) {
        jw_vec_scale(dst, dim, 1.0f / weight_sum, dst);
    }
}

/*
 * =============================================================================
 * 随机向量生成
 * =============================================================================
 */

/* 生成随机向量 */
JW_API void jw_vec_random(jw_vec_t vec, jw_dim_t dim)
{
    if (vec == NULL || dim == 0) {
        return;
    }
    
    for (jw_dim_t i = 0; i < dim; i++) {
        vec[i] = jw_rand_float() * 2.0f - 1.0f;  /* [-1, 1) */
    }
}

/* 生成随机归一化向量 */
JW_API void jw_vec_random_normalized(jw_vec_t vec, jw_dim_t dim)
{
    jw_vec_random(vec, dim);
    jw_vec_normalize(vec, dim);
}

/* 生成高斯随机向量 */
JW_API void jw_vec_random_gaussian(jw_vec_t vec, jw_dim_t dim,
                                    jw_float32_t mean, jw_float32_t stddev)
{
    if (vec == NULL || dim == 0) {
        return;
    }
    
    /* Box-Muller变换 */
    for (jw_dim_t i = 0; i < dim; i += 2) {
        jw_float32_t u1 = jw_rand_float();
        jw_float32_t u2 = jw_rand_float();
        
        /* 避免log(0) */
        while (u1 < JW_FLT_EPSILON) {
            u1 = jw_rand_float();
        }
        
        jw_float32_t r = jw_math_sqrt_f32(-2.0f * jw_math_log_f32(u1));
        jw_float32_t theta = 2.0f * 3.14159265358979f * u2;
        
        vec[i] = mean + stddev * r * jw_math_cos_f32(theta);
        if (i + 1 < dim) {
            vec[i + 1] = mean + stddev * r * jw_math_sin_f32(theta);
        }
    }
}

/*
 * =============================================================================
 * SQ (Scalar Quantization) 实现
 * =============================================================================
 */

