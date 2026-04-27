/*
 * demo.c - JinWo VecDB 使用示例
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * 本示例演示如何使用JinWo VecDB的基本功能:
 *   - 创建内存池
 *   - 向量操作
 *   - 创建Collection
 *   - 插入和搜索向量
 */

#include "jw_stdio.h"
#include "jw_stdlib.h"
#include "jw_string.h"
#include "jw_vecdb.h"
#include "jw_arena.h"
#include "jw_vector.h"

static void generate_random_vector(jw_float32_t *vec, jw_int32_t dim)
{
    for (jw_int32_t i = 0; i < dim; i++) {
        vec[i] = jw_rand_float() * 2.0f - 1.0f;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    jw_printf("========================================\n");
    jw_printf("  JinWo VecDB 演示程序\n");
    jw_printf("========================================\n\n");

    jw_str_t version = jw_vecdb_version();
    jw_printf("版本: %.*s\n\n", (int)version.slen, version.ptr);

    jw_printf("--- 演示1: 内存池 ---");

    jw_arena_t *arena = NULL;
    jw_status_t status = jw_arena_create(1048576, &arena);
    if (status != JW_SUCCESS || arena == NULL) {
        jw_printf("创建内存池失败: %d\n", status);
        return 1;
    }
    jw_printf("内存池创建成功\n");

    jw_printf("\n--- 演示2: 向量操作 ---");
    jw_dim_t dim = 128;
    jw_vec_t vec1 = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    jw_vec_t vec2 = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));

    generate_random_vector(vec1, dim);
    generate_random_vector(vec2, dim);

    jw_float32_t dot = jw_vec_dot(vec1, vec2, dim);
    jw_float32_t l2 = jw_vec_distance_l2(vec1, vec2, dim);
    jw_float32_t cosine = jw_vec_cosine_similarity(vec1, vec2, dim);

    jw_printf("\n向量维度: %d\n", dim);
    jw_printf("点积: %.4f\n", dot);
    jw_printf("L2距离: %.4f\n", l2);
    jw_printf("余弦相似度: %.4f\n", cosine);

    jw_printf("\n--- 演示3: 集合操作 ---");
    jw_collection_config_t collection_config = {0};
    collection_config.dimension = dim;
    collection_config.metric = JW_METRIC_L2;

    jw_collection_t *collection = NULL;
    collection_config.name.ptr = "test_collection";
    collection_config.name.slen = 13;
    collection = jw_collection_create(arena, &collection_config);
    if (status != JW_SUCCESS || collection == NULL) {
        jw_printf("创建集合失败: %d\n", status);
        jw_arena_destroy(arena);
        return 1;
    }
    jw_printf("\n集合创建成功: test_collection\n");

    // 插入一些向量
    jw_printf("\n插入10个向量...\n");
    for (jw_int32_t i = 0; i < 10; i++) {
        jw_vec_t vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
        generate_random_vector(vec, dim);
        status = jw_collection_insert(collection, vec, NULL);
        if (status != JW_SUCCESS) {
            jw_printf("插入向量失败: %d\n", status);
        }
    }
    jw_printf("插入完成\n");

    // 搜索向量
    jw_printf("\n搜索相似向量...\n");
    jw_printf("[DEBUG] 准备搜索，生成查询向量\n");
    jw_vec_t query = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    generate_random_vector(query, dim);

    jw_search_options_t options = {0};
    options.k = 5;
    jw_search_result_ex_t results[5];
    jw_printf("[DEBUG] 调用jw_collection_search...\n");
    jw_size_t result_count = jw_collection_search(collection, query, &options, results);
    jw_printf("[DEBUG] jw_collection_search返回: %u\n", (unsigned)result_count);
    if (result_count > 0) {
        jw_printf("搜索结果 (前5个):\n");
        for (jw_uint32_t i = 0; i < result_count; i++) {
            jw_printf("ID: %d, 距离: %.4f\n", 
                     (int)results[i].vid, results[i].score);
        }
    } else {
        jw_printf("搜索失败\n");
    }

    // 清理
    jw_collection_destroy(collection);
    jw_arena_destroy(arena);

    jw_printf("\n演示完成!\n");
    return 0;
}
