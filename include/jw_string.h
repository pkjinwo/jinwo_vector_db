/*
 * jw_string.h - JinWo VecDB 字符串工具
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
 * 字符串工具说明:
 * 
 * 提供字符串处理相关的工具函数，包括:
 *   - 字符串比较
 *   - 字符串复制
 *   - 字符串连接
 *   - 字符串分割
 *   - 字符串转换
 * 
 * 版本: 0.1.32
 * 作者: 灵活就业码农
 */

#ifndef JW_STRING_H
#define JW_STRING_H

#include "jw_types.h"
#include "jw_arena.h"

/* 前向声明 */


JW_BEGIN_DECL

/*
 * =============================================================================
 * 字符串比较
 * =============================================================================
 */

/**
 * 字符串比较 (区分大小写)
 * 
 * @param s1 字符串1
 * @param s2 字符串2
 * @return 0表示相等，负数表示s1小于s2，正数表示s1大于s2
 */
JW_API int jw_strcmp(const jw_str_t *s1, const jw_str_t *s2);

/**
 * 字符串比较 (不区分大小写)
 * 
 * @param s1 字符串1
 * @param s2 字符串2
 * @return 0表示相等，负数表示s1小于s2，正数表示s1大于s2
 */
JW_API int jw_strcasecmp(const jw_str_t *s1, const jw_str_t *s2);

/**
 * 字符串前缀比较
 * 
 * @param s 字符串
 * @param prefix 前缀
 * @return 0表示s以prefix开头
 */
JW_API int jw_strncmp(const jw_str_t *s, const jw_str_t *prefix, jw_size_t n);

/**
 * 字符串前缀比较 (不区分大小写)
 * 
 * @param s 字符串
 * @param prefix 前缀
 * @return 0表示s以prefix开头
 */
JW_API int jw_strncasecmp(const jw_str_t *s, const jw_str_t *prefix, jw_size_t n);

/*
 * =============================================================================
 * 字符串长度
 * =============================================================================
 */

/**
 * 获取字符串长度
 * 
 * @param s 字符串
 * @return 字符串长度
 */
JW_API jw_size_t jw_strlen(const jw_str_t *s);

/**
 * 获取字符串长度 (带长度限制)
 * 
 * @param s 字符串
 * @param max_len 最大长度
 * @return 字符串长度
 */
JW_API jw_size_t jw_strnlen(const jw_str_t *s, jw_size_t max_len);

/**
 * 获取C风格字符串指针
 * 
 * @param s 字符串
 * @return C风格字符串指针
 */
JW_API const char *jw_str_cstr(const jw_str_t *s);

/*
 * =============================================================================
 * 字符串复制
 * =============================================================================
 */

/**
 * 字符串复制
 * 
 * @param dest 目标字符串
 * @param src 源字符串
 * @return dest
 */
JW_API jw_str_t *jw_strcpy(jw_str_t *dest, const jw_str_t *src);

/**
 * 字符串复制 (带长度限制)
 * 
 * @param dest 目标字符串
 * @param src 源字符串
 * @param n 最大复制长度
 * @return dest
 */
JW_API jw_str_t *jw_strncpy(jw_str_t *dest, const jw_str_t *src, jw_size_t n);

/**
 * 字符串复制 (使用内存池)
 * 
 * @param arena 内存池
 * @param src 源字符串
 * @return 复制的字符串
 */
JW_API jw_str_t *jw_strdup(jw_arena_t *arena, const jw_str_t *src);

/*
 * =============================================================================
 * 字符串连接
 * =============================================================================
 */

/**
 * 字符串连接
 * 
 * @param dest 目标字符串
 * @param src 源字符串
 * @return dest
 */
JW_API jw_str_t *jw_strcat(jw_str_t *dest, const jw_str_t *src);

/**
 * 字符串连接 (带长度限制)
 * 
 * @param dest 目标字符串
 * @param src 源字符串
 * @param n 最大连接长度
 * @return dest
 */
JW_API jw_str_t *jw_strncat(jw_str_t *dest, const jw_str_t *src, jw_size_t n);

/**
 * 字符串格式化
 * 
 * @param arena 内存池
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 格式化后的字符串
 */
JW_API jw_str_t *jw_strprintf(jw_arena_t *arena, const char *format, ...);

/*
 * =============================================================================
 * 字符串查找
 * =============================================================================
 */

/**
 * 查找字符
 * 
 * @param s 字符串
 * @param c 字符
 * @return 字符位置，未找到返回NULL
 */
