/*
 * test_quantization.c - JinWo VecDB 向量量化测试
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

#include "jw_quant.h"
#include "jw_arena.h"
#include "jw_types.h"
#include "jw_stdio.h"
#include "jw_stdlib.h"
#include "jw_math.h"

static jw_uint32_t test_passed = 0;
static jw_uint32_t test_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    jw_printf("Running test_%s... ", #name); \
    test_##name(); \
    test_passed++; \
    jw_printf("PASS\n"); \
} while (0)
#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s\n", #condition); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while (0)
#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s == %s\n", #expected, #actual); \
        jw_printf("  Expected: %d, Actual: %d\n", (int)(expected), (int)(actual)); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while (0)
#define ASSERT_NEAR(expected, actual, epsilon) do { \
    if (jw_math_abs_f64((expected) - (actual)) > (epsilon)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s ~= %s\n", #expected, #actual); \
        jw_printf("  Expected: %f, Actual: %f, Epsilon: %f\n", (float)(expected), (float)(actual), (float)(epsilon)); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while (0)

static jw_arena_t* create_test_arena(void)
{
    jw_arena_t *arena;
    jw_status_t status = jw_arena_create(4096 * 1024, &arena);
    if (status != JW_SUCCESS) {
        jw_printf("FAIL\n");
        jw_printf("  Pool creation failed with status: %d\n", status);
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__);
        test_failed++;
        return NULL;
    }
    return arena;
}

/**
 * 生成随机向量
 */
static void generate_random_vector(jw_vec_t vec, jw_dim_t dim, jw_float32_t min, jw_float32_t max)
{
    for (jw_dim_t i = 0; i < dim; i++) {
        vec[i] = min + jw_rand_float() * (max - min);
    }
}

/**
 * 测试SQ (Scalar Quantization) 量化
 */
TEST(sq_quantization)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_TRUE(arena != NULL);
    
    jw_dim_t dim = 8;
    jw_vec_t vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    ASSERT_TRUE(vec != NULL);
    
    // 生成测试向量
    generate_random_vector(vec, dim, -10.0f, 10.0f);
    
    // 测试SQ量化
    jw_sq_quantizer_t *sq = jw_sq_quantizer_create(arena, dim);
    ASSERT_TRUE(sq != NULL);
    
    jw_status_t status = jw_sq_quantizer_train(sq, vec, 1);
    ASSERT_TRUE(status == JW_SUCCESS);
    
    jw_uint8_t *quantized = (jw_uint8_t *)jw_arena_alloc(arena, dim * sizeof(jw_uint8_t));
    ASSERT_TRUE(quantized != NULL);
    
    status = jw_sq_quantize(sq, vec, quantized);
    ASSERT_TRUE(status == JW_SUCCESS);
    
    jw_vec_t dequantized = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    ASSERT_TRUE(dequantized != NULL);
    
    status = jw_sq_dequantize(sq, quantized, dequantized);
    ASSERT_TRUE(status == JW_SUCCESS);
    
    // 验证量化误差
    for (jw_dim_t i = 0; i < dim; i++) {
        ASSERT_NEAR(vec[i], dequantized[i], 0.1f);
    }
    
    jw_arena_destroy(arena);
}

/**
 * 测试PQ (Product Quantization) 量化
 */
TEST(pq_quantization)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_TRUE(arena != NULL);
    
    jw_dim_t dim = 8;
    jw_vec_t vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    ASSERT_TRUE(vec != NULL);
    
    // 生成测试向量
    generate_random_vector(vec, dim, -10.0f, 10.0f);
    
    // 测试PQ量化
    jw_pq_quantizer_t *pq;
    jw_status_t status = jw_pq_create(arena, dim, 4, 8, &pq);
    ASSERT_TRUE(status == JW_SUCCESS);
    
    status = jw_pq_train(pq, vec, 1);
    ASSERT_TRUE(status == JW_SUCCESS);
    
    jw_uint8_t *quantized = (jw_uint8_t *)jw_arena_alloc(arena, pq->nsub * sizeof(jw_uint8_t));
    ASSERT_TRUE(quantized != NULL);
    
    jw_pq_encode(pq, vec, quantized);
    
    // PQ没有直接的dequantize函数，使用距离计算来验证
    jw_float32_t dist = jw_pq_distance(pq, vec, quantized, JW_METRIC_L2);
    ASSERT_NEAR(0.0f, dist, 1.0f);
    
    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Running quantization tests...\n\n");
    
    RUN_TEST(sq_quantization);
    RUN_TEST(pq_quantization);
    
    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);
    
    if (test_failed == 0) {
        jw_printf("All tests passed!\n");
        return 0;
    } else {
        jw_printf("Some tests failed!\n");
        return 1;
    }
}
