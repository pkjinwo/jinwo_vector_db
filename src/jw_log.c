/*
 * jw_log.c - JinWo VecDB 日志系统实现
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_log.h"
#include "jw_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#ifdef JW_WIN32
    #include <windows.h>
    #define strcasecmp _stricmp
#else
    #include <sys/time.h>
    #include <pthread.h>
#endif

/*
 * =============================================================================
 * 内部状态
 * =============================================================================
 */

/* 日志全局状态 */
static struct {
    jw_log_level_t level;           /* 当前日志级别 */
    jw_log_callback_t callback;     /* 用户回调函数 */
    void *callback_data;            /* 回调用户数据 */
    FILE *log_file;                 /* 日志文件句柄 */
    jw_bool_t console_enabled;      /* 控制台输出开关 */
    char format[256];               /* 日志格式 */
    jw_bool_t initialized;          /* 是否已初始化 */
#ifdef JW_WIN32
    CRITICAL_SECTION lock;          /* Windows临界区 */
#else
    pthread_mutex_t lock;           /* POSIX互斥锁 */
#endif
} g_log_state = {
    .level = JW_LOG_INFO,
    .callback = NULL,
    .callback_data = NULL,
    .log_file = NULL,
    .console_enabled = JW_TRUE,
    .format = "[%T] [%L] %F:%N %M: %m",
    .initialized = JW_FALSE
};

/* 日志级别名称表 */
static const char *level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/*
 * =============================================================================
 * 内部辅助函数
 * =============================================================================
 */

/* 初始化日志系统 (线程安全，只执行一次) */
static void ensure_initialized(void)
{
    if (g_log_state.initialized) {
        return;
    }
    
#ifdef JW_WIN32
    InitializeCriticalSection(&g_log_state.lock);
#else
    pthread_mutex_init(&g_log_state.lock, NULL);
#endif
    
    g_log_state.initialized = JW_TRUE;
}

/* 加锁 */
static void lock(void)
{
    ensure_initialized();
#ifdef JW_WIN32
    EnterCriticalSection(&g_log_state.lock);
#else
    pthread_mutex_lock(&g_log_state.lock);
#endif
}

/* 解锁 */
static void unlock(void)
{
#ifdef JW_WIN32
    LeaveCriticalSection(&g_log_state.lock);
#else
    pthread_mutex_unlock(&g_log_state.lock);
#endif
}

/* 格式化时间戳 */
static void format_timestamp(char *buf, jw_size_t size, jw_uint64_t timestamp)
{
    time_t seconds = (time_t)(timestamp / 1000);
    jw_uint32_t millis = (jw_uint32_t)(timestamp % 1000);
    struct tm *tm_info = localtime(&seconds);
    
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%03u",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             millis);
}

