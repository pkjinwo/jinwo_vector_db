/*
 * jw_stdio.c - JinWo VecDB 标准输入输出替代实现
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_stdio.h"
#include "jw_stdarg.h"
#include "jw_string.h"
#include "jw_stdlib.h"
#include <stdio.h>

#ifdef JW_WIN32
    #include <windows.h>
    #define write(fd, buf, len) ((jw_int32_t)WriteFile((HANDLE)(fd), (buf), (DWORD)(len), NULL, NULL))
    #define stdout_handle GetStdHandle(STD_OUTPUT_HANDLE)
    #define stderr_handle GetStdHandle(STD_ERROR_HANDLE)
    #define INVALID_FD NULL
#else
    #include <unistd.h>
    #define stdout_handle 1
    #define stderr_handle 2
    #define INVALID_FD -1
#endif

/*
 * =============================================================================
 * 内部辅助函数
 * =============================================================================
 */

static jw_int32_t output_string(const char *str, jw_size_t len)
{
#ifdef JW_WIN32
    DWORD written = 0;
    WriteFile(stdout_handle, str, (DWORD)len, &written, NULL);
    return (jw_int32_t)written;
#else
    return (jw_int32_t)write(stdout_handle, str, len);
#endif
}

static jw_int32_t output_string_err(const char *str, jw_size_t len)
{
#ifdef JW_WIN32
    DWORD written = 0;
    WriteFile(stderr_handle, str, (DWORD)len, &written, NULL);
    return (jw_int32_t)written;
#else
    return (jw_int32_t)write(stderr_handle, str, len);
#endif
}

static void output_char(char c)
{
#ifdef JW_WIN32
    DWORD written = 0;
    WriteFile(stdout_handle, &c, 1, &written, NULL);
#else
    write(stdout_handle, &c, 1);
#endif
}

static jw_int32_t output_signed_int(jw_int64_t value, char *buf, jw_size_t width)
{
    char temp[32];
    char *p = temp + 32;
    jw_bool_t negative = (value < 0);

    if (value == 0) {
        *--p = '0';
    } else {
        jw_int64_t v = value;
        while (v != 0) {
            *--p = '0' + (char)(v < 0 ? -(v % 10) : v % 10);
            v /= 10;
        }
        if (negative) {
            *--p = '-';
        }
    }

    jw_size_t len = (jw_size_t)(temp + 32 - p);
    jw_size_t pad = (width > len) ? (width - len) : 0;

    jw_size_t i;
    for (i = 0; i < pad; i++) {
        buf[i] = ' ';
    }
    jw_memcpy(buf + pad, p, len);

    return (jw_int32_t)(pad + len);
}

static jw_int32_t output_unsigned_int(jw_uint64_t value, char *buf, int base, jw_size_t width, jw_bool_t uppercase)
{
    char temp[32];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char *p = temp + 32;

    if (value == 0) {
        *--p = '0';
    } else {
        jw_uint64_t v = value;
        while (v != 0) {
            *--p = digits[v % base];
            v /= base;
        }
    }

    jw_size_t len = (jw_size_t)(temp + 32 - p);
    jw_size_t pad = (width > len) ? (width - len) : 0;

    jw_size_t i;
    for (i = 0; i < pad; i++) {
        buf[i] = '0';
    }
    jw_memcpy(buf + pad, p, len);

    return (jw_int32_t)(pad + len);
}

static jw_int32_t output_float(double value, char *buf, int precision)
{
    if (precision < 0) {
        precision = 6;
    }

    char temp[64];
    char *p = temp;
    jw_bool_t negative = (value < 0);

    if (negative) {
        value = -value;
    }

    jw_int64_t int_part = (jw_int64_t)value;
    double frac_part = value - (double)int_part;

    p += output_signed_int(int_part, p, 0);
    *p++ = '.';

    for (int i = 0; i < precision; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        *p++ = '0' + digit;
        frac_part -= digit;
    }

    jw_size_t len = (jw_size_t)(p - temp);
    if (negative) {
        buf[0] = '-';
        jw_memcpy(buf + 1, temp, len);
        return (jw_int32_t)(len + 1);
    }

    jw_memcpy(buf, temp, len);
    return (jw_int32_t)len;
}

static const char *parse_format(const char *fmt, jw_size_t *width, int *precision, jw_bool_t *uppercase)
{
    *width = 0;
    *precision = -1;
    *uppercase = JW_FALSE;

    if (*fmt == '0') {
        fmt++;
    }

    while (*fmt >= '0' && *fmt <= '9') {
        *width = *width * 10 + (*fmt - '0');
        fmt++;
    }

    if (*fmt == '.') {
        fmt++;
        *precision = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            *precision = *precision * 10 + (*fmt - '0');
            fmt++;
        }
    }

    if (*fmt == 'l') {
        fmt++;
        if (*fmt == 'l') {
            fmt++;
        }
    }

    return fmt;
}

