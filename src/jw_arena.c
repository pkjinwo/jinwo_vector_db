/*
 * jw_arena.c - JinWo VecDB 块式内存池实现
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
 * 设计要点：
 *   - Block 链式增长：一个 arena 可以有多个 block，满了自动追加
 *   - 分配策略：从 current block 分配，不够就追加新 block 到链尾
 *   - current 始终指向最后一个 block（链尾），确保分配是最快的
 *   - 销毁策略：释放所有 block 和 arena 结构体
 *   - 并非线程安全，调用者需要自行加锁
 */

#include "jw_arena.h"
#include "jw_types.h"
#include "jw_string.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __ANDROID__
#include <android/log.h>
#define JW_LOG_TAG "jinwo_vecdb"
#define JW_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, JW_LOG_TAG, __VA_ARGS__)
#else
#define JW_LOGE(...) ((void)0)
#endif

/* ========================================================================== */

#define JW_ARENA_ALIGNMENT         8
#define JW_ARENA_ALIGN(size)       (((size) + (JW_ARENA_ALIGNMENT - 1)) & ~(JW_ARENA_ALIGNMENT - 1))
#define JW_ARENA_DEFAULT_BLOCK_SIZE (4 * 1024 * 1024)   /* 4MB */
#define JW_ARENA_MIN_BLOCK_SIZE     (64 * 1024)          /* 64KB */

/* ========================================================================== */

/**
 * 分配一个新的 block
 */
static jw_arena_block_t *block_alloc(jw_size_t size)
{
    jw_arena_block_t *block = (jw_arena_block_t *)malloc(sizeof(jw_arena_block_t));
    if (block == NULL) return NULL;

    block->data = malloc(size);
    if (block->data == NULL) {
        free(block);
        return NULL;
    }

    block->size = size;
    block->used = 0;
    block->next = NULL;

    return block;
}

/**
 * 释放单个 block
 */
static void block_free(jw_arena_block_t *block)
{
    if (block == NULL) return;
    if (block->data != NULL) free(block->data);
    free(block);
}

/**
 * 追加新 block 到链尾，设为 current
 */
static jw_arena_block_t *arena_append_block(jw_arena_t *arena, jw_size_t min_size)
{
    jw_size_t sz = arena->block_size;
    if (sz < min_size)  sz = min_size;
    if (sz < JW_ARENA_MIN_BLOCK_SIZE) sz = JW_ARENA_MIN_BLOCK_SIZE;

    jw_arena_block_t *block = block_alloc(sz);
    if (block == NULL) return NULL;

    /* 追加到链尾 */
    if (arena->head == NULL) {
        arena->head = block;
    } else {
        /* current 始终指向链尾 */
        arena->current->next = block;
    }

    arena->current = block;
    arena->total_size += block->size;
    return block;
}

/* ==========================================================================
 * 公共 API
 * ========================================================================== */

JW_API jw_status_t jw_arena_create(jw_size_t block_size, jw_arena_t **arena)
{
    if (arena == NULL) return JW_INVALID_PARAM;

    if (block_size == 0) {
        block_size = JW_ARENA_DEFAULT_BLOCK_SIZE;
    }

    jw_arena_t *a = (jw_arena_t *)calloc(1, sizeof(jw_arena_t));
    if (a == NULL) return JW_OUT_OF_MEMORY;

    a->block_size = block_size;

    /* 预分配第一个 block */
    if (arena_append_block(a, block_size) == NULL) {
        free(a);
        return JW_OUT_OF_MEMORY;
    }

    *arena = a;
    return JW_SUCCESS;
}

JW_API void jw_arena_destroy(jw_arena_t *arena)
{
    if (arena == NULL) return;

    /* 释放所有 block */
    jw_arena_block_t *block = arena->head;
    while (block != NULL) {
        jw_arena_block_t *next = block->next;
        block_free(block);
        block = next;
    }

    free(arena);
}

