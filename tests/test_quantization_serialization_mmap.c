/*
 * test_quantization_serialization_mmap.c - 测试量化、序列化和MMAP模式
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_vecdb.h"
#include "jw_quant.h"
#include "jw_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 测试参数 */
#define TEST_DIM 64
#define TEST_VECTORS 50
#define TEST_K 10
#define TEST_INDEX_PATH "./test_index.bin"
#define TEST_MMAP_PATH "./test_mmap.bin"

/* 生成随机向量 */
static void generate_random_vectors(jw_float32_t *vectors, jw_size_t count, jw_dim_t dim)
{
    for (jw_size_t i = 0; i < count * dim; i++) {
        vectors[i] = (jw_float32_t)rand() / RAND_MAX * 2.0f - 1.0f;
    }
}

/* 测试SQ量化 */
static jw_bool_t test_sq_quantization(void)
{
    printf("\n=== 测试SQ量化 ===\n");
    
    jw_arena_t *arena = NULL;
    jw_arena_create(256 * 1024 * 1024, &arena);
    if (arena == NULL) {
        printf("[ERROR] 无法创建内存池\n");
        return JW_FALSE;
    }
    
    /* 创建量化器 */
    jw_sq_quantizer_t *sq = jw_sq_quantizer_create(arena, TEST_DIM);
    if (sq == NULL) {
        printf("[ERROR] 无法创建SQ量化器\n");
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 生成测试数据 */
    jw_float32_t *vectors = (jw_float32_t *)malloc(TEST_VECTORS * TEST_DIM * sizeof(jw_float32_t));
    if (vectors == NULL) {
        printf("[ERROR] 无法分配内存\n");
        jw_sq_quantizer_destroy(sq);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    generate_random_vectors(vectors, TEST_VECTORS, TEST_DIM);
    
    /* 训练量化器 */
    jw_status_t status = jw_sq_quantizer_train(sq, vectors, TEST_VECTORS);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 训练SQ量化器失败: %d\n", status);
        free(vectors);
        jw_sq_quantizer_destroy(sq);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 训练SQ量化器成功\n");
    
    /* 测试量化和反量化 */
    jw_uint8_t *quantized = (jw_uint8_t *)malloc(TEST_DIM * sizeof(jw_uint8_t));
    jw_float32_t *dequantized = (jw_float32_t *)malloc(TEST_DIM * sizeof(jw_float32_t));
    
    if (quantized == NULL || dequantized == NULL) {
        printf("[ERROR] 无法分配内存\n");
        free(vectors);
        free(quantized);
        free(dequantized);
        jw_sq_quantizer_destroy(sq);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 量化第一个向量 */
    status = jw_sq_quantize(sq, vectors, quantized);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 量化失败: %d\n", status);
        free(vectors);
        free(quantized);
        free(dequantized);
        jw_sq_quantizer_destroy(sq);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 反量化 */
    status = jw_sq_dequantize(sq, quantized, dequantized);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 反量化失败: %d\n", status);
        free(vectors);
        free(quantized);
        free(dequantized);
        jw_sq_quantizer_destroy(sq);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 计算误差 */
    jw_float32_t error = 0.0f;
    for (jw_dim_t i = 0; i < TEST_DIM; i++) {
        jw_float32_t diff = vectors[i] - dequantized[i];
        error += diff * diff;
    }
    error = jw_math_sqrt_f32(error / TEST_DIM);
    
    printf("[SUCCESS] 量化/反量化测试完成，平均误差: %.6f\n", error);
    
    /* 测试距离计算 */
    jw_float32_t dist1 = jw_vec_distance(vectors, vectors + TEST_DIM, TEST_DIM, JW_METRIC_L2);
    jw_float32_t dist2 = jw_sq_distance(sq, quantized, vectors + TEST_DIM, JW_METRIC_L2);
    
    printf("[SUCCESS] 距离计算测试: 原始距离=%.6f, 量化距离=%.6f\n", dist1, dist2);
    
    /* 清理 */
    free(vectors);
    free(quantized);
    free(dequantized);
    jw_sq_quantizer_destroy(sq);
    jw_arena_destroy(arena);
    
    return JW_TRUE;
}

/* 测试索引序列化和反序列化 */
static jw_bool_t test_index_serialization(void)
{
    printf("\n=== 测试索引序列化和反序列化 ===\n");
    
    jw_arena_t *arena = NULL;
    jw_arena_create(256 * 1024 * 1024, &arena);
    if (arena == NULL) {
        printf("[ERROR] 无法创建内存池\n");
        return JW_FALSE;
    }
    
    /* 生成测试数据 */
    jw_float32_t *vectors = (jw_float32_t *)malloc(TEST_VECTORS * TEST_DIM * sizeof(jw_float32_t));
    if (vectors == NULL) {
        printf("[ERROR] 无法分配内存\n");
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    generate_random_vectors(vectors, TEST_VECTORS, TEST_DIM);
    
    /* 创建索引配置 */
    jw_index_config_t config = {
        .type = JW_INDEX_IVF_SQ,
        .dim = TEST_DIM,
        .metric = JW_METRIC_L2,
        .quant = JW_QUANT_UINT8
    };
    config.params.ivf.nlist = 5;
    config.params.ivf.nprobe = 3;
    
    /* 创建索引 */
    jw_index_t *index = jw_index_create(arena, &config);
    if (index == NULL) {
        printf("[ERROR] 无法创建索引\n");
        free(vectors);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 训练索引 */
    jw_status_t status = jw_index_train(index, vectors, TEST_VECTORS);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 训练索引失败: %d\n", status);
        jw_index_destroy(index);
        free(vectors);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    /* 调试信息：检查arena使用情况 */
    printf("[DEBUG] Arena used after training: %zu bytes\n", arena->used);
    printf("[DEBUG] Arena size: %zu bytes\n", arena->size);
    
    /* 检查ivf->sq是否为NULL */
    jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
    printf("[DEBUG] ivf->sq: %p\n", ivf->sq);
    printf("[DEBUG] ivf->type: %d\n", ivf->type);
    printf("[DEBUG] ivf->quant_type: %d\n", ivf->quant_type);
    
    /* 添加向量 */
    jw_vid_t *vids = (jw_vid_t *)malloc(TEST_VECTORS * sizeof(jw_vid_t));
    if (vids == NULL) {
        printf("[ERROR] 无法分配内存\n");
        jw_index_destroy(index);
        free(vectors);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    for (jw_size_t i = 0; i < TEST_VECTORS; i++) {
        vids[i] = i + 1;
    }
    
    status = jw_index_add_batch(index, vids, vectors, TEST_VECTORS);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 添加向量失败: %d\n", status);
        jw_index_destroy(index);
        free(vectors);
        free(vids);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 索引创建和训练成功\n");
    
    /* 测试搜索 */
    jw_search_result_t results[TEST_K];
    jw_size_t found = jw_index_search(index, vectors, TEST_K, results);
    if (found == 0) {
        printf("[ERROR] 搜索失败\n");
        jw_index_destroy(index);
        free(vectors);
        free(vids);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 搜索测试成功，找到 %zu 个结果\n", found);
    
    /* 保存索引 */
    jw_str_t path = { .ptr = TEST_INDEX_PATH, .slen = strlen(TEST_INDEX_PATH) };
    status = jw_index_save(index, &path);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 保存索引失败: %d\n", status);
        jw_index_destroy(index);
        free(vectors);
        free(vids);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 索引保存成功\n");
    
    /* 销毁索引 */
    jw_index_destroy(index);
    
    /* 重新加载索引 */
    jw_index_t *loaded_index = jw_index_load(arena, &path);
    if (loaded_index == NULL) {
        printf("[ERROR] 加载索引失败\n");
        free(vectors);
        free(vids);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 索引加载成功\n");
    
    /* 测试加载后的索引 */
    found = jw_index_search(loaded_index, vectors, TEST_K, results);
    if (found == 0) {
        printf("[ERROR] 加载后的索引搜索失败\n");
        jw_index_destroy(loaded_index);
        free(vectors);
        free(vids);
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 加载后的索引搜索测试成功，找到 %zu 个结果\n", found);
    
    /* 清理 */
    jw_index_destroy(loaded_index);
    free(vectors);
    free(vids);
    jw_arena_destroy(arena);
    
    /* 删除测试文件 */
    unlink(TEST_INDEX_PATH);
    
    return JW_TRUE;
}

/* 测试MMAP模式 */
static jw_bool_t test_mmap_mode(void)
{
    printf("\n=== 测试MMAP模式 ===\n");
    
    jw_arena_t *arena = NULL;
    jw_arena_create(256 * 1024 * 1024, &arena);
    if (arena == NULL) {
        printf("[ERROR] 无法创建内存池\n");
        return JW_FALSE;
    }
    
    /* 创建MMAP存储 */
    jw_storage_config_t config = JW_STORAGE_CONFIG_DEFAULT;
    config.type = JW_STORAGE_TYPE_MMAP;
    config.path = (jw_str_t){ .ptr = TEST_MMAP_PATH, .slen = strlen(TEST_MMAP_PATH) };
    config.mode = JW_STORAGE_CREATE;
    config.sync_on_write = JW_TRUE;
    
    jw_storage_t *storage = jw_storage_create(arena, &config);
    if (storage == NULL) {
        printf("[ERROR] 无法创建MMAP存储\n");
        jw_arena_destroy(arena);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] MMAP存储创建成功\n");
    
    /* 写入测试数据 */
    jw_float32_t test_vector[TEST_DIM];
    generate_random_vectors(test_vector, 1, TEST_DIM);
    
    jw_uint64_t offset;
    jw_status_t status = jw_storage_write_vector(storage, 1, test_vector, TEST_DIM, &offset);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 写入向量失败: %d\n", status);
        jw_storage_close(storage);
        jw_arena_destroy(arena);
        unlink(TEST_MMAP_PATH);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 写入向量成功，偏移量: %llu\n", offset);
    
    /* 读取测试数据 */
    jw_uint64_t read_vid;
    jw_float32_t read_vector[TEST_DIM];
    status = jw_storage_read_vector(storage, offset, &read_vid, read_vector, TEST_DIM);
    if (status != JW_SUCCESS) {
        printf("[ERROR] 读取向量失败: %d\n", status);
        jw_storage_close(storage);
        jw_arena_destroy(arena);
        unlink(TEST_MMAP_PATH);
        return JW_FALSE;
    }
    
    printf("[SUCCESS] 读取向量成功，VID: %llu\n", read_vid);
    
    /* 验证数据 */
    jw_float32_t error = 0.0f;
    for (jw_dim_t i = 0; i < TEST_DIM; i++) {
        jw_float32_t diff = test_vector[i] - read_vector[i];
        error += diff * diff;
    }
    error = jw_math_sqrt_f32(error / TEST_DIM);
    
    printf("[SUCCESS] 数据验证成功，误差: %.6f\n", error);
    
    /* 清理 */
    jw_storage_close(storage);
    jw_arena_destroy(arena);
    
    /* 删除测试文件 */
    unlink(TEST_MMAP_PATH);
    
    return JW_TRUE;
}

/* 主测试函数 */
int main(int argc, char *argv[])
{
    printf("开始测试量化、序列化和MMAP模式...\n");
    
    /* 初始化随机数种子 */
    srand(42);
    
    /* 测试SQ量化 */
    if (!test_sq_quantization()) {
        printf("[FAILED] SQ量化测试失败\n");
        return 1;
    }
    
    /* 测试索引序列化和反序列化 */
    if (!test_index_serialization()) {
        printf("[FAILED] 索引序列化测试失败\n");
        return 1;
    }
    
    /* 测试MMAP模式 */
    if (!test_mmap_mode()) {
        printf("[FAILED] MMAP模式测试失败\n");
        return 1;
    }
    
    printf("\n=== 所有测试通过 ===\n");
    return 0;
}
