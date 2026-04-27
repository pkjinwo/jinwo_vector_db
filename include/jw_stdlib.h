/*
 * jw_stdlib.h - JinWo VecDB 标准库替代实现
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

#ifndef JW_STDLIB_H
#define JW_STDLIB_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 内存管理函数
 * =============================================================================
 */

/**
 * 分配内存
 *
 * @param size 内存大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_malloc(jw_size_t size);

/**
 * 分配并初始化内存
 *
 * @param count 元素数量
 * @param size 每个元素大小
 * @return 分配的内存指针，失败返回 NULL
 */
JW_API void *jw_calloc(jw_size_t count, jw_size_t size);

/**
 * 重新分配内存
 *
 * @param ptr 原内存指针
 * @param size 新内存大小
 * @return 重新分配的内存指针，失败返回 NULL
 */
JW_API void *jw_realloc(void *ptr, jw_size_t size);

/**
 * 释放内存
 *
 * @param ptr 内存指针
 */
JW_API void jw_free(void *ptr);

/*
 * =============================================================================
 * 随机数函数
 * =============================================================================
 */

/**
 * 设置随机数种子
 *
 * @param seed 种子值
 */
JW_API void jw_srand(jw_uint64_t seed);

/**
 * 生成随机数
 *
 * @return 64位随机数
 */
JW_API jw_uint64_t jw_rand(void);

/**
 * 生成 [0,1) 范围的浮点数
 *
 * @return 随机浮点数
 */
JW_API jw_float32_t jw_rand_float(void);

/**
 * 生成 [0,1) 范围的双精度浮点数
 *
 * @return 随机双精度浮点数
 */
JW_API jw_float64_t jw_rand_double(void);

/*
 * =============================================================================
 * 数学函数
 * =============================================================================
 */

/**
 * 计算整数绝对值
 *
 * @param x 输入值
 * @return 绝对值
 */
JW_API jw_int32_t jw_abs(jw_int32_t x);

/*
 * =============================================================================
 * 系统函数
 * =============================================================================
 */

/**
 * 终止程序
 *
 * @param status 退出状态
 */
JW_API void jw_exit(jw_int32_t status);

JW_END_DECL

#endif /* JW_STDLIB_H */