JW_API void *jw_arena_alloc(jw_arena_t *arena, jw_size_t size)
{
    if (arena == NULL || size == 0) return NULL;

    size = JW_ARENA_ALIGN(size);

    /* current 始终是链尾，直接检查 */
    jw_arena_block_t *block = arena->current;
    if (block == NULL) {
        /* 不应该发生（create 时已分配第一个），但兜底 */
        block = arena_append_block(arena, size);
        if (block == NULL) return NULL;
    }

    if (block->used + size > block->size) {
        /* 当前 block 满了，追加新 block */
        block = arena_append_block(arena, size);
        if (block == NULL) return NULL;
    }

    void *ptr = (char *)block->data + block->used;
    block->used += size;
    arena->total_used += size;

    /** 验证返回地址确实在 block 范围内 */
    if ((char*)ptr < (char*)block->data || (char*)ptr + size > (char*)block->data + block->size) {
        JW_LOGE("jw_arena_alloc: RETURNED PTR OUT OF BLOCK BOUNDS!"
            " ptr=%p block_data=%p block_size=%" PRIu64 " used=%" PRIu64
            " size=%" PRIu64,
            ptr, block->data, (uint64_t)block->size,
            (uint64_t)(block->used - size), (uint64_t)size);
    }

    return ptr;
}

JW_API void *jw_arena_calloc(jw_arena_t *arena, jw_size_t count, jw_size_t size)
{
    if (arena == NULL || count == 0 || size == 0) return NULL;

    jw_size_t total = count * size;

    /* 溢出检查 */
    if (total / size != count) return NULL;

    void *ptr = jw_arena_alloc(arena, total);
    if (ptr != NULL) {
        jw_memset(ptr, 0, total);

        /** 验证 memset 确实执行了（防止编译器优化掉） */
        volatile jw_uint8_t *check = (volatile jw_uint8_t *)ptr;
        jw_bool_t not_zero = JW_FALSE;
        for (jw_size_t i = 0; i < total && i < 64; i++) {
            if (check[i] != 0) {
                not_zero = JW_TRUE;
                break;
            }
        }
        if (not_zero) {
            JW_LOGE("jw_arena_calloc: memset DID NOT ZERO!"
                " ptr=%p total=%" PRIu64, ptr, (uint64_t)total);
        }
    }
    return ptr;
}

JW_API void *jw_arena_memdup(jw_arena_t *arena, const void *src, jw_size_t size)
{
    if (arena == NULL || src == NULL || size == 0) return NULL;

    void *ptr = jw_arena_alloc(arena, size);
    if (ptr != NULL) {
        jw_memcpy(ptr, src, size);
    }
    return ptr;
}

JW_API char *jw_arena_strdup(jw_arena_t *arena, const char *src)
{
    if (arena == NULL || src == NULL) return NULL;

    size_t len = strlen(src);
    char *ptr = (char *)jw_arena_alloc(arena, (jw_size_t)(len + 1));
    if (ptr != NULL) {
        jw_memcpy(ptr, src, len + 1);
    }
    return ptr;
}

JW_API void jw_arena_reset(jw_arena_t *arena)
{
    if (arena == NULL) return;

    if (arena->head != NULL) {
        /* 释放除第一个以外的所有 block */
        jw_arena_block_t *extra = arena->head->next;
        while (extra != NULL) {
            jw_arena_block_t *next = extra->next;
            arena->total_size -= extra->size;
            block_free(extra);
            extra = next;
        }

        /* 重置第一个 block */
        arena->head->used = 0;
        arena->head->next = NULL;
        arena->current = arena->head;
        arena->total_used = 0;
        arena->total_size = arena->head->size;
    }
}

JW_API jw_size_t jw_arena_get_used_size(const jw_arena_t *arena)
{
    return (arena != NULL) ? arena->total_used : 0;
}

JW_API void jw_arena_stat(const jw_arena_t *arena, jw_size_t *used, jw_size_t *total)
{
    if (arena == NULL) {
        if (used)  *used  = 0;
        if (total) *total = 0;
        return;
    }
    if (used)  *used  = arena->total_used;
    if (total) *total = arena->total_size;
}

JW_API const char *jw_arena_get_name(const jw_arena_t *arena)
{
    if (arena == NULL || arena->name == NULL) return "(unnamed)";
    return arena->name;
}

JW_API jw_size_t jw_arena_get_block_count(const jw_arena_t *arena)
{
    if (arena == NULL) return 0;

    jw_size_t count = 0;
    jw_arena_block_t *block = arena->head;
    while (block != NULL) {
        count++;
        block = block->next;
    }
    return count;
}
