/*
 * jw_types.c - JinWo VecDB 类型定义和基础工具实现
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

#include "jw_types.h"
#include "jw_math.h"
#include "jw_stdarg.h"
#include "jw_stdlib.h"
#include "jw_string.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>

/*
 * =============================================================================
 * 全局变量
 * =============================================================================
 */

static jw_bool_t g_allocator_initialized = JW_FALSE;
static jw_allocator_t g_allocator = {
    .alloc = NULL,
    .realloc = NULL,
    .free = NULL,
    .user_data = NULL
};

static jw_bool_t g_simd_enabled = JW_TRUE;
static jw_bool_t g_simd_available = JW_FALSE;

/*
 * =============================================================================
 * 内部辅助函数
 * =============================================================================
 */

static void* default_alloc(size_t size, void* user_data)
{
    (void)user_data;
    return malloc(size);
}

static void* default_realloc(void* ptr, size_t size, void* user_data)
{
    (void)user_data;
    return realloc(ptr, size);
}

static void default_free(void* ptr, void* user_data)
{
    (void)user_data;
    free(ptr);
}

static void initialize_default_allocator(void)
{
    if (!g_allocator_initialized) {
        g_allocator = (
            jw_allocator_t
        ) {
            .alloc = default_alloc,
            .realloc = default_realloc,
            .free = default_free,
            .user_data = NULL
        };
        g_allocator_initialized = JW_TRUE;
    }
}

/*
 * =============================================================================
 * 内存分配 API
 * =============================================================================
 */

JW_API jw_allocator_t *jw_get_allocator(void)
{
    initialize_default_allocator();
    return &g_allocator;
}

JW_API jw_status_t jw_set_allocator(const jw_allocator_t *allocator)
{
    if (allocator == NULL) {
        return JW_INVALID_PARAM;
    }
    
    if (allocator->alloc == NULL) {
        return JW_INVALID_PARAM;
    }
    
    g_allocator = *allocator;
    g_allocator_initialized = JW_TRUE;
    
    return JW_SUCCESS;
}

JW_API void *jw_alloc(jw_size_t size)
{
    jw_allocator_t *alloc = jw_get_allocator();
    if (alloc->alloc == NULL) {
        return NULL;
    }
    return alloc->alloc(size, alloc->user_data);
}

JW_API void *jw_malloc(jw_size_t size)
{
    return jw_alloc(size);
}

JW_API void *jw_realloc(void *ptr, jw_size_t size)
{
    jw_allocator_t *alloc = jw_get_allocator();
    if (alloc->realloc == NULL) {
        return NULL;
    }
    return alloc->realloc(ptr, size, alloc->user_data);
}

JW_API void jw_free(void *ptr)
{
    jw_allocator_t *alloc = jw_get_allocator();
    if (alloc->free != NULL && ptr != NULL) {
        alloc->free(ptr, alloc->user_data);
    }
}

JW_API void *jw_calloc(jw_size_t count, jw_size_t size)
{
    jw_size_t total = count * size;
    void *ptr = jw_alloc(total);
    if (ptr != NULL) {
        jw_memset(ptr, 0, total);
    }
    return ptr;
}

/*
 * =============================================================================
 * 错误处理 API
 * =============================================================================
 */

static const char *g_status_messages[] = {
    "Success",                                     /* JW_SUCCESS */
    "Invalid argument",                           /* JW_INVALID_PARAM */
    "Out of memory",                              /* JW_OUT_OF_MEMORY */
    "File not found",                             /* JW_NOT_FOUND */
    "File I/O error",                             /* JW_IO_ERROR */
    "Index not found",                            /* JW_NOT_FOUND */
    "Collection not found",                       /* JW_NOT_FOUND */
    "Vector not found",                           /* JW_NOT_FOUND */
    "Invalid vector dimension",                   /* JW_INVALID_PARAM */
    "Invalid index type",                         /* JW_INVALID_PARAM */
    "Invalid operation",                          /* JW_INVALID_PARAM */
    "Duplicate key",                              /* JW_ALREADY_EXISTS */
    "Lock error",                                 /* JW_BUSY */
    "Network error",                              /* JW_NOT_SUPPORTED */
    "Timeout",                                    /* JW_TIMEOUT */
    "Unsupported feature",                       /* JW_NOT_SUPPORTED */
    "Internal error",                             /* JW_UNKNOWN_ERROR */
    "Unknown error",                              /* JW_UNKNOWN_ERROR */
};

