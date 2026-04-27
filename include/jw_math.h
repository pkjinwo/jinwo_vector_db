/*
 * jw_math.h - JinWo VecDB 数学工具
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

#ifndef JW_MATH_H
#define JW_MATH_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 数学基本运算
 * =============================================================================
 */

/**
 * 计算浮点数的绝对值 (float)
 *
 * @param x 输入值
 * @return 绝对值
 */
JW_API jw_float32_t jw_math_abs_f32(jw_float32_t x);

/**
 * 计算浮点数的绝对值 (double)
 *
 * @param x 输入值
 * @return 绝对值
 */
JW_API jw_float64_t jw_math_abs_f64(jw_float64_t x);

/**
 * 计算平方根 (float)
 *
 * @param x 输入值
 * @return 平方根
 */
JW_API jw_float32_t jw_math_sqrt_f32(jw_float32_t x);

/**
 * 计算平方根 (double)
 *
 * @param x 输入值
 * @return 平方根
 */
JW_API jw_float64_t jw_math_sqrt_f64(jw_float64_t x);

/**
 * 计算 exp(x) - 1，使用恒等式确保精度
 *
 * @param x 输入值
 * @return exp(x) - 1
 */
JW_API jw_float64_t jw_math_expm1(jw_float64_t x);

/**
 * 计算 log(1 + x)，使用恒等式确保精度
 *
 * @param x 输入值
 * @return log(1 + x)
 */
JW_API jw_float64_t jw_math_log1p(jw_float64_t x);

/**
 * 计算自然对数 (float)
 *
 * @param x 输入值 (x > 0)
 * @return ln(x)
 */
JW_API jw_float32_t jw_math_log_f32(jw_float32_t x);

/**
 * 计算自然对数 (double)
 *
 * @param x 输入值 (x > 0)
 * @return ln(x)
 */
JW_API jw_float64_t jw_math_log_f64(jw_float64_t x);

/**
 * 计算余弦 (float)
 *
 * @param x 输入值 (弧度)
 * @return cos(x)
 */
JW_API jw_float32_t jw_math_cos_f32(jw_float32_t x);

/**
 * 计算余弦 (double)
 *
 * @param x 输入值 (弧度)
 * @return cos(x)
 */
JW_API jw_float64_t jw_math_cos_f64(jw_float64_t x);

/**
 * 计算正弦 (float)
 *
 * @param x 输入值 (弧度)
 * @return sin(x)
 */
JW_API jw_float32_t jw_math_sin_f32(jw_float32_t x);

/**
 * 计算正弦 (double)
 *
 * @param x 输入值 (弧度)
 * @return sin(x)
 */
JW_API jw_float64_t jw_math_sin_f64(jw_float64_t x);

/**
 * 限制浮点数在指定范围内 (float)
 *
 * @param x 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
JW_API jw_float32_t jw_math_clamp_f32(jw_float32_t x, jw_float32_t min, jw_float32_t max);

/**
 * 限制浮点数在指定范围内 (double)
 *
 * @param x 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
JW_API jw_float64_t jw_math_clamp_f64(jw_float64_t x, jw_float64_t min, jw_float64_t max);

JW_END_DECL

#endif /* JW_MATH_H */