JW_API jw_size_t jw_strchr(const jw_str_t *s, int c);

/**
 * 反向查找字符
 * 
 * @param s 字符串
 * @param c 字符
 * @return 字符位置，未找到返回NULL
 */
JW_API jw_size_t jw_strrchr(const jw_str_t *s, int c);

/**
 * 查找子字符串
 * 
 * @param haystack 目标字符串
 * @param needle 子字符串
 * @return 子字符串位置，未找到返回-1
 */
JW_API jw_size_t jw_strstr(const jw_str_t *haystack, const jw_str_t *needle);

/*
 * =============================================================================
 * 字符串分割
 * =============================================================================
 */

/**
 * 字符串分割
 * 
 * @param str 字符串
 * @param delim 分隔符
 * @return 分割后的字符串数组，最后一个元素为NULL
 */
JW_API jw_str_t **jw_strsplit(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *delim);

/**
 * 字符串分割 (带最大分割数)
 * 
 * @param str 字符串
 * @param delim 分隔符
 * @param maxsplit 最大分割数
 * @return 分割后的字符串数组，最后一个元素为NULL
 */
JW_API jw_str_t **jw_strsplitn(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *delim, jw_size_t maxsplit);

/*
 * =============================================================================
 * 字符串转换
 * =============================================================================
 */

/**
 * 字符串转整数
 * 
 * @param str 字符串
 * @param endptr 结束位置 (可为NULL)
 * @param base 进制
 * @return 转换后的整数
 */
JW_API jw_int64_t jw_strtoll(const jw_str_t *str, jw_size_t *endptr, int base);

/**
 * 字符串转浮点数
 * 
 * @param str 字符串
 * @param endptr 结束位置 (可为NULL)
 * @return 转换后的浮点数
 */
JW_API jw_float64_t jw_strtod(const jw_str_t *str, jw_size_t *endptr);

/**
 * 整数转字符串
 * 
 * @param arena 内存池
 * @param value 整数值
 * @return 转换后的字符串
 */
JW_API jw_str_t *jw_lltostr(jw_arena_t *arena, jw_int64_t value);

/**
 * 浮点数转字符串
 * 
 * @param arena 内存池
 * @param value 浮点数值
 * @param precision 精度
 * @return 转换后的字符串
 */
JW_API jw_str_t *jw_dtostr(jw_arena_t *arena, jw_float64_t value, int precision);

/*
 * =============================================================================
 * 字符串操作
 * =============================================================================
 */

/**
 * 字符串修剪 (去除首尾空白字符)
 * 
 * @param arena 内存池
 * @param str 字符串
 * @return 修剪后的字符串
 */
JW_API jw_str_t *jw_strtrim(jw_arena_t *arena, const jw_str_t *str);

/**
 * 字符串转小写
 * 
 * @param arena 内存池
 * @param str 字符串
 * @return 转换后的字符串
 */
JW_API jw_str_t *jw_strtolower(jw_arena_t *arena, const jw_str_t *str);

/**
 * 字符串转大写
 * 
 * @param arena 内存池
 * @param str 字符串
 * @return 转换后的字符串
 */
JW_API jw_str_t *jw_strtoupper(jw_arena_t *arena, const jw_str_t *str);

/**
 * 字符串替换
 * 
 * @param arena 内存池
 * @param str 字符串
 * @param old_str 要替换的子字符串
 * @param new_str 新的子字符串
 * @return 替换后的字符串
 */
JW_API jw_str_t *jw_strreplace(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *old_str, const jw_str_t *new_str);

/*
 * =============================================================================
 * 安全字符串操作
 * =============================================================================
 */

/**
 * 安全的字符串复制
 * 
 * @param dest 目标字符串
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小
 * @return dest
 */
JW_API jw_str_t *jw_strlcpy(jw_str_t *dest, const jw_str_t *src, jw_size_t dest_size);

/**
 * 安全的字符串连接
 *
 * @param dest 目标字符串
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小
 * @return dest
 */
JW_API jw_str_t *jw_strlcat(jw_str_t *dest, const jw_str_t *src, jw_size_t dest_size);

/*
 * =============================================================================
 * 内存操作函数
 * =============================================================================
 *
 * 说明: jw_memcpy/jw_memset/jw_memmove 的声明已移至 jw_types.h，
 * 以避免循环依赖。此处仅保留注释。
 */

JW_END_DECL

#endif /* JW_STRING_H */
