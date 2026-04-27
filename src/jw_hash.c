/*
 * jw_hash.c - JinWo VecDB 哈希工具实现
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

#include "jw_hash.h"
#include "jw_arena.h"
#include "jw_string.h"

/*
 * =============================================================================
 * 哈希函数
 * =============================================================================
 */

JW_API jw_uint32_t jw_hash_murmur3_32(const void *key, jw_size_t len, jw_uint32_t seed)
{
    const jw_uint8_t *data = (const jw_uint8_t *)key;
    const jw_size_t nblocks = len / 4;
    
    jw_uint32_t h1 = seed;
    
    const jw_uint32_t c1 = 0xcc9e2d51;
    const jw_uint32_t c2 = 0x1b873593;
    
    /* 处理4字节块 */
    const jw_uint32_t *blocks = (const jw_uint32_t *)(data + nblocks * 4);
    for (jw_size_t i = -nblocks; i; i++) {
        jw_uint32_t k1 = blocks[i];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }
    
    /* 处理剩余字节 */
    const jw_uint8_t *tail = (const jw_uint8_t *)(data + nblocks * 4);
    jw_uint32_t k1 = 0;
    
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1;
                k1 = (k1 << 15) | (k1 >> 17);
                k1 *= c2;
                h1 ^= k1;
    }
    
    /* 最终混合 */
    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    
    return h1;
}

