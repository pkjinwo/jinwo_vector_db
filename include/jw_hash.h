/*
 * jw_hash.h - JinWo VecDB 哈希工具
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
 * 哈希工具说明:
 * 
 * 提供哈希计算相关的工具函数，包括:
 *   - 字符串哈希
 *   - 整数哈希
 *   - 向量哈希
 *   - 哈希表实现
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_HASH_H
#define JW_HASH_H

#include "jw_types.h"
#include "jw_arena.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 哈希函数
 * =============================================================================
 */

/**
 * MurmurHash3 32位哈希函数
 * 
 * @param key 键值
 * @param len 长度
 * @param seed 种子
 * @return 32位哈希值
 */
JW_API jw_uint32_t jw_hash_murmur3_32(const void *key, jw_size_t len, jw_uint32_t seed);

/**
 * MurmurHash3 64位哈希函数
 * 
 * @param key 键值
 * @param len 长度
 * @param seed 种子
 * @return 64位哈希值
 */
JW_API jw_uint64_t jw_hash_murmur3_64(const void *key, jw_size_t len, jw_uint64_t seed);

/**
 * 字符串哈希
 * 
 * @param str 字符串
 * @return 哈希值
 */
JW_API jw_uint64_t jw_hash_string(const char *str);

/**
 * 整数哈希
 * 
 * @param value 整数值
 * @return 哈希值
 */
JW_API jw_uint64_t jw_hash_int(jw_int64_t value);

/**
 * 向量哈希
 * 
 * @param vec 向量
 * @param dim 维度
 * @return 哈希值
 */
JW_API jw_uint64_t jw_hash_vector(const jw_float32_t *vec, jw_size_t dim);

/*
 * =============================================================================
 * 哈希表
 * =============================================================================
 */

/**
 * 哈希表项
 */
typedef struct jw_hash_entry {
    void *key;                   /* 键 */
    void *value;                 /* 值 */
    struct jw_hash_entry *next;  /* 下一个元素 */
} jw_hash_entry_t;

/**
 * 哈希表
 */
typedef struct jw_hash_table {
    jw_arena_t *arena;             /* 内存池 */
    jw_hash_entry_t **buckets;   /* 桶数组 */
    jw_size_t size;              /* 桶数量 */
    jw_size_t count;             /* 元素数量 */
    
    /* 哈希函数 */
    jw_uint64_t (*hash_func)(const void *key);
    
    /* 比较函数 */
    int (*compare_func)(const void *key1, const void *key2);
    
    /* 键复制函数 */
    void *(*key_copy_func)(jw_arena_t *arena, const void *key);
    
    /* 值复制函数 */
    void *(*value_copy_func)(jw_arena_t *arena, const void *value);
}
jw_hash_table_t;

/**
 * 创建哈希表
 * 
 * @param arena 内存池
 * @param size 桶数量
 * @return 哈希表指针
 */
JW_API jw_hash_table_t *jw_hash_table_create(jw_arena_t *arena, jw_size_t size);

/**
 * 销毁哈希表
 * 
 * @param table 哈希表
 */
JW_API void jw_hash_table_destroy(jw_hash_table_t *table);

/**
 * 设置哈希函数
 * 
 * @param table 哈希表
 * @param hash_func 哈希函数
 */
JW_API void jw_hash_table_set_hash_func(jw_hash_table_t *table, jw_uint64_t (*hash_func)(const void *key));

/**
 * 设置比较函数
 * 
 * @param table 哈希表
 * @param compare_func 比较函数
 */
JW_API void jw_hash_table_set_compare_func(jw_hash_table_t *table, int (*compare_func)(const void *key1, const void *key2));

/**
 * 设置键复制函数
 * 
 * @param table 哈希表
 * @param key_copy_func 键复制函数
 */
JW_API void jw_hash_table_set_key_copy_func(jw_hash_table_t *table, void *(*key_copy_func)(jw_arena_t *arena, const void *key));

/**
 * 设置值复制函数
 * 
 * @param table 哈希表
 * @param value_copy_func 值复制函数
 */
JW_API void jw_hash_table_set_value_copy_func(jw_hash_table_t *table, void *(*value_copy_func)(jw_arena_t *arena, const void *value));

/**
 * 插入键值对
 * 
 * @param table 哈希表
 * @param key 键
 * @param value 值
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_hash_table_insert(jw_hash_table_t *table, const void *key, const void *value);

/**
 * 查找值
 * 
 * @param table 哈希表
 * @param key 键
 * @return 值，未找到返回NULL
 */
JW_API void *jw_hash_table_find(jw_hash_table_t *table, const void *key);

/**
 * 删除键值对
 * 
 * @param table 哈希表
 * @param key 键
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_hash_table_delete(jw_hash_table_t *table, const void *key);

/**
 * 获取哈希表大小
 * 
 * @param table 哈希表
 * @return 元素数量
 */
JW_API jw_size_t jw_hash_table_size(const jw_hash_table_t *table);

/**
 * 清空哈希表
 * 
 * @param table 哈希表
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_hash_table_clear(jw_hash_table_t *table);

/*
 * =============================================================================
 * 哈希表迭代器
 * =============================================================================
 */

/**
 * 哈希表迭代器
 */
typedef struct jw_hash_iterator {
    jw_hash_table_t *table;      /* 哈希表 */
    jw_size_t bucket_index;      /* 当前桶索引 */
    jw_hash_entry_t *current;    /* 当前元素 */
} jw_hash_iterator_t;

/**
 * 创建哈希表迭代器
 * 
 * @param table 哈希表
 * @return 迭代器指针
 */
JW_API jw_hash_iterator_t *jw_hash_iterator_create(jw_hash_table_t *table);

/**
 * 销毁哈希表迭代器
 * 
 * @param iter 迭代器
 */
JW_API void jw_hash_iterator_destroy(jw_hash_iterator_t *iter);

/**
 * 检查迭代器是否有效
 * 
 * @param iter 迭代器
 * @return JW_TRUE 有效
 */
JW_API jw_bool_t jw_hash_iterator_valid(const jw_hash_iterator_t *iter);

/**
 * 移动到下一个元素
 * 
 * @param iter 迭代器
 * @return JW_TRUE 成功
 */
JW_API jw_bool_t jw_hash_iterator_next(jw_hash_iterator_t *iter);

/**
 * 获取当前键
 * 
 * @param iter 迭代器
 * @return 键
 */
JW_API void *jw_hash_iterator_get_key(const jw_hash_iterator_t *iter);

/**
 * 获取当前值
 * 
 * @param iter 迭代器
 * @return 值
 */
JW_API void *jw_hash_iterator_get_value(const jw_hash_iterator_t *iter);

/*
 * =============================================================================
 * 便捷哈希表
 * =============================================================================
 */

/**
 * 创建字符串哈希表
 * 
 * @param arena 内存池
 * @param size 桶数量
 * @return 哈希表指针
 */
JW_API jw_hash_table_t *jw_hash_table_create_string(jw_arena_t *arena, jw_size_t size);

/**
 * 创建整数哈希表
 * 
 * @param arena 内存池
 * @param size 桶数量
 * @return 哈希表指针
 */
JW_API jw_hash_table_t *jw_hash_table_create_int(jw_arena_t *arena, jw_size_t size);

JW_END_DECL

#endif /* JW_HASH_H */
