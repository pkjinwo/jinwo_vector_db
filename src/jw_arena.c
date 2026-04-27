/*
 * jw_arena.c - JinWo VecDB 内存 arena 分配器实现
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

#include "jw_arena.h"
#include "jw_types.h"
#include <stdlib.h>
#include <string.h>

/*
 * =============================================================================
 * 内存对齐宏
 * =============================================================================
 */

#define JW_ARENA_ALIGNMENT 8
#define JW_ARENA_ALIGN_SIZE(size) (((size) + (JW_ARENA_ALIGNMENT - 1)) & ~(JW_ARENA_ALIGNMENT - 1))

/*
 * =============================================================================
 * 内存 arena API 实现
 * =============================================================================
 */

JW_API jw_status_t jw_arena_create(jw_size_t size, jw_arena_t **arena)
{
    if (arena == NULL || size == 0) {
        return JW_INVALID_PARAM;
    }

    jw_arena_t *a = (jw_arena_t *)malloc(sizeof(jw_arena_t));
    if (a == NULL) {
        return JW_OUT_OF_MEMORY;
    }

    a->start = malloc(size);
    if (a->start == NULL) {
        free(a);
        return JW_OUT_OF_MEMORY;
    }

    a->current = a->start;
    a->size = size;
    a->used = 0;

    *arena = a;
    return JW_SUCCESS;
}

JW_API void jw_arena_destroy(jw_arena_t *arena)
{
    if (arena == NULL) {
        return;
    }

    if (arena->start != NULL) {
        free(arena->start);
    }
    free(arena);
}

JW_API void *jw_arena_alloc(jw_arena_t *arena, jw_size_t size)
{
    if (arena == NULL || size == 0) {
        return NULL;
    }

    size = JW_ARENA_ALIGN_SIZE(size);
    if (arena->used + size > arena->size) {
        return NULL;
    }

    void *ptr = arena->current;
    arena->current = (char *)arena->current + size;
    arena->used += size;

    return ptr;
}

JW_API void *jw_arena_calloc(jw_arena_t *arena, jw_size_t count, jw_size_t size)
{
    if (arena == NULL || count == 0 || size == 0) {
        return NULL;
    }

    jw_size_t total_size = count * size;
    void *ptr = jw_arena_alloc(arena, total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

JW_API void *jw_arena_memdup(jw_arena_t *arena, const void *src, jw_size_t size)
{
    if (arena == NULL || src == NULL || size == 0) {
        return NULL;
    }

    void *ptr = jw_arena_alloc(arena, size);
    if (ptr != NULL) {
        memcpy(ptr, src, size);
    }

    return ptr;
}

JW_API char *jw_arena_strdup(jw_arena_t *arena, const char *src)
{
    if (arena == NULL || src == NULL) {
        return NULL;
    }

    jw_size_t size = strlen(src) + 1;
    return (char *)jw_arena_memdup(arena, src, size);
}

JW_API void jw_arena_reset(jw_arena_t *arena)
{
    if (arena == NULL) {
        return;
    }

    arena->current = arena->start;
    arena->used = 0;
}

JW_API jw_size_t jw_arena_get_used_size(const jw_arena_t *arena)
{
    if (arena == NULL) {
        return 0;
    }

    return arena->used;
}

JW_API void jw_arena_stat(const jw_arena_t *arena, jw_size_t *used, jw_size_t *total)
{
    if (arena == NULL) {
        if (used != NULL) {
            *used = 0;
        }
        if (total != NULL) {
            *total = 0;
        }
        return;
    }

    if (used != NULL) {
        *used = arena->used;
    }
    if (total != NULL) {
        *total = arena->size;
    }
}