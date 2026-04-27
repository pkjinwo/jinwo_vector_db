/*
 * jw_math.c - JinWo VecDB 数学工具实现
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_math.h"

JW_API jw_float32_t jw_math_abs_f32(jw_float32_t x)
{
    return (x < 0) ? -x : x;
}

JW_API jw_float64_t jw_math_abs_f64(jw_float64_t x)
{
    return (x < 0) ? -x : x;
}

JW_API jw_float32_t jw_math_sqrt_f32(jw_float32_t x)
{
    if (x <= 0) return 0;

    jw_float32_t result = x;
    jw_float32_t xhalf = 0.5f * x;

    union {
        jw_float32_t f;
        jw_int32_t i;
    } conv;

    conv.f = x;
    conv.i = (0x5f3759d5 - (conv.i >> 1));
    result = conv.f;
    result = result * (1.5f - xhalf * result * result);
    result = result * (1.5f - xhalf * result * result);
    result = result * (1.5f - xhalf * result * result);

    return x * result;
}

JW_API jw_float64_t jw_math_sqrt_f64(jw_float64_t x)
{
    if (x <= 0) return 0;
    
    jw_float64_t result = x;
    jw_float64_t xhalf = 0.5 * x;
    
    union {
        jw_float64_t f;
        jw_int64_t i;
    } conv;
    
    conv.f = x;
    conv.i = (0x5fe6eb50c7b537a9ULL - (conv.i >> 1));
    result = conv.f;
    result = result * (1.5 - xhalf * result * result);
    result = result * (1.5 - xhalf * result * result);
    result = result * (1.5 - xhalf * result * result);
    
    return x * result;
}

JW_API jw_float64_t jw_math_expm1(jw_float64_t x)
{
    if (x == 0) return 0;
    if (x > 50) return (jw_float64_t)1e20;
    if (x < -50) return -1;
    
    jw_float64_t result = 0;
    jw_float64_t term = x;
    
    for (int n = 1; n <= 100; n++) {
        result += term;
        term *= x / (n + 1);
        if (jw_math_abs_f64(term) < 1e-15) break;
    }
    
    return result;
}

JW_API jw_float64_t jw_math_log1p(jw_float64_t x)
{
    if (x == 0) return 0;
    if (x <= -1.0) return (jw_float64_t)-1e300;

    if (x > 0.001 || x < -0.001) {
        return jw_math_log_f64(1.0 + x);
    }

    jw_float64_t result = 0;
    jw_float64_t term = x;

    for (int n = 1; n <= 100; n++) {
        result += term / n;
        term *= -x;
        if (jw_math_abs_f64(term) < 1e-15) break;
    }

    return result;
}

JW_API jw_float32_t jw_math_log_f32(jw_float32_t x)
{
    if (x <= 0) return (jw_float32_t)-1e30f;
    
    int exponent = 0;
    while (x >= 2.0f) {
        x *= 0.5f;
        exponent++;
    }
    while (x < 1.0f) {
        x *= 2.0f;
        exponent--;
    }
    
    jw_float32_t y = (x - 1.0f) / (x + 1.0f);
    jw_float32_t y2 = y * y;
    jw_float32_t result = 0;
    jw_float32_t term = y;
    
    for (int n = 1; n <= 50; n += 2) {
        result += term / n;
        term *= y2;
    }
    
    result = 2.0f * result + (jw_float32_t)exponent * 0.6931471805599453f;
    
    return result;
}

JW_API jw_float64_t jw_math_log_f64(jw_float64_t x)
{
    if (x <= 0) return -1e300;
    
    int exponent = 0;
    while (x >= 2.0) {
        x *= 0.5;
        exponent++;
    }
    while (x < 1.0) {
        x *= 2.0;
        exponent--;
    }
    
    jw_float64_t y = (x - 1.0) / (x + 1.0);
    jw_float64_t y2 = y * y;
    jw_float64_t result = 0;
    jw_float64_t term = y;
    
    for (int n = 1; n <= 100; n += 2) {
        result += term / n;
        term *= y2;
    }
    
    result = 2.0 * result + (jw_float64_t)exponent * 0.6931471805599453;
    
    return result;
}

JW_API jw_float32_t jw_math_cos_f32(jw_float32_t x)
{
    while (x > 3.14159265f) x -= 6.28318531f;
    while (x < -3.14159265f) x += 6.28318531f;
    
    jw_float32_t result = 1.0f;
    jw_float32_t term = 1.0f;
    jw_float32_t x2 = x * x;
    
    for (int n = 1; n <= 20; n++) {
        term *= -x2 / ((jw_float32_t)(2 * n) * (jw_float32_t)(2 * n - 1));
        result += term;
        if (jw_math_abs_f32(term) < 1e-15f) break;
    }
    
    return result;
}

JW_API jw_float64_t jw_math_cos_f64(jw_float64_t x)
{
    while (x > 3.141592653589793) x -= 6.283185307179586;
    while (x < -3.141592653589793) x += 6.283185307179586;
    
    jw_float64_t result = 1.0;
    jw_float64_t term = 1.0;
    jw_float64_t x2 = x * x;
    
    for (int n = 1; n <= 100; n++) {
        term *= -x2 / ((jw_float64_t)(2 * n) * (jw_float64_t)(2 * n - 1));
        result += term;
        if (jw_math_abs_f64(term) < 1e-15) break;
    }
    
    return result;
}

JW_API jw_float32_t jw_math_sin_f32(jw_float32_t x)
{
    while (x > 3.14159265f) x -= 6.28318531f;
    while (x < -3.14159265f) x += 6.28318531f;
    
    jw_float32_t result = x;
    jw_float32_t term = x;
    jw_float32_t x2 = x * x;
    
    for (int n = 1; n <= 20; n++) {
        term *= -x2 / ((jw_float32_t)(2 * n) * (jw_float32_t)(2 * n + 1));
        result += term;
        if (jw_math_abs_f32(term) < 1e-15f) break;
    }
    
    return result;
}

JW_API jw_float64_t jw_math_sin_f64(jw_float64_t x)
{
    while (x > 3.141592653589793) x -= 6.283185307179586;
    while (x < -3.141592653589793) x += 6.283185307179586;
    
    jw_float64_t result = x;
    jw_float64_t term = x;
    jw_float64_t x2 = x * x;
    
    for (int n = 1; n <= 100; n++) {
        term *= -x2 / ((jw_float64_t)(2 * n) * (jw_float64_t)(2 * n + 1));
        result += term;
        if (jw_math_abs_f64(term) < 1e-15) break;
    }
    
    return result;
}

JW_API jw_float32_t jw_math_clamp_f32(jw_float32_t x, jw_float32_t min, jw_float32_t max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

JW_API jw_float64_t jw_math_clamp_f64(jw_float64_t x, jw_float64_t min, jw_float64_t max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}
