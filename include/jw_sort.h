/*
 * jw_sort.h - JinWo VecDB 排序工具
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

#ifndef JW_SORT_H
#define JW_SORT_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 排序函数
 * =============================================================================
 */

/**
 * 快速排序
 *
 * @param base     数组起始地址
 * @param nmemb    数组元素个数
 * @param size     每个元素的大小 (字节)
 * @param compar   比较函数
 * @param user_data 用户数据 (传递给比较函数)
 */
JW_API void jw_qsort(void *base,
                     jw_size_t nmemb,
                     jw_size_t size,
                     jw_int32_t (*compar)(const void *, const void *, void *),
                     void *user_data);

/**
 * 简化版快速排序 (无 user_data)
 *
 * @param base   数组起始地址
 * @param nmemb  数组元素个数
 * @param size   每个元素的大小 (字节)
 * @param compar 比较函数
 */
JW_API void jw_qsort_simple(void *base,
                             jw_size_t nmemb,
                             jw_size_t size,
                             jw_int32_t (*compar)(const void *, const void *));

/**
 * 插入排序 (稳定，适用于小数组)
 *
 * @param base     数组起始地址
 * @param nmemb    数组元素个数
 * @param size     每个元素的大小 (字节)
 * @param compar   比较函数
 * @param user_data 用户数据 (传递给比较函数)
 */
JW_API void jw_insertion_sort(void *base,
                              jw_size_t nmemb,
                              jw_size_t size,
                              jw_int32_t (*compar)(const void *, const void *, void *),
                              void *user_data);

JW_END_DECL

#endif /* JW_SORT_H */
