/*
 * jw_string.c - JinWo VecDB 字符串工具实现
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

#include "jw_string.h"
#include "jw_arena.h"
#include "jw_stdarg.h"

/*
 * =============================================================================
 * 字符处理函数
 * =============================================================================
 */

static int jw_tolower(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static int jw_toupper(int c)
{
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

static int jw_isspace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

static int jw_isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

static int jw_isalpha(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/*
 * =============================================================================
 * 字符串比较
 * =============================================================================
 */

JW_API int jw_strcmp(const jw_str_t *s1, const jw_str_t *s2)
{
    jw_size_t min_len = (s1->slen < s2->slen) ? s1->slen : s2->slen;
    jw_size_t i = 0;
    
    while (i < min_len && s1->ptr[i] == s2->ptr[i]) {
        i++;
    }
    
    if (i < min_len) {
        return (unsigned char)s1->ptr[i] - (unsigned char)s2->ptr[i];
    }
    
    return (int)(s1->slen - s2->slen);
}

JW_API int jw_strcasecmp(const jw_str_t *s1, const jw_str_t *s2)
{
    jw_size_t min_len = (s1->slen < s2->slen) ? s1->slen : s2->slen;
    jw_size_t i = 0;
    
    while (i < min_len) {
        int c1 = jw_tolower((unsigned char)s1->ptr[i]);
        int c2 = jw_tolower((unsigned char)s2->ptr[i]);
        if (c1 != c2) {
            return c1 - c2;
        }
        i++;
    }
    
    return (int)(s1->slen - s2->slen);
}

JW_API int jw_strncmp(const jw_str_t *s, const jw_str_t *prefix, jw_size_t n)
{
    jw_size_t min_len = (s->slen < prefix->slen) ? s->slen : prefix->slen;
    jw_size_t compare_len = (min_len < n) ? min_len : n;
    jw_size_t i = 0;
    
    while (i < compare_len && s->ptr[i] == prefix->ptr[i]) {
        i++;
    }
    
    if (i < compare_len) {
        return (unsigned char)s->ptr[i] - (unsigned char)prefix->ptr[i];
    }
    
    if (i < n) {
        return (int)(s->slen - prefix->slen);
    }
    
    return 0;
}

JW_API int jw_strncasecmp(const jw_str_t *s, const jw_str_t *prefix, jw_size_t n)
{
    jw_size_t min_len = (s->slen < prefix->slen) ? s->slen : prefix->slen;
    jw_size_t compare_len = (min_len < n) ? min_len : n;
    jw_size_t i = 0;
    
    while (i < compare_len) {
        int c1 = jw_tolower((unsigned char)s->ptr[i]);
        int c2 = jw_tolower((unsigned char)prefix->ptr[i]);
        if (c1 != c2) {
            return c1 - c2;
        }
        i++;
    }
    
    if (i < n) {
        return (int)(s->slen - prefix->slen);
    }
    
    return 0;
}

/*
 * =============================================================================
 * 内存操作函数
 * =============================================================================
 */

void *jw_memcpy(void *dest, const void *src, jw_size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *jw_memset(void *s, int c, jw_size_t n)
{
    char *p = (char *)s;
    while (n--) {
        *p++ = (char)c;
    }
    return s;
}

void *jw_memmove(void *dest, const void *src, jw_size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if (d < s) {
        /* 从前往后复制 */
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        /* 从后往前复制 */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

static void *jw_memchr(const void *s, int c, jw_size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    while (n--) {
        if (*p == (unsigned char)c) {
            return (void *)p;
        }
        p++;
    }
    return NULL;
}

/*
 * =============================================================================
 * 字符串长度
 * =============================================================================
 */

JW_API jw_size_t jw_strlen(const jw_str_t *s)
{
    return s->slen;
}

JW_API jw_size_t jw_strnlen(const jw_str_t *s, jw_size_t max_len)
{
    return (s->slen < max_len) ? s->slen : max_len;
}

JW_API const char *jw_str_cstr(const jw_str_t *s)
{
    return s->ptr;
}

/*
 * =============================================================================
 * 字符串复制
 * =============================================================================
 */

JW_API jw_str_t *jw_strcpy(jw_str_t *dest, const jw_str_t *src)
{
    if (!dest || !src) {
        return dest;
    }
    
    jw_memcpy(dest->ptr, src->ptr, src->slen);
    dest->slen = src->slen;
    return dest;
}

JW_API jw_str_t *jw_strncpy(jw_str_t *dest, const jw_str_t *src, jw_size_t n)
{
    if (!dest || !src) {
        return dest;
    }
    
    jw_size_t copy_len = (src->slen < n) ? src->slen : n;
    jw_memcpy(dest->ptr, src->ptr, copy_len);
    dest->slen = copy_len;
    return dest;
}

JW_API jw_str_t *jw_strdup(jw_arena_t *arena, const jw_str_t *src)
{
    if (!src || !src->ptr) {
        return NULL;
    }
    
    jw_str_t *copy = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (copy) {
        copy->ptr = jw_arena_calloc(arena, src->slen, sizeof(char));
        if (copy->ptr) {
            jw_memcpy(copy->ptr, src->ptr, src->slen);
            copy->slen = src->slen;
        } else {
            jw_free(copy);
            copy = NULL;
        }
    }
    return copy;
}

/*
 * =============================================================================
 * 字符串连接
 * =============================================================================
 */

JW_API jw_str_t *jw_strcat(jw_str_t *dest, const jw_str_t *src)
{
    if (!dest || !src) {
        return dest;
    }
    
    jw_memcpy(dest->ptr + dest->slen, src->ptr, src->slen);
    dest->slen += src->slen;
    return dest;
}

JW_API jw_str_t *jw_strncat(jw_str_t *dest, const jw_str_t *src, jw_size_t n)
{
    if (!dest || !src) {
        return dest;
    }
    
    jw_size_t copy_len = (src->slen < n) ? src->slen : n;
    jw_memcpy(dest->ptr + dest->slen, src->ptr, copy_len);
    dest->slen += copy_len;
    return dest;
}

static int jw_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    char *p = str;
    const char *f = format;
    size_t remaining = size;
    
    if (remaining > 0) {
        remaining--; /* 留一个位置给终止符 */
    }
    
    while (*f && remaining > 0) {
        if (*f == '%') {
            f++;
            
            /* 处理格式说明符 */
            switch (*f) {
                case 'd':
                case 'i': {
                    jw_int64_t val = va_arg(args, jw_int64_t);
                    char buf[32];
                    char *bp = buf + sizeof(buf) - 1;
                    jw_bool_t negative = JW_FALSE;
                    
                    if (val < 0) {
                        negative = JW_TRUE;
                        val = -val;
                    }
                    
                    *bp = '\0';
                    do {
                        *--bp = (char)('0' + (val % 10));
                        val /= 10;
                    } while (val > 0);
                    
                    if (negative && remaining > 0) {
                        *p++ = '-';
                        remaining--;
                    }
                    
                    while (*bp && remaining > 0) {
                        *p++ = *bp++;
                        remaining--;
                    }
                    break;
                }
                
                case 'u': {
                    jw_uint64_t val = va_arg(args, jw_uint64_t);
                    char buf[32];
                    char *bp = buf + sizeof(buf) - 1;
                    
                    *bp = '\0';
                    do {
                        *--bp = (char)('0' + (val % 10));
                        val /= 10;
                    } while (val > 0);
                    
                    while (*bp && remaining > 0) {
                        *p++ = *bp++;
                        remaining--;
                    }
                    break;
                }
                
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (!s) {
                        s = "(null)";
                    }
                    while (*s && remaining > 0) {
                        *p++ = *s++;
                        remaining--;
                    }
                    break;
                }
                
                case 'c': {
                    int c = va_arg(args, int);
                    *p++ = (char)c;
                    remaining--;
                    break;
                }
                
                case 'f': {
                    jw_float64_t val = va_arg(args, jw_float64_t);
                    char buf[64];
                    char *bp = buf;
                    jw_bool_t negative = JW_FALSE;
                    
                    if (val < 0) {
                        negative = JW_TRUE;
                        val = -val;
                    }
                    
                    /* 处理整数部分 */
                    jw_int64_t int_part = (jw_int64_t)val;
                    jw_float64_t frac_part = val - int_part;
                    
                    if (negative && remaining > 0) {
                        *bp++ = '-';
                    }
                    
                    /* 整数部分转换 */
                    if (int_part == 0) {
                        *bp++ = '0';
                    } else {
                        char temp[32];
                        char *tp = temp + sizeof(temp) - 1;
                        *tp = '\0';
                        do {
                            *--tp = (char)('0' + (int_part % 10));
                            int_part /= 10;
                        } while (int_part > 0);
                        while (*tp) {
                            *bp++ = *tp++;
                        }
                    }
                    
                    /* 处理小数部分 */
                    *bp++ = '.';
                    for (int i = 0; i < 6; i++) {
                        frac_part *= 10;
                        int digit = (int)frac_part;
                        *bp++ = (char)('0' + digit);
                        frac_part -= digit;
                    }
                    
                    *bp = '\0';
                    
                    char *buf_ptr = buf;
                    while (*buf_ptr && remaining > 0) {
                        *p++ = *buf_ptr++;
                        remaining--;
                    }
                    break;
                }
                
                case '%': {
                    *p++ = '%';
                    remaining--;
                    break;
                }
                
                default:
                    *p++ = *f;
                    remaining--;
                    break;
            }
            f++;
        } else {
            *p++ = *f++;
            remaining--;
        }
    }
    
    if (size > 0) {
        *p = '\0';
    }
    
    return (int)(p - str);
}

JW_API jw_str_t *jw_strprintf(jw_arena_t *arena, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    /* 计算所需的缓冲区大小 */
    char dummy[1];
    int len = jw_vsnprintf(dummy, 1, format, args);
    va_end(args);
    
    if (len < 0) {
        return NULL;
    }
    
    /* 分配内存并格式化 */
    jw_str_t *str = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (str) {
        str->ptr = jw_arena_calloc(arena, len, sizeof(char));
        if (str->ptr) {
            va_start(args, format);
            jw_vsnprintf(str->ptr, len, format, args);
            va_end(args);
            str->slen = len;
        } else {
            jw_free(str);
            str = NULL;
        }
    }
    
    return str;
}

/*
 * =============================================================================
 * 字符串查找
 * =============================================================================
 */

JW_API jw_size_t jw_strchr(const jw_str_t *s, int c)
{
    for (jw_size_t i = 0; i < s->slen; i++) {
        if (s->ptr[i] == (char)c) {
            return i;
        }
    }
    return (jw_size_t)-1;
}

JW_API jw_size_t jw_strrchr(const jw_str_t *s, int c)
{
    for (jw_size_t i = s->slen - 1; i != (jw_size_t)-1; i--) {
        if (s->ptr[i] == (char)c) {
            return i;
        }
    }
    return (jw_size_t)-1;
}

JW_API jw_size_t jw_strstr(const jw_str_t *haystack, const jw_str_t *needle)
{
    if (needle->slen == 0) {
        return 0;
    }
    
    if (needle->slen > haystack->slen) {
        return (jw_size_t)-1;
    }
    
    for (jw_size_t i = 0; i <= haystack->slen - needle->slen; i++) {
        jw_size_t j = 0;
        while (j < needle->slen && haystack->ptr[i + j] == needle->ptr[j]) {
            j++;
        }
        if (j == needle->slen) {
            return i;
        }
    }
    
    return (jw_size_t)-1;
}

static jw_size_t jw_strpbrk(const jw_str_t *s, const jw_str_t *accept)
{
    for (jw_size_t i = 0; i < s->slen; i++) {
        for (jw_size_t j = 0; j < accept->slen; j++) {
            if (s->ptr[i] == accept->ptr[j]) {
                return i;
            }
        }
    }
    return (jw_size_t)-1;
}

/*
 * =============================================================================
 * 字符串分割
 * =============================================================================
 */

JW_API jw_str_t **jw_strsplit(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *delim)
{
    return jw_strsplitn(arena, str, delim, 0);
}

JW_API jw_str_t **jw_strsplitn(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *delim, jw_size_t maxsplit)
{
    if (!str || !str->ptr || !delim || !delim->ptr) {
        return NULL;
    }
    
    jw_size_t count = 0;
    jw_size_t pos = 0;
    
    /* 计算分割后的字符串数量 */
    while (pos < str->slen) {
        jw_size_t end = (jw_size_t)(strpbrk(str->ptr + pos, delim->ptr) - (str->ptr + pos));
        if (end == (jw_size_t)-1 || end >= str->slen - pos) {
            count++;
            break;
        }
        count++;
        if (maxsplit && count >= maxsplit) {
            break;
        }
        pos += end + 1;
    }
    
    /* 分配字符串数组 */
    jw_str_t **result = jw_arena_calloc(arena, count + 1, sizeof(jw_str_t *));
    if (!result) {
        return NULL;
    }
    
    /* 分割字符串 */
    pos = 0;
    jw_size_t idx = 0;
    
    while (pos < str->slen && (maxsplit == 0 || idx < maxsplit)) {
        jw_size_t end = (jw_size_t)(strpbrk(str->ptr + pos, delim->ptr) - (str->ptr + pos));
        if (end == (jw_size_t)-1 || end >= str->slen - pos) {
            jw_str_t *token = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
            if (token) {
                token->ptr = jw_arena_calloc(arena, str->slen - pos, sizeof(char));
                if (token->ptr) {
                    jw_memcpy(token->ptr, str->ptr + pos, str->slen - pos);
                    token->slen = str->slen - pos;
                } else {
                    jw_free(token);
                    token = NULL;
                }
            }
            result[idx++] = token;
            break;
        }
        
        jw_str_t *token = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
        if (token) {
            token->ptr = jw_arena_calloc(arena, end + 1, sizeof(char));
            if (token->ptr) {
                jw_memcpy(token->ptr, str->ptr + pos, end);
                token->ptr[end] = '\0';
                token->slen = end;
            } else {
                jw_free(token);
                token = NULL;
            }
        }
        result[idx++] = token;
        pos += end + 1;
    }
    
    result[idx] = NULL;
    return result;
}

/*
 * =============================================================================
 * 字符串转换
 * =============================================================================
 */

JW_API jw_int64_t jw_strtoll(const jw_str_t *str, jw_size_t *endptr, int base)
{
    jw_int64_t result = 0;
    jw_int64_t sign = 1;
    jw_size_t pos = 0;
    
    /* 跳过空白 */
    while (pos < str->slen && jw_isspace((unsigned char)str->ptr[pos])) {
        pos++;
    }
    
    /* 处理符号 */
    if (pos < str->slen && (str->ptr[pos] == '+' || str->ptr[pos] == '-')) {
        sign = (str->ptr[pos] == '-') ? -1 : 1;
        pos++;
    }
    
    /* 处理基数 */
    if (base == 0) {
        if (pos < str->slen && str->ptr[pos] == '0') {
            pos++;
            if (pos < str->slen && (str->ptr[pos] == 'x' || str->ptr[pos] == 'X')) {
                base = 16;
                pos++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }
    
    /* 转换数字 */
    while (pos < str->slen) {
        int digit;
        if (str->ptr[pos] >= '0' && str->ptr[pos] <= '9') {
            digit = str->ptr[pos] - '0';
        } else if (str->ptr[pos] >= 'a' && str->ptr[pos] <= 'z') {
            digit = str->ptr[pos] - 'a' + 10;
        } else if (str->ptr[pos] >= 'A' && str->ptr[pos] <= 'Z') {
            digit = str->ptr[pos] - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) {
            break;
        }
        
        result = result * base + digit;
        pos++;
    }
    
    if (endptr) {
        *endptr = pos;
    }
    
    return result * sign;
}

JW_API jw_float64_t jw_strtod(const jw_str_t *str, jw_size_t *endptr)
{
    jw_float64_t result = 0.0;
    jw_float64_t sign = 1.0;
    jw_float64_t fraction = 0.1;
    jw_bool_t has_fraction = JW_FALSE;
    jw_int64_t exponent = 0;
    jw_int64_t exp_sign = 1;
    jw_size_t pos = 0;
    
    /* 跳过空白 */
    while (pos < str->slen && jw_isspace((unsigned char)str->ptr[pos])) {
        pos++;
    }
    
    /* 处理符号 */
    if (pos < str->slen && (str->ptr[pos] == '+' || str->ptr[pos] == '-')) {
        sign = (str->ptr[pos] == '-') ? -1.0 : 1.0;
        pos++;
    }
    
    /* 处理整数部分 */
    while (pos < str->slen && str->ptr[pos] >= '0' && str->ptr[pos] <= '9') {
        result = result * 10.0 + (str->ptr[pos] - '0');
        pos++;
    }
    
    /* 处理小数部分 */
    if (pos < str->slen && str->ptr[pos] == '.') {
        has_fraction = JW_TRUE;
        pos++;
        while (pos < str->slen && str->ptr[pos] >= '0' && str->ptr[pos] <= '9') {
            result += (str->ptr[pos] - '0') * fraction;
            fraction *= 0.1;
            pos++;
        }
    }
    
    /* 处理指数部分 */
    if (pos < str->slen && (str->ptr[pos] == 'e' || str->ptr[pos] == 'E')) {
        pos++;
        if (pos < str->slen && (str->ptr[pos] == '+' || str->ptr[pos] == '-')) {
            exp_sign = (str->ptr[pos] == '-') ? -1 : 1;
            pos++;
        }
        while (pos < str->slen && str->ptr[pos] >= '0' && str->ptr[pos] <= '9') {
            exponent = exponent * 10 + (str->ptr[pos] - '0');
            pos++;
        }
    }
    
    /* 应用指数 */
    while (exponent > 0) {
        if (exp_sign > 0) {
            result *= 10.0;
        } else {
            result *= 0.1;
        }
        exponent--;
    }
    
    if (endptr) {
        *endptr = pos;
    }
    
    return result * sign;
}

JW_API jw_str_t *jw_lltostr(jw_arena_t *arena, jw_int64_t value)
{
    char buffer[32];
    char *p = buffer + sizeof(buffer) - 1;
    jw_bool_t negative = JW_FALSE;
    
    if (value < 0) {
        negative = JW_TRUE;
        value = -value;
    }
    
    *p = '\0';
    do {
        *--p = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0);
    
    if (negative) {
        *--p = '-';
    }
    
    jw_str_t *str = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (str) {
        str->slen = (jw_size_t)(buffer + sizeof(buffer) - 1 - p);
        str->ptr = jw_arena_calloc(arena, str->slen, sizeof(char));
        if (str->ptr) {
            jw_memcpy(str->ptr, p, str->slen);
        } else {
            str = NULL;
        }
    }
    return str;
}

JW_API jw_str_t *jw_dtostr(jw_arena_t *arena, jw_float64_t value, int precision)
{
    char buffer[64];
    char *p = buffer;
    jw_bool_t negative = JW_FALSE;
    
    if (value < 0) {
        negative = JW_TRUE;
        value = -value;
    }
    
    /* 处理整数部分 */
    jw_int64_t int_part = (jw_int64_t)value;
    jw_float64_t frac_part = value - int_part;
    
    if (negative) {
        *p++ = '-';
    }
    
    /* 整数部分转换 */
    if (int_part == 0) {
        *p++ = '0';
    } else {
        char temp[32];
        char *tp = temp + sizeof(temp) - 1;
        *tp = '\0';
        do {
            *--tp = (char)('0' + (int_part % 10));
            int_part /= 10;
        } while (int_part > 0);
        while (*tp) {
            *p++ = *tp++;
        }
    }
    
    /* 处理小数部分 */
    if (precision > 0) {
        *p++ = '.';
        while (precision-- > 0) {
            frac_part *= 10;
            int digit = (int)frac_part;
            *p++ = (char)('0' + digit);
            frac_part -= digit;
        }
    }
    
    *p = '\0';
    
    jw_str_t *str = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (str) {
        str->slen = (jw_size_t)(p - buffer);
        str->ptr = jw_arena_calloc(arena, str->slen, sizeof(char));
        if (str->ptr) {
            jw_memcpy(str->ptr, buffer, str->slen);
        } else {
            str = NULL;
        }
    }
    return str;
}

/*
 * =============================================================================
 * 字符串操作
 * =============================================================================
 */

JW_API jw_str_t *jw_strtrim(jw_arena_t *arena, const jw_str_t *str)
{
    if (!str || !str->ptr) {
        return NULL;
    }
    
    jw_size_t start = 0;
    /* 跳过前导空白 */
    while (start < str->slen && jw_isspace((unsigned char)str->ptr[start])) {
        start++;
    }
    
    jw_size_t end = str->slen;
    /* 找到末尾 */
    while (end > start && jw_isspace((unsigned char)str->ptr[end - 1])) {
        end--;
    }
    
    /* 复制修剪后的字符串 */
    jw_size_t len = end - start;
    jw_str_t *result = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (result) {
        result->ptr = jw_arena_calloc(arena, len, sizeof(char));
        if (result->ptr) {
            jw_memcpy(result->ptr, str->ptr + start, len);
            result->slen = len;
        } else {
            jw_free(result);
            result = NULL;
        }
    }
    
    return result;
}

JW_API jw_str_t *jw_strtolower(jw_arena_t *arena, const jw_str_t *str)
{
    if (!str || !str->ptr) {
        return NULL;
    }
    
    jw_str_t *result = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (result) {
        result->ptr = jw_arena_calloc(arena, str->slen, sizeof(char));
        if (result->ptr) {
            for (jw_size_t i = 0; i < str->slen; i++) {
                result->ptr[i] = (char)jw_tolower((unsigned char)str->ptr[i]);
            }
            result->slen = str->slen;
        } else {
            result = NULL;
        }
    }
    
    return result;
}

JW_API jw_str_t *jw_strtoupper(jw_arena_t *arena, const jw_str_t *str)
{
    if (!str || !str->ptr) {
        return NULL;
    }
    
    jw_str_t *result = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (result) {
        result->ptr = jw_arena_calloc(arena, str->slen, sizeof(char));
        if (result->ptr) {
            for (jw_size_t i = 0; i < str->slen; i++) {
                result->ptr[i] = (char)jw_toupper((unsigned char)str->ptr[i]);
            }
            result->slen = str->slen;
        } else {
            result = NULL;
        }
    }
    
    return result;
}

JW_API jw_str_t *jw_strreplace(jw_arena_t *arena, const jw_str_t *str, const jw_str_t *old_str, const jw_str_t *new_str)
{
    if (!str || !str->ptr || !old_str || !old_str->ptr || !new_str || !new_str->ptr) {
        return NULL;
    }
    
    jw_size_t old_len = old_str->slen;
    jw_size_t new_len = new_str->slen;
    
    if (old_len == 0) {
        return jw_strdup(arena, str);
    }
    
    /* 计算替换后的长度 */
    jw_size_t count = 0;
    jw_size_t pos = 0;
    while (pos <= str->slen - old_len) {
        jw_size_t i = 0;
        while (i < old_len && str->ptr[pos + i] == old_str->ptr[i]) {
            i++;
        }
        if (i == old_len) {
            count++;
            pos += old_len;
        } else {
            pos++;
        }
    }
    
    jw_size_t len = str->slen + count * (new_len - old_len);
    jw_str_t *result = jw_arena_calloc(arena, 1, sizeof(jw_str_t));
    if (!result) {
        return NULL;
    }
    
    result->ptr = jw_arena_calloc(arena, len, sizeof(char));
    if (!result->ptr) {
        result = NULL;
    }
    
    /* 执行替换 */
    char *dest = result->ptr;
    pos = 0;
    
    while (pos < str->slen) {
        if (pos <= str->slen - old_len) {
            jw_size_t i = 0;
            while (i < old_len && str->ptr[pos + i] == old_str->ptr[i]) {
                i++;
            }
            if (i == old_len) {
                jw_memcpy(dest, new_str->ptr, new_len);
                dest += new_len;
                pos += old_len;
                continue;
            }
        }
        *dest++ = str->ptr[pos++];
    }
    
    result->slen = len;
    return result;
}

/*
 * =============================================================================
 * 安全字符串操作
 * =============================================================================
 */

JW_API jw_str_t *jw_strlcpy(jw_str_t *dest, const jw_str_t *src, jw_size_t dest_size)
{
    if (!dest || !src) {
        return dest;
    }
    
    if (dest_size == 0) {
        dest->slen = 0;
        return dest;
    }
    
    jw_size_t copy_len = (src->slen < dest_size) ? src->slen : dest_size;
    jw_memcpy(dest->ptr, src->ptr, copy_len);
    dest->slen = copy_len;
    
    return dest;
}

JW_API jw_str_t *jw_strlcat(jw_str_t *dest, const jw_str_t *src, jw_size_t dest_size)
{
    if (!dest || !src) {
        return dest;
    }
    
    if (dest->slen >= dest_size) {
        return dest;
    }
    
    jw_size_t remaining = dest_size - dest->slen;
    jw_size_t copy_len = (src->slen < remaining) ? src->slen : remaining;
    
    jw_memcpy(dest->ptr + dest->slen, src->ptr, copy_len);
    dest->slen += copy_len;
    
    return dest;
}
