/*
 * test_performance.c - JinWo VecDB 性能基准测试
 *
 * Copyright 2026 北京金幄科技有限公司
 */

#include "jw_stdio.h"
#include "jw_stdlib.h"
#include "jw_string.h"
#include "jw_vecdb.h"
#include "jw_arena.h"

#define WARMUP_ITERATIONS 100
#define BENCH_ITERATIONS  10000
#define LARGE_ITERATIONS 1000

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

static void print_header(const char *test_name)
{
    jw_printf("\n==================================\n");
    jw_printf("%s\n", test_name);
    jw_printf("==================================\n");
}

static void bench_index_performance(void)
{
    print_header("Index Performance Test");
    
    jw_vecdb_t *db = NULL;
    jw_status_t status = jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to open database: %d\n", status);
        return;
    }

    jw_collection_t *coll = NULL;
    status = jw_vecdb_create_collection(db, jw_str("test_perf"), 128, &coll);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to create collection: %d\n", status);
        jw_vecdb_close(db);
        return;
    }

    // 插入向量
    jw_printf("Inserting vectors...\n");
    jw_float32_t vec[128];
    jw_vid_t vid;
    
    jw_float64_t start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        generate_random_vector(vec, 128);
        jw_collection_insert(coll, vec, &vid);
    }
    jw_float64_t end = get_time_ms();
    jw_printf("Insert 1000 vectors: %.2f ms\n", end - start);

    // 搜索性能
    jw_printf("Searching vectors...\n");
    jw_search_result_t results[10];
    
    start = get_time_ms();
    for (int i = 0; i < 100; i++) {
        generate_random_vector(vec, 128);
        jw_size_t count = jw_collection_search(coll, vec, 10, results);
    }
    end = get_time_ms();
    jw_printf("Search 100 times: %.2f ms\n", end - start);

    jw_collection_close(coll);
    jw_vecdb_close(db);
}

static void bench_storage_performance(void)
{
    print_header("Storage Performance Test");
    
    // 测试内存数据库
    jw_vecdb_t *db = NULL;
    jw_status_t status = jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to open memory database: %d\n", status);
        return;
    }

    jw_collection_t *coll = NULL;
    status = jw_vecdb_create_collection(db, jw_str("test_storage"), 64, &coll);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to create collection: %d\n", status);
        jw_vecdb_close(db);
        return;
    }

    // 测试存储性能
    jw_float32_t vec[64];
    jw_vid_t vid;
    
    jw_printf("Memory storage performance...\n");
    jw_float64_t start = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        generate_random_vector(vec, 64);
        jw_collection_insert(coll, vec, &vid);
    }
    jw_float64_t end = get_time_ms();
    jw_printf("Memory insert 1000 vectors: %.2f ms\n", end - start);

    jw_collection_close(coll);
    jw_vecdb_close(db);
}

static void bench_concurrent_performance(void)
{
    print_header("Concurrent Performance Test");
    
    jw_vecdb_t *db = NULL;
    jw_status_t status = jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to open database: %d\n", status);
        return;
    }

    jw_collection_t *coll = NULL;
    status = jw_vecdb_create_collection(db, jw_str("test_concurrent"), 128, &coll);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to create collection: %d\n", status);
        jw_vecdb_close(db);
        return;
    }

    // 测试并发插入
    jw_printf("Concurrent insert performance...\n");
    jw_float32_t vec[128];
    jw_vid_t vid;
    
    jw_float64_t start = get_time_ms();
    for (int i = 0; i < 500; i++) {
        generate_random_vector(vec, 128);
        jw_collection_insert(coll, vec, &vid);
    }
    jw_float64_t end = get_time_ms();
    jw_printf("Sequential insert 500 vectors: %.2f ms\n", end - start);

    jw_collection_close(coll);
    jw_vecdb_close(db);
}

static void bench_quantization_performance(void)
{
    print_header("Quantization Performance Test");
    
    jw_vecdb_t *db = NULL;
    jw_status_t status = jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to open database: %d\n", status);
        return;
    }

    jw_collection_t *coll = NULL;
    status = jw_vecdb_create_collection(db, jw_str("test_quant"), 128, &coll);
    if (status != JW_SUCCESS) {
        jw_printf("Failed to create collection: %d\n", status);
        jw_vecdb_close(db);
        return;
    }

    // 测试量化性能
    jw_float32_t vec[128];
    jw_vid_t vid;
    
    jw_printf("Quantization performance...\n");
    jw_float64_t start = get_time_ms();
    for (int i = 0; i < 500; i++) {
        generate_random_vector(vec, 128);
        jw_collection_insert(coll, vec, &vid);
    }
    jw_float64_t end = get_time_ms();
    jw_printf("Insert 500 vectors with quantization: %.2f ms\n", end - start);

    // 测试搜索性能
    jw_search_result_t results[5];
    start = get_time_ms();
    for (int i = 0; i < 100; i++) {
        generate_random_vector(vec, 128);
        jw_size_t count = jw_collection_search(coll, vec, 5, results);
    }
    end = get_time_ms();
    jw_printf("Search 100 times with quantization: %.2f ms\n", end - start);

    jw_collection_close(coll);
    jw_vecdb_close(db);
}

int main(int argc, char *argv[])
{
    jw_printf("=== JinWo VecDB Performance Benchmark ===\n\n");

    bench_index_performance();
    bench_storage_performance();
    bench_concurrent_performance();
    bench_quantization_performance();

    jw_printf("\n=== Benchmark completed ===\n");
    return 0;
}