#define JW_STATUS_MESSAGE_COUNT (sizeof(g_status_messages) / sizeof(g_status_messages[0]))

JW_API const char *jw_status_to_string(jw_status_t status)
{
    if (status >= 0 && status < JW_STATUS_MESSAGE_COUNT) {
        return g_status_messages[status];
    }
    return g_status_messages[JW_UNKNOWN_ERROR];
}

JW_API jw_status_t jw_status_from_errno(int errno_val)
{
    switch (errno_val) {
        case ENOENT:
            return JW_NOT_FOUND;
        case EIO:
            return JW_IO_ERROR;
        case ENOMEM:
            return JW_OUT_OF_MEMORY;
        case EINVAL:
            return JW_INVALID_PARAM;
        case EBUSY:
            return JW_BUSY;
        case ETIMEDOUT:
            return JW_TIMEOUT;
        default:
            return JW_UNKNOWN_ERROR;
    }
}

JW_API const char* jw_strerror(jw_status_t status)
{
    return jw_status_to_string(status);
}

/*
 * =============================================================================
 * 时间 API
 * =============================================================================
 */

JW_API jw_uint64_t jw_time_now(void)
{
    return (jw_uint64_t)time(NULL);
}

JW_API jw_time_t jw_time_now_ms(void)
{
#ifdef JW_WIN32
    return GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (jw_time_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

JW_API jw_time_t jw_time_diff(jw_time_t start, jw_time_t end)
{
    return end - start;
}

JW_API void jw_sleep(jw_uint32_t ms)
{
#ifdef JW_WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

/*
 * =============================================================================
 * 随机数 API
 * =============================================================================
 */

static jw_uint64_t g_rand_state = 12345;
static jw_bool_t g_rand_initialized = JW_FALSE;

JW_API void jw_srand(jw_uint64_t seed)
{
    g_rand_state = seed;
    g_rand_initialized = JW_TRUE;
}

JW_API jw_uint64_t jw_rand(void)
{
    if (!g_rand_initialized) {
        jw_srand(jw_time_now());
    }
    
    /* 线性同余生成器 */
    g_rand_state = 6364136223846793005ULL * g_rand_state + 1442695040888963407ULL;
    return g_rand_state;
}

JW_API jw_float32_t jw_rand_float(void)
{
    return (jw_float32_t)(jw_rand() >> 11) / (jw_float32_t)(1ull << 53);
}

JW_API jw_float64_t jw_rand_double(void)
{
    return (jw_float64_t)(jw_rand() >> 11) / (jw_float64_t)(1ull << 53);
}

JW_API jw_int32_t jw_abs(jw_int32_t x)
{
    return (x < 0) ? -x : x;
}

/*
 * =============================================================================
 * 系统 API
 * =============================================================================
 */

JW_API void jw_exit(jw_int32_t status)
{
    exit(status);
}

/*
 * =============================================================================
 * SIMD API
 * =============================================================================
 */

JW_API void jw_set_simd_enabled(jw_bool_t enable)
{
    g_simd_enabled = enable;
}

JW_API jw_bool_t jw_is_simd_available(void)
{
    return g_simd_available;
}

/*
 * =============================================================================
 * 平台相关 API
 * =============================================================================
 */

#ifdef JW_WIN32

#include <windows.h>

JW_API jw_status_t jw_platform_init(void)
{
    return JW_SUCCESS;
}

JW_API void jw_platform_shutdown(void)
{
}

JW_API jw_size_t jw_platform_get_page_size(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (jw_size_t)si.dwPageSize;
}

#else

#include <sys/mman.h>
#include <unistd.h>

JW_API jw_status_t jw_platform_init(void)
{
    return JW_SUCCESS;
}

JW_API void jw_platform_shutdown(void)
{
}

JW_API jw_size_t jw_platform_get_page_size(void)
{
    return (jw_size_t)sysconf(_SC_PAGESIZE);
}

#endif