JW_API jw_uint64_t jw_hash_murmur3_64(const void *key, jw_size_t len, jw_uint64_t seed)
{
    const jw_uint8_t *data = (const jw_uint8_t *)key;
    const jw_size_t nblocks = len / 16;

    jw_uint64_t h1 = seed;
    jw_uint64_t h2 = seed;

    const jw_uint64_t c1 = 0x87c37b91114253d5ULL;
    const jw_uint64_t c2 = 0x4cf5ad432745937fULL;

    const jw_uint64_t *blocks = (const jw_uint64_t *)data;
    for (jw_size_t i = 0; i < nblocks; i++) {
        jw_uint64_t k1 = blocks[i * 2];
        jw_uint64_t k2 = blocks[i * 2 + 1];

        k1 *= c1;
        k1 = (k1 << 31) | (k1 >> 33);
        k1 *= c2;
        h1 ^= k1;

        h1 = (h1 << 27) | (h1 >> 37);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = (k2 << 33) | (k2 >> 31);
        k2 *= c1;
        h2 ^= k2;

        h2 = (h2 << 31) | (h2 >> 33);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    const jw_uint8_t *tail = data + nblocks * 16;
    jw_uint64_t k1 = 0;
    jw_uint64_t k2 = 0;
    jw_size_t rest = len & 15;

    if (rest >= 8) {
        k1 = 0;
        k2 = 0;
        switch (rest) {
            case 15: k2 ^= ((jw_uint64_t)tail[14]) << 48;
            case 14: k2 ^= ((jw_uint64_t)tail[13]) << 40;
            case 13: k2 ^= ((jw_uint64_t)tail[12]) << 32;
            case 12: k2 ^= ((jw_uint64_t)tail[11]) << 24;
            case 11: k2 ^= ((jw_uint64_t)tail[10]) << 16;
            case 10: k2 ^= ((jw_uint64_t)tail[9]) << 8;
            case 9:  k2 ^= ((jw_uint64_t)tail[8]);
                     k2 *= c2;
                     k2 = (k2 << 33) | (k2 >> 31);
                     k2 *= c1;
                     h2 ^= k2;
            case 8:  k1 ^= ((jw_uint64_t)tail[7]) << 56;
            case 7:  k1 ^= ((jw_uint64_t)tail[6]) << 48;
            case 6:  k1 ^= ((jw_uint64_t)tail[5]) << 40;
            case 5:  k1 ^= ((jw_uint64_t)tail[4]) << 32;
            case 4:  k1 ^= ((jw_uint64_t)tail[3]) << 24;
            case 3:  k1 ^= ((jw_uint64_t)tail[2]) << 16;
            case 2:  k1 ^= ((jw_uint64_t)tail[1]) << 8;
            case 1:  k1 ^= ((jw_uint64_t)tail[0]);
                     k1 *= c1;
                     k1 = (k1 << 31) | (k1 >> 33);
                     k1 *= c2;
                     h1 ^= k1;
        }
    } else {
        k1 = 0;
        k2 = 0;
        switch (rest) {
            case 7:  k1 ^= ((jw_uint64_t)tail[6]) << 48;
            case 6:  k1 ^= ((jw_uint64_t)tail[5]) << 40;
            case 5:  k1 ^= ((jw_uint64_t)tail[4]) << 32;
            case 4:  k1 ^= ((jw_uint64_t)tail[3]) << 24;
            case 3:  k1 ^= ((jw_uint64_t)tail[2]) << 16;
            case 2:  k1 ^= ((jw_uint64_t)tail[1]) << 8;
            case 1:  k1 ^= ((jw_uint64_t)tail[0]);
                     k1 *= c1;
                     k1 = (k1 << 31) | (k1 >> 33);
                     k1 *= c2;
                     h1 ^= k1;
        }
    }

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53ULL;
    h1 ^= h1 >> 33;

    h2 ^= h2 >> 33;
    h2 *= 0xff51afd7ed558ccdULL;
    h2 ^= h2 >> 33;
    h2 *= 0xc4ceb9fe1a85ec53ULL;
    h2 ^= h2 >> 33;

    h1 += h2;

    return h1;
}

JW_API jw_uint64_t jw_hash_string(const char *str)
{
    if (str == NULL) {
        return 0;
    }
    jw_size_t len = 0;
    const char *p = str;
    while (*p++) {
        len++;
    }
    return jw_hash_murmur3_64(str, len, 0);
}

JW_API jw_uint64_t jw_hash_int(jw_int64_t value)
{
    return jw_hash_murmur3_64(&value, sizeof(value), 0);
}

JW_API jw_uint64_t jw_hash_vector(const jw_float32_t *vec, jw_size_t dim)
{
    return jw_hash_murmur3_64(vec, dim * sizeof(jw_float32_t), 0);
}

/*
 * =============================================================================
 * 哈希表
 * =============================================================================
 */

static jw_uint64_t default_hash_func(const void *key)
{
    return jw_hash_murmur3_64(key, sizeof(void *), 0);
}

static int default_compare_func(const void *key1, const void *key2)
{
    return (key1 == key2) ? 0 : 1;
}

static void *default_copy_func(jw_arena_t *arena, const void *value)
{
    return (void *)value;
}

JW_API jw_hash_table_t *jw_hash_table_create(jw_arena_t *arena, jw_size_t size)
{
    if (!arena || size == 0) {
        return NULL;
    }
    
    jw_hash_table_t *table = jw_arena_calloc(arena, 1, sizeof(jw_hash_table_t));
    if (!table) {
        return NULL;
    }
    
    table->arena = arena;
    table->size = size;
    table->count = 0;
    table->hash_func = default_hash_func;
    table->compare_func = default_compare_func;
    table->key_copy_func = default_copy_func;
    table->value_copy_func = default_copy_func;
    
    table->buckets = jw_arena_calloc(arena, size, sizeof(jw_hash_entry_t *));
    if (!table->buckets) {
        jw_free(table);
        return NULL;
    }
    
    return table;
}

JW_API void jw_hash_table_destroy(jw_hash_table_t *table)
{
    if (table) {
        /* 哈希表在内存池中分配，不需要单独释放 */
    }
}

JW_API void jw_hash_table_set_hash_func(jw_hash_table_t *table, jw_uint64_t (*hash_func)(const void *key))
{
    if (table && hash_func) {
        table->hash_func = hash_func;
    }
}

JW_API void jw_hash_table_set_compare_func(jw_hash_table_t *table, int (*compare_func)(const void *key1, const void *key2))
{
    if (table && compare_func) {
        table->compare_func = compare_func;
    }
}

JW_API void jw_hash_table_set_key_copy_func(jw_hash_table_t *table, void *(*key_copy_func)(jw_arena_t *arena, const void *key))
{
    if (table && key_copy_func) {
        table->key_copy_func = key_copy_func;
    }
}

JW_API void jw_hash_table_set_value_copy_func(jw_hash_table_t *table, void *(*value_copy_func)(jw_arena_t *arena, const void *value))
{
    if (table && value_copy_func) {
        table->value_copy_func = value_copy_func;
    }
}

JW_API jw_status_t jw_hash_table_insert(jw_hash_table_t *table, const void *key, const void *value)
{
    if (!table || !key) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t hash = table->hash_func(key);
    jw_size_t bucket = hash % table->size;
    
    /* 检查是否已存在 */
    jw_hash_entry_t *entry = table->buckets[bucket];
    while (entry) {
        if (table->compare_func(entry->key, key) == 0) {
            /* 更新值 */
            entry->value = table->value_copy_func(table->arena, value);
            return JW_SUCCESS;
        }
        entry = entry->next;
    }
    
    /* 创建新条目 */
    entry = jw_arena_calloc(table->arena, 1, sizeof(jw_hash_entry_t));
    if (!entry) {
        return JW_OUT_OF_MEMORY;
    }
    
    entry->key = table->key_copy_func(table->arena, key);
    entry->value = table->value_copy_func(table->arena, value);
    entry->next = table->buckets[bucket];
    table->buckets[bucket] = entry;
    table->count++;
    
    return JW_SUCCESS;
}

JW_API void *jw_hash_table_find(jw_hash_table_t *table, const void *key)
{
    if (!table || !key) {
        return NULL;
    }
    
    jw_uint64_t hash = table->hash_func(key);
    jw_size_t bucket = hash % table->size;
    
    jw_hash_entry_t *entry = table->buckets[bucket];
    while (entry) {
        if (table->compare_func(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

JW_API jw_status_t jw_hash_table_delete(jw_hash_table_t *table, const void *key)
{
    if (!table || !key) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t hash = table->hash_func(key);
    jw_size_t bucket = hash % table->size;
    
    jw_hash_entry_t *entry = table->buckets[bucket];
    jw_hash_entry_t *prev = NULL;
    
    while (entry) {
        if (table->compare_func(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[bucket] = entry->next;
            }
            table->count--;
            return JW_SUCCESS;
        }
        prev = entry;
        entry = entry->next;
    }
    
    return JW_NOT_FOUND;
}

JW_API jw_size_t jw_hash_table_size(const jw_hash_table_t *table)
{
    return table ? table->count : 0;
}

JW_API jw_status_t jw_hash_table_clear(jw_hash_table_t *table)
{
    if (!table) {
        return JW_INVALID_PARAM;
    }
    
    for (jw_size_t i = 0; i < table->size; i++) {
        table->buckets[i] = NULL;
    }
    
    table->count = 0;
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 哈希表迭代器
 * =============================================================================
 */

JW_API jw_hash_iterator_t *jw_hash_iterator_create(jw_hash_table_t *table)
{
    if (!table) {
        return NULL;
    }
    
    jw_hash_iterator_t *iter = jw_arena_calloc(table->arena, 1, sizeof(jw_hash_iterator_t));
    if (!iter) {
        return NULL;
    }
    
    iter->table = table;
    iter->bucket_index = 0;
    iter->current = NULL;
    
    /* 找到第一个元素 */
    while (iter->bucket_index < table->size) {
        iter->current = table->buckets[iter->bucket_index];
        if (iter->current) {
            break;
        }
        iter->bucket_index++;
    }
    
    return iter;
}

JW_API void jw_hash_iterator_destroy(jw_hash_iterator_t *iter)
{
    if (iter) {
        jw_free(iter);
    }
}

JW_API jw_bool_t jw_hash_iterator_valid(const jw_hash_iterator_t *iter)
{
    return (iter && iter->current) ? JW_TRUE : JW_FALSE;
}

JW_API jw_bool_t jw_hash_iterator_next(jw_hash_iterator_t *iter)
{
    if (!iter || !iter->current) {
        return JW_FALSE;
    }
    
    /* 移动到下一个元素 */
    iter->current = iter->current->next;
    
    /* 如果当前桶没有更多元素，移动到下一个桶 */
    while (!iter->current && iter->bucket_index < iter->table->size - 1) {
        iter->bucket_index++;
        iter->current = iter->table->buckets[iter->bucket_index];
    }
    
    return iter->current != NULL;
}

JW_API void *jw_hash_iterator_get_key(const jw_hash_iterator_t *iter)
{
    return (iter && iter->current) ? iter->current->key : NULL;
}

JW_API void *jw_hash_iterator_get_value(const jw_hash_iterator_t *iter)
{
    return (iter && iter->current) ? iter->current->value : NULL;
}

/*
 * =============================================================================
 * 便捷哈希表
 * =============================================================================
 */

static jw_uint64_t string_hash_func(const void *key)
{
    return jw_hash_string((const char *)key);
}

static int string_compare_func(const void *key1, const void *key2)
{
    const char *s1 = (const char *)key1;
    const char *s2 = (const char *)key2;
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

static void *string_copy_func(jw_arena_t *arena, const void *key)
{
    const char *str = (const char *)key;
    jw_size_t len = 0;
    const char *p = str;
    while (*p++) {
        len++;
    }
    char *copy = jw_arena_alloc(arena, len + 1);
    if (copy) {
        jw_memcpy(copy, str, len + 1);
    }
    return copy;
}

JW_API jw_hash_table_t *jw_hash_table_create_string(jw_arena_t *arena, jw_size_t size)
{
    jw_hash_table_t *table = jw_hash_table_create(arena, size);
    if (table) {
        jw_hash_table_set_hash_func(table, string_hash_func);
        jw_hash_table_set_compare_func(table, string_compare_func);
        jw_hash_table_set_key_copy_func(table, string_copy_func);
    }
    return table;
}

static jw_uint64_t int_hash_func(const void *key)
{
    return jw_hash_int(*(const jw_int64_t *)key);
}

static int int_compare_func(const void *key1, const void *key2)
{
    return (*(const jw_int64_t *)key1 == *(const jw_int64_t *)key2) ? 0 : 1;
}

static void *int_copy_func(jw_arena_t *arena, const void *key)
{
    jw_int64_t *copy = jw_arena_calloc(arena, 1, sizeof(jw_int64_t));
    if (copy) {
        *copy = *(const jw_int64_t *)key;
    }
    return copy;
}

JW_API jw_hash_table_t *jw_hash_table_create_int(jw_arena_t *arena, jw_size_t size)
{
    jw_hash_table_t *table = jw_hash_table_create(arena, size);
    if (table) {
        jw_hash_table_set_hash_func(table, int_hash_func);
        jw_hash_table_set_compare_func(table, int_compare_func);
        jw_hash_table_set_key_copy_func(table, int_copy_func);
    }
    return table;
}
