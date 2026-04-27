/*
 * vector_bench.c - JinWo VecDB 向量操作性能测试
 *
 * Copyright 2026 北京金幄科技有限公司
 */

#include "jw_stdio.h"
#include "jw_stdlib.h"
#include "jw_string.h"
#include "jw_vecdb.h"
#include "jw_arena.h"
#include "jw_vector.h"

#define WARMUP_ITERATIONS 100
#define BENCH_ITERATIONS  10000

static jw_float64_t get_time_ms(void)
{
    return jw_time_now();
}

static void generate_random_vector(jw_float32_t *vec, jw_int32_t dim)
{
    for (jw_int32_t i = 0; i < dim; i++) {
        vec[i] = jw_rand_float() * 2.0f - 1.0f;
    }
}

static void bench_dot_product(jw_int32_t dim, jw_int32_t iterations)
{
    jw_float32_t *a = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));
    jw_float32_t *b = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));

    generate_random_vector(a, dim);
    generate_random_vector(b, dim);

    volatile jw_float32_t result = 0;
    for (jw_int32_t i = 0; i < WARMUP_ITERATIONS; i++) {
        result = jw_vec_dot(a, b, dim);
    }

    jw_float64_t start = get_time_ms();
    for (jw_int32_t i = 0; i < iterations; i++) {
        result = jw_vec_dot(a, b, dim);
    }
    jw_float64_t end = get_time_ms();

    jw_printf("Dot product (dim=%d): %.2f us/iter\n", dim, (end - start) * 1000 / iterations);

    jw_free(a);
    jw_free(b);
}

static void bench_l2_distance(jw_int32_t dim, jw_int32_t iterations)
{
    jw_float32_t *a = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));
    jw_float32_t *b = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));

    generate_random_vector(a, dim);
    generate_random_vector(b, dim);

    volatile jw_float32_t result = 0;
    for (jw_int32_t i = 0; i < WARMUP_ITERATIONS; i++) {
        result = jw_vec_distance_l2(a, b, dim);
    }

    jw_float64_t start = get_time_ms();
    for (jw_int32_t i = 0; i < iterations; i++) {
        result = jw_vec_distance_l2(a, b, dim);
    }
    jw_float64_t end = get_time_ms();

    jw_printf("L2 distance (dim=%d): %.2f us/iter\n", dim, (end - start) * 1000 / iterations);

    jw_free(a);
    jw_free(b);
}

static void bench_cosine_similarity(jw_int32_t dim, jw_int32_t iterations)
{
    jw_float32_t *a = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));
    jw_float32_t *b = (jw_float32_t *)jw_malloc(dim * sizeof(jw_float32_t));

    generate_random_vector(a, dim);
    generate_random_vector(b, dim);

    volatile jw_float32_t result = 0;
    for (jw_int32_t i = 0; i < WARMUP_ITERATIONS; i++) {
        result = jw_vec_cosine_similarity(a, b, dim);
    }

    jw_float64_t start = get_time_ms();
    for (jw_int32_t i = 0; i < iterations; i++) {
        result = jw_vec_cosine_similarity(a, b, dim);
    }
    jw_float64_t end = get_time_ms();

    jw_printf("Cosine similarity (dim=%d): %.2f us/iter\n", dim, (end - start) * 1000 / iterations);

    jw_free(a);
    jw_free(b);
}

int main(int argc, char *argv[])
{
    jw_printf("=== JinWo VecDB Vector Benchmark ===\n\n");

    // 测试不同维度的向量操作性能
    int dims[] = {64, 128, 256, 512, 1024};
    int num_dims = sizeof(dims) / sizeof(dims[0]);

    for (int i = 0; i < num_dims; i++) {
        int dim = dims[i];
        jw_printf("\nTesting dimension: %d\n", dim);
        jw_printf("==================================\n");
        bench_dot_product(dim, BENCH_ITERATIONS);
        bench_l2_distance(dim, BENCH_ITERATIONS);
        bench_cosine_similarity(dim, BENCH_ITERATIONS);
    }

    jw_printf("\n=== Benchmark completed ===\n");
    return 0;
}
