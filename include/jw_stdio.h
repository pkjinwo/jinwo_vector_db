/*
 * jw_stdio.h - JinWo VecDB 标准输入输出替代
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

#ifndef JW_STDIO_H
#define JW_STDIO_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 格式化输出
 * =============================================================================
 */

/**
 * 可变参数格式化输出
 *
 * @param fmt 格式字符串
 * @param ap  可变参数列表
 * @return 输出字符数
 */
JW_API jw_int32_t jw_vprintf(const char *fmt, va_list ap);

/**
 * 格式化输出到标准输出
 *
 * 支持的格式说明符:
 *   %d  - 有符号十进制整数
 *   %u  - 无符号十进制整数
 *   %x  - 十六进制整数 (小写)
 *   %X  - 十六进制整数 (大写)
 *   %p  - 指针地址
 *   %s  - 字符串
 *   %c  - 字符
 *   %f  - 单精度浮点数
 *   %lf - 双精度浮点数
 *   %zu - size_t 类型
 *   %zd - ssize_t 类型
 *   %ld - long 类型
 *   %lu - unsigned long 类型
 *   %lld - long long 类型
 *   %llu - unsigned long long 类型
 *   %%  - 输出百分号
 *
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 输出字符数
 */
JW_API jw_int32_t jw_printf(const char *fmt, ...);

/**
 * 格式化输出到字符串
 *
 * @param str 目标字符串
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @return 输出的字符数 (不包含终止符)
 */
JW_API jw_int32_t jw_snprintf(char *str, jw_size_t size, const char *fmt, ...);

/**
 * 可变参数版本 jw_snprintf
 */
JW_API jw_int32_t jw_vsnprintf(char *str, jw_size_t size, const char *fmt, va_list ap);

/**
 * 输出一个字符到标准输出
 *
 * @param c 字符
 */
JW_API void jw_putchar(char c);

/**
 * 输出字符串到标准输出
 *
 * @param str 字符串
 */
JW_API void jw_puts(const char *str);

/**
 * 输出字符串到标准错误
 *
 * @param str 字符串
 */
JW_API void jw_puts_err(const char *str);

JW_END_DECL

#endif /* JW_STDIO_H */
