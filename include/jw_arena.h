/*
 * jw_arena.h - JinWo VecDB 块式内存池
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
 * 块式内存池，用于管理生命周期相同的对象：
 *   - 每次分配从预申请的 block 中切分，不需要逐个 free
 *   - block 满了自动追加新 block，没有固定大小限制
 *   - 销毁时一次性释放所有 block
 * 
 * 与 malloc/free 的关系：
 *   jw_arena 的 block 底层用 malloc 申请，用 free 释放。
 *   arena 的价值在于"批量管理"——不需要追踪每个对象的生命周期。
 *   适合：collection、index 等长生命周期对象
 *   不适合：搜索结果等临时数据（用 jw_malloc/jw_free）
 * 
 * 典型用法：
 *   jw_arena_t *arena;
 *   jw_arena_create(4*1024*1024, &arena);
 *   
 *   void *data = jw_arena_alloc(arena, 1024);
 *   void *more = jw_arena_calloc(arena, 100, sizeof(int));
 *   // ... 不用关心每个 data 的释放 ...
 *   
 *   jw_arena_destroy(arena);  // 一次性释放全部
 * 
 * 注意：arena 操作本身不是线程安全的，调用者需要自行加锁。
 * 
 * 版本: 0.3.0
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

/* 内存 block（arena 内部的大块内存） */
typedef struct jw_arena_block {
    void   *data;                  /* 块内存起始地址 */
    jw_size_t size;                /* 块总大小 */
    jw_size_t used;                /* 已使用字节数 */
    struct jw_arena_block *next;   /* 链到下一个 block */
} jw_arena_block_t;

/* 块式内存池 */
typedef struct jw_arena_t {
    jw_arena_block_t *head;        /* 第一个 block */
    jw_arena_block_t *current;     /* 当前用于分配的 block（总是在链尾） */
    jw_size_t block_size;          /* 新 block 的默认大小 */
    jw_size_t total_size;          /* 所有 block 的总大小 */
    jw_size_t total_used;          /* 所有 block 的总已用大小 */
    const char *name;              /* arena 名称（调试用） */
} jw_arena_t;

/*
 * =============================================================================
 * 内存 arena API
 * =============================================================================
 */

/**
 * 创建内存 arena
 *
 * @param block_size 每个 block 的大小（默认 4MB），arena 会自动增长
 * @param arena      输出参数，返回创建的 arena
 * @return JW_SUCCESS 成功，其他失败
 */
JW_API jw_status_t jw_arena_create(jw_size_t block_size,
                                    jw_arena_t **arena);

/**
 * 销毁 arena
 *
 * 释放所有 block 内存和 arena 结构体本身。
 *
 * @param arena arena 指针
 */
JW_API void jw_arena_destroy(jw_arena_t *arena);

/**
 * 分配内存
 *
 * 从当前 block 分配。如果当前 block 空间不足，自动追加新 block。
 *
 * @param arena arena 指针
 * @param size  分配大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_alloc(jw_arena_t *arena, jw_size_t size);

/**
 * 分配并清零内存
 *
 * @param arena arena 指针
 * @param count 元素数量
 * @param size  每个元素大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_calloc(jw_arena_t *arena, jw_size_t count, jw_size_t size);

/**
 * 复制内存
 *
 * @param arena arena 指针
 * @param src   源内存
 * @param size  复制大小
 * @return 复制后的内存指针，失败返回 NULL
 */
JW_API void *jw_arena_memdup(jw_arena_t *arena, const void *src, jw_size_t size);

/**
 * 复制字符串
 *
 * @param arena arena 指针
 * @param src   源字符串
 * @return 复制后的字符串指针，失败返回 NULL
 */
JW_API char *jw_arena_strdup(jw_arena_t *arena, const char *src);

/**
 * 重置 arena
 *
 * 释放除第一个 block 外的所有 block，将第一个 block 的使用量归零。
 * 不会影响已经指向旧 block 的指针——调用者需要确保这些指针不再被使用。
 *
 * @param arena arena 指针
 */
JW_API void jw_arena_reset(jw_arena_t *arena);

/**
 * 获取 arena 已使用大小
 *
 * @param arena arena 指针
 * @return 已使用大小（字节）
 */
JW_API jw_size_t jw_arena_get_used_size(const jw_arena_t *arena);

/**
 * 获取 arena 使用情况
 *
 * @param arena arena 指针
 * @param used  输出：已使用大小
 * @param total 输出：总大小
 */
JW_API void jw_arena_stat(const jw_arena_t *arena, jw_size_t *used, jw_size_t *total);

/**
 * 获取 arena 名称
 *
 * @param arena arena 指针
 * @return 名称字符串，未设置时返回 "(unnamed)"
 */
JW_API const char *jw_arena_get_name(const jw_arena_t *arena);

/**
 * 获取当前 block 数量
 *
 * @param arena arena 指针
 * @return block 数量
 */
JW_API jw_size_t jw_arena_get_block_count(const jw_arena_t *arena);

JW_END_DECL

#endif /* JW_ARENA_H */
