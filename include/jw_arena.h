/*
 * jw_arena.h - JinWo VecDB 内存 arena 分配器
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
 * 内存 arena 分配器说明:
 * 
 * 采用简单的内存 arena 模式，特点：
 *   - 预分配大块内存
 *   - 顺序分配，无需复杂的内存管理
 *   - O(1) 时间复杂度的批量重置
 *   - 不支持单个内存块的释放
 *   - 适合临时内存需求和批量操作
 * 
 * 版本: 0.1.20
 * 作者: 灵活就业码农
 */

#ifndef JW_ARENA_H
#define JW_ARENA_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 内存 arena 结构
 * =============================================================================
 */

typedef struct jw_arena_t {
    void *start;      /* 内存块起始地址 */
    void *current;    /* 当前分配位置 */
    jw_size_t size;   /* 总大小 */
    jw_size_t used;   /* 已使用大小 */
} jw_arena_t;

/*
 * =============================================================================
 * 内存 arena API
 * =============================================================================
 */

/**
 * 创建内存 arena
 * 
 * @param size 预分配内存大小
 * @param arena 输出参数，返回创建的 arena
 * @return JW_SUCCESS 成功，其他失败
 */
JW_API jw_status_t jw_arena_create(jw_size_t size, jw_arena_t **arena);

/**
 * 销毁内存 arena
 * 
 * @param arena 内存 arena
 */
JW_API void jw_arena_destroy(jw_arena_t *arena);

/**
 * 分配内存
 * 
 * @param arena 内存 arena
 * @param size 分配大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_alloc(jw_arena_t *arena, jw_size_t size);

/**
 * 分配并清零内存
 * 
 * @param arena 内存 arena
 * @param count 元素数量
 * @param size 每个元素大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_calloc(jw_arena_t *arena, jw_size_t count, jw_size_t size);

/**
 * 复制内存
 * 
 * @param arena 内存 arena
 * @param src 源内存
 * @param size 复制大小
 * @return 复制后的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_memdup(jw_arena_t *arena, const void *src, jw_size_t size);

/**
 * 复制字符串
 * 
 * @param arena 内存 arena
 * @param src 源字符串
 * @return 复制后的字符串指针，失败返回 NULL
 */
JW_API char *jw_arena_strdup(jw_arena_t *arena, const char *src);

/**
 * 重置内存 arena
 * 
 * @param arena 内存 arena
 */
JW_API void jw_arena_reset(jw_arena_t *arena);

/**
 * 获取内存 arena 已使用大小
 * 
 * @param arena 内存 arena
 * @return 已使用大小
 */
JW_API jw_size_t jw_arena_get_used_size(const jw_arena_t *arena);

/**
 * 获取内存 arena 使用情况
 * 
 * @param arena 内存 arena
 * @param used 输出参数，已使用大小
 * @param total 输出参数，总大小
 */
JW_API void jw_arena_stat(const jw_arena_t *arena, jw_size_t *used, jw_size_t *total);

JW_END_DECL

#endif /* JW_ARENA_H */