static jw_int32_t format_and_output(const char *fmt, jw_va_list *args)
{
    char buffer[128];
    jw_int32_t total = 0;
    char c;

    while ((c = *fmt++) != '\0') {
        if (c != '%') {
            output_char(c);
            total++;
            continue;
        }

        if (*fmt == '%') {
            output_char('%');
            total++;
            fmt++;
            continue;
        }

        jw_size_t width = 0;
        int precision = -1;
        jw_bool_t uppercase = JW_FALSE;

        fmt = parse_format(fmt, &width, &precision, &uppercase);

        c = *fmt++;
        jw_size_t len;

        switch (c) {
            case 'd':
            case 'i': {
                jw_int64_t value = jw_va_arg(*args, jw_int64_t);
                len = (jw_size_t)output_signed_int(value, buffer, width);
                break;
            }
            case 'u': {
                jw_uint64_t value = jw_va_arg(*args, jw_uint64_t);
                len = (jw_size_t)output_unsigned_int(value, buffer, 10, width, uppercase);
                break;
            }
            case 'x':
            case 'X': {
                jw_uint64_t value = jw_va_arg(*args, jw_uint64_t);
                uppercase = (c == 'X');
                len = (jw_size_t)output_unsigned_int(value, buffer, 16, width, uppercase);
                break;
            }
            case 'p': {
                jw_uint64_t value = (jw_uint64_t)(uintptr_t)jw_va_arg(*args, void*);
                len = (jw_size_t)output_unsigned_int(value, buffer, 16, sizeof(void*) * 2, uppercase);
                break;
            }
            case 's': {
                const char *str = jw_va_arg(*args, const char*);
                if (str == NULL) {
                    str = "(null)";
                }
                len = strlen(str);
                if (precision >= 0 && (jw_size_t)precision < len) {
                    len = (jw_size_t)precision;
                }
                jw_memcpy(buffer, str, len);
                break;
            }
            case 'c': {
                buffer[0] = (char)jw_va_arg(*args, int);
                len = 1;
                break;
            }
            case 'f':
            case 'F': {
                double value = jw_va_arg(*args, double);
                len = (jw_size_t)output_float(value, buffer, precision);
                break;
            }
            case 'l': {
                if (*fmt == 'd' || *fmt == 'i') {
                    long value = jw_va_arg(*args, long);
                    len = (jw_size_t)output_signed_int(value, buffer, width);
                    fmt++;
                } else if (*fmt == 'u') {
                    unsigned long value = jw_va_arg(*args, unsigned long);
                    len = (jw_size_t)output_unsigned_int(value, buffer, 10, width, uppercase);
                    fmt++;
                } else if (*fmt == 'l') {
                    fmt++;
                    if (*fmt == 'd' || *fmt == 'i') {
                        long long value = jw_va_arg(*args, long long);
                        len = (jw_size_t)output_signed_int(value, buffer, width);
                        fmt++;
                    } else if (*fmt == 'u') {
                        unsigned long long value = jw_va_arg(*args, unsigned long long);
                        len = (jw_size_t)output_unsigned_int(value, buffer, 10, width, uppercase);
                        fmt++;
                    } else {
                        continue;
                    }
                } else {
                    continue;
                }
                break;
            }
            case 'z': {
                if (*fmt == 'd' || *fmt == 'i') {
                    jw_int64_t value = (jw_int64_t)jw_va_arg(*args, jw_size_t);
                    len = (jw_size_t)output_signed_int(value, buffer, width);
                    fmt++;
                } else if (*fmt == 'u') {
                    jw_uint64_t value = jw_va_arg(*args, jw_size_t);
                    len = (jw_size_t)output_unsigned_int(value, buffer, 10, width, uppercase);
                    fmt++;
                } else {
                    continue;
                }
                break;
            }
            case '\0':
                goto end;
            default:
                continue;
        }

        output_string(buffer, len);
        total += (jw_int32_t)len;
    }

end:
    return total;
}

/*
 * =============================================================================
 * 公共 API 实现
 * =============================================================================
 */

JW_API jw_int32_t jw_vprintf(const char *fmt, jw_va_list ap)
{
    jw_va_list args;
    va_copy(args, ap);
    jw_int32_t result = format_and_output(fmt, &args);
    jw_va_end(args);
    return result;
}

JW_API jw_int32_t jw_printf(const char *fmt, ...)
{
    jw_va_list args;
    jw_va_start(args, fmt);
    jw_int32_t result = jw_vprintf(fmt, args);
    jw_va_end(args);
    return result;
}

JW_API jw_int32_t jw_vsnprintf(char *str, jw_size_t size, const char *fmt, jw_va_list ap)
{
    if (str == NULL || size == 0) {
        return 0;
    }

    jw_va_list args;
    va_copy(args, ap);

    jw_int32_t len = vsnprintf(str, size, fmt, args);

    jw_va_end(args);

    return len;
}

JW_API jw_int32_t jw_snprintf(char *str, jw_size_t size, const char *fmt, ...)
{
    jw_va_list args;
    jw_va_start(args, fmt);
    jw_int32_t result = jw_vsnprintf(str, size, fmt, args);
    jw_va_end(args);
    return result;
}

JW_API void jw_putchar(char c)
{
    output_char(c);
}

JW_API void jw_puts(const char *str)
{
    output_string(str, strlen(str));
    output_char('\n');
}

JW_API void jw_puts_err(const char *str)
{
    output_string_err(str, strlen(str));
    output_char('\n');
}