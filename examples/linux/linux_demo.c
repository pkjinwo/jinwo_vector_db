/*
 * linux_demo.c - JinWo VecDB Linux 平台演示程序
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * 本示例演示如何在Linux平台使用JinWo VecDB的基本功能:
 *   - 打开和关闭数据库
 *   - 创建和管理集合
 *   - 插入和搜索向量
 *   - 错误处理
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "jw_vecdb.h"

static void generate_random_vector(float *vec, int dim)
{
    for (int i = 0; i < dim; i++) {
        vec[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    srand(time(NULL));

    printf("========================================\n");
    printf("  JinWo VecDB Linux 演示程序\n");
    printf("========================================\n\n");

    // 打开数据库
    printf("打开数据库...\n");
    jw_vecdb_t *db = NULL;
    int result = jw_vecdb_open(&db, "./vecdb", true);
    if (result != JW_OK) {
        printf("打开数据库失败: %d\n", result);
        return 1;
    }
    printf("数据库打开成功\n");

    // 获取版本
    const char *version = jw_version();
    printf("版本: %s\n\n", version);

    // 创建集合
    printf("创建集合...\n");
    jw_collection_t *collection = NULL;
    result = jw_collection_create(&collection, db, "test", 128);
    if (result != JW_OK) {
        printf("创建集合失败: %d\n", result);
        jw_vecdb_close(db);
        return 1;
    }
    printf("集合创建成功: test\n\n");

    // 插入向量
    printf("插入10个向量...\n");
    float vector[128];
    for (int i = 0; i < 10; i++) {
        generate_random_vector(vector, 128);
        uint64_t id;
        result = jw_collection_insert(collection, vector, &id);
        if (result != JW_OK) {
            printf("插入向量 %d 失败: %d\n", i, result);
        } else {
            printf("插入向量 %d 成功，ID: %llu\n", i, id);
        }
    }
    printf("插入完成\n\n");

    // 搜索向量
    printf("搜索相似向量...\n");
    float query[128];
    generate_random_vector(query, 128);

    jw_search_result_t *results = NULL;
    size_t result_count;
    result = jw_collection_search(collection, query, 5, &results, &result_count);
    if (result != JW_OK) {
        printf("搜索失败: %d\n", result);
    } else if (result_count > 0) {
        printf("搜索结果 (前%d个):\n", (int)result_count);
        for (size_t i = 0; i < result_count; i++) {
            printf("ID: %llu, 距离: %.4f\n", results[i].id, results[i].distance);
        }
        free(results);
    } else {
        printf("搜索失败，无结果\n");
    }

    // 列出集合
    printf("\n列出所有集合...\n");
    char **names = NULL;
    size_t count;
    result = jw_collection_list(db, &names, &count);
    if (result != JW_OK) {
        printf("列出集合失败: %d\n", result);
    } else {
        printf("共有 %zu 个集合:\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("  %s\n", names[i]);
            free(names[i]);
        }
        free(names);
    }

    // 清理
    jw_collection_close(collection);
    jw_vecdb_close(db);

    printf("\n演示完成!\n");
    return 0;
}