/* 获取时间戳 (毫秒) */
static jw_uint64_t get_timestamp_ms(void)
{
#ifdef JW_WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    /* 转换为Unix时间戳 (100纳秒 -> 毫秒) */
    return (uli.QuadPart - 116444736000000000ULL) / 10000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (jw_uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

/* 格式化日志消息 */
static void format_message(
    char *buf, 
    jw_size_t size,
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    const char *message)
{
    const char *src = g_log_state.format;
    char *dst = buf;
    jw_size_t remaining = size - 1;
    jw_uint64_t timestamp = get_timestamp_ms();
    char time_buf[32];
    
    /* 提取文件名 (不含路径) */
    const char *filename = strrchr(file, '/');
    if (filename == NULL) {
        filename = strrchr(file, '\\');
    }
    filename = (filename != NULL) ? filename + 1 : file;
    
    format_timestamp(time_buf, sizeof(time_buf), timestamp);
    
    while (*src && remaining > 0) {
        if (*src == '%' && *(src + 1)) {
            src++;
            int written = 0;
            
            switch (*src) {
                case 'T':   /* 时间戳 */
                    written = snprintf(dst, remaining, "%s", time_buf);
                    break;
                case 'L':   /* 日志级别 */
                    written = snprintf(dst, remaining, "%-5s", level_names[level]);
                    break;
                case 'F':   /* 文件名 */
                    written = snprintf(dst, remaining, "%s", filename);
                    break;
                case 'N':   /* 行号 */
                    written = snprintf(dst, remaining, "%d", line);
                    break;
                case 'M':   /* 函数名 */
                    written = snprintf(dst, remaining, "%s", func);
                    break;
                case 'm':   /* 消息 */
                    written = snprintf(dst, remaining, "%s", message);
                    break;
                case '%':   /* 百分号 */
                    *dst = '%';
                    written = 1;
                    break;
                default:
                    written = 0;
                    break;
            }
            
            if (written > 0) {
                dst += written;
                remaining -= written;
            }
            src++;
        } else {
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
}

/*
 * =============================================================================
 * 公共API实现
 * =============================================================================
 */

JW_API void jw_log_set_level(jw_log_level_t level)
{
    lock();
    g_log_state.level = level;
    unlock();
}

JW_API jw_log_level_t jw_log_get_level(void)
{
    return g_log_state.level;
}

JW_API void jw_log_set_callback(jw_log_callback_t callback, void *user_data)
{
    lock();
    g_log_state.callback = callback;
    g_log_state.callback_data = user_data;
    unlock();
}

JW_API jw_status_t jw_log_set_file(const char *filepath, jw_bool_t append)
{
    lock();
    
    /* 关闭现有文件 */
    if (g_log_state.log_file != NULL) {
        fclose(g_log_state.log_file);
        g_log_state.log_file = NULL;
    }
    
    /* 打开新文件 */
    if (filepath != NULL) {
        const char *mode = append ? "a" : "w";
        g_log_state.log_file = fopen(filepath, mode);
        if (g_log_state.log_file == NULL) {
            unlock();
            return JW_FILE_NOT_FOUND;
        }
    }
    
    unlock();
    return JW_SUCCESS;
}

JW_API void jw_log_set_console(jw_bool_t enable)
{
    lock();
    g_log_state.console_enabled = enable;
    unlock();
}

JW_API void jw_log_set_format(const char *format)
{
    lock();
    if (format != NULL) {
        strncpy(g_log_state.format, format, sizeof(g_log_state.format) - 1);
        g_log_state.format[sizeof(g_log_state.format) - 1] = '\0';
    }
    unlock();
}

JW_API void jw_log_write(
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    const char *fmt,
    ...)
{
    /* 检查级别 */
    if (level < g_log_state.level) {
        return;
    }
    
    /* 格式化用户消息 */
    char user_msg[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(user_msg, sizeof(user_msg), fmt, args);
    va_end(args);
    
    lock();
    
    /* 如果有用户回调，调用回调 */
    if (g_log_state.callback != NULL) {
        g_log_state.callback(
            level, file, line, func,
            get_timestamp_ms(),
            user_msg,
            g_log_state.callback_data
        );
        unlock();
        return;
    }
    
    /* 格式化完整日志行 */
    char log_line[4096];
    format_message(log_line, sizeof(log_line), level, file, line, func, user_msg);
    
    /* 输出到控制台 */
    if (g_log_state.console_enabled) {
        FILE *out = (level >= JW_LOG_ERROR) ? stderr : stdout;
        fprintf(out, "%s\n", log_line);
        fflush(out);
    }
    
    /* 输出到文件 */
    if (g_log_state.log_file != NULL) {
        fprintf(g_log_state.log_file, "%s\n", log_line);
        fflush(g_log_state.log_file);
    }
    
    unlock();
    
    /* FATAL级别退出程序 */
    if (level == JW_LOG_FATAL) {
        abort();
    }
}

JW_API void jw_log_write_v(
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    const char *fmt,
    va_list args)
{
    /* 检查级别 */
    if (level < g_log_state.level) {
        return;
    }
    
    /* 格式化用户消息 */
    char user_msg[2048];
    vsnprintf(user_msg, sizeof(user_msg), fmt, args);
    
    /* 调用主日志函数 */
    jw_log_write(level, file, line, func, "%s", user_msg);
}

JW_API const char *jw_log_level_string(jw_log_level_t level)
{
    if (level >= 0 && level < sizeof(level_names) / sizeof(level_names[0])) {
        return level_names[level];
    }
    return "UNKNOWN";
}

JW_API jw_log_level_t jw_log_level_from_string(const char *str)
{
    if (str == NULL) {
        return JW_LOG_INFO;
    }
    
    for (int i = 0; i < (int)(sizeof(level_names) / sizeof(level_names[0])); i++) {
        if (strcasecmp(str, level_names[i]) == 0) {
            return (jw_log_level_t)i;
        }
    }
    
    return JW_LOG_INFO;
}

JW_API jw_uint64_t jw_log_timestamp(void)
{
    return get_timestamp_ms();
}

JW_API void jw_log_flush(void)
{
    lock();
    
    if (g_log_state.console_enabled) {
        fflush(stdout);
        fflush(stderr);
    }
    
    if (g_log_state.log_file != NULL) {
        fflush(g_log_state.log_file);
    }
    
    unlock();
}

JW_API void jw_log_shutdown(void)
{
    lock();
    
    if (g_log_state.log_file != NULL) {
        fclose(g_log_state.log_file);
        g_log_state.log_file = NULL;
    }
    
    g_log_state.initialized = JW_FALSE;
    
    unlock();
    
#ifdef JW_WIN32
    DeleteCriticalSection(&g_log_state.lock);
#else
    pthread_mutex_destroy(&g_log_state.lock);
#endif
}