/*
 * jw_stdarg.h - JinWo VecDB 可变参数支持
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

#ifndef JW_STDARG_H
#define JW_STDARG_H

#include <stdarg.h>

/*
 * 可变参数宏定义
 * 这里直接使用标准库的宏，因为它们是语言级别的特性
 */

#define JW_VA_LIST  va_list
#define JW_VA_START va_start
#define JW_VA_ARG   va_arg
#define JW_VA_END   va_end

/*
 * 类型别名，保持一致性
 */
typedef va_list jw_va_list;

#define jw_va_list  va_list
#define jw_va_start va_start
#define jw_va_arg   va_arg
#define jw_va_end   va_end

#endif /* JW_STDARG_H */
