/*
 * jw_error.c - JinWo VecDB 错误处理扩展实现
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_error.h"
#include "jw_string.h"
#include "jw_stdio.h"
#include <stdio.h>
#include <string.h>

/*
 * =============================================================================
 * 错误消息定义
 * =============================================================================
 */

/* 通用错误消息 (1-99) */
static const char *g_general_messages[] = {
    "Success",                                 /* 0 */
    "Unknown error",                           /* -1 */
    "Invalid argument",                        /* -2 */
    "Out of memory",                          /* -3 */
    "Not found",                              /* -4 */
    "Already exists",                          /* -5 */
    "Buffer too small",                       /* -6 */
    "Not supported",                          /* -7 */
    "Permission denied",                       /* -8 */
    "Timeout",                                /* -9 */
    "Resource busy",                          /* -10 */
    "Empty object",                           /* -11 */
    "Too large",                              /* -12 */
    "Operation cancelled",                     /* -13 */
};

/* 向量数据库错误消息 (100-199) */
static const char *g_vecdb_messages[] = {
    "Invalid vector data",                   /* -100 */
    "Index file corrupted",                   /* -101 */
    "Collection is full",                    /* -102 */
    "Invalid vector dimension",               /* -103 */
    "Index not ready",                       /* -104 */
    "Vector ID already exists",              /* -105 */
    "Vector not found",                      /* -106 */
    "Collection already exists",             /* -107 */
    "Collection not found",                  /* -108 */
    "Index type mismatch",                   /* -109 */
    "Quantization error",                    /* -110 */
};

/* 存储相关错误消息 (200-299) */
static const char *g_storage_messages[] = {
    "File not found",                        /* -200 */
    "File corrupted",                        /* -201 */
    "Disk full",                             /* -202 */
    "I/O error",                             /* -203 */
    "File is locked",                        /* -204 */
    "Path too long",                         /* -205 */
    "Read-only mode",                        /* -206 */
};

/* 并发相关错误消息 (300-399) */
static const char *g_concurrency_messages[] = {
    "Lock acquisition timeout",              /* -300 */
    "Deadlock detected",                     /* -301 */
    "Thread creation failed",                /* -302 */
    "Mutex error",                           /* -303 */
};

/* 详细错误描述 */
static const char *g_error_descriptions[] = {
    /* 通用错误 */
    "An unknown error occurred",                              /* -1 */
    "The provided argument is invalid or out of range",      /* -2 */
    "Memory allocation failed - system is low on memory",    /* -3 */
    "The requested resource or item was not found",          /* -4 */
    "The item already exists and cannot be created",         /* -5 */
    "The provided buffer is too small to hold the data",     /* -6 */
    "This operation is not supported on this platform",      /* -7 */
    "Permission denied - check file access rights",          /* -8 */
    "Operation timed out - try again later",                 /* -9 */
    "The resource is busy - try again",                     /* -10 */
    "The object is empty and operation requires data",       /* -11 */
    "The data exceeds the maximum allowed size",            /* -12 */
    "The operation was cancelled by user",                  /* -13 */

    /* 向量数据库错误 */
    "The vector data is invalid (wrong dimension or NaN)",  /* -100 */
    "The index file is corrupted - rebuild required",        /* -101 */
    "The collection has reached maximum capacity",           /* -102 */
    "Vector dimension does not match collection dimension",   /* -103 */
    "Index is not built yet - call build() first",           /* -104 */
    "A vector with this ID already exists",                 /* -105 */
    "No vector found with the specified ID",                /* -106 */
    "A collection with this name already exists",           /* -107 */
    "No collection found with the specified name",         /* -108 */
    "Index type does not match collection configuration",    /* -109 */
    "Error during quantization training or encoding",        /* -110 */

    /* 存储错误 */
    "The specified file does not exist",                    /* -200 */
    "The file is corrupted and cannot be read",             /* -201 */
    "Disk is full - cannot write data",                    /* -202 */
    "An I/O error occurred during the operation",          /* -203 */
    "The file is locked by another process",               /* -204 */
    "The file path exceeds maximum allowed length",         /* -205 */
    "Database is opened in read-only mode",                /* -206 */

    /* 并发错误 */
    "Failed to acquire lock within timeout period",        /* -300 */
    "Deadlock detected - circular dependency",             /* -301 */
    "Failed to create or join thread",                     /* -302 */
    "Mutex operation failed",                              /* -303 */
};

/*
 * =============================================================================
 * 线程局部错误信息 (简化实现)
 * =============================================================================
 */

static jw_error_details_t g_thread_error = {
    .status = JW_SUCCESS,
    .category = JW_ERROR_CATEGORY_SUCCESS,
    .message = "Success",
    .details = NULL,
    .file = NULL,
    .line = 0,
    .function = NULL,
    .timestamp = 0
};

/*
 * =============================================================================
 * 公共 API 实现
 * =============================================================================
 */

JW_API jw_error_category_t jw_error_get_category(jw_status_t status)
{
    if (status >= 0) {
        return JW_ERROR_CATEGORY_SUCCESS;
    }

    if (status >= -99) {
        return JW_ERROR_CATEGORY_GENERAL;
    }

    if (status >= -199) {
        return JW_ERROR_CATEGORY_VECDB;
    }

    if (status >= -299) {
        return JW_ERROR_CATEGORY_STORAGE;
    }

    if (status >= -399) {
        return JW_ERROR_CATEGORY_CONCURRENCY;
    }

    return JW_ERROR_CATEGORY_UNKNOWN;
}

JW_API jw_bool_t jw_error_is_success(jw_status_t status)
{
    return (status == JW_SUCCESS) ? JW_TRUE : JW_FALSE;
}

JW_API jw_bool_t jw_error_is_fatal(jw_status_t status)
{
    switch (status) {
        case JW_OUT_OF_MEMORY:
        case JW_FILE_CORRUPTED:
        case JW_DEADLOCK:
        case JW_DISK_FULL:
            return JW_TRUE;
        default:
            return JW_FALSE;
    }
}

JW_API const char *jw_error_message(jw_status_t status)
{
    if (status >= 0) {
        return "Success";
    }

    if (status >= -13) {
        return g_general_messages[-status];
    }

    if (status >= -110) {
        return g_vecdb_messages[-(status + 100)];
    }

    if (status >= -206) {
        return g_storage_messages[-(status + 200)];
    }

    if (status >= -303) {
        return g_concurrency_messages[-(status + 300)];
    }

    return "Unknown error";
}

JW_API const char *jw_error_description(jw_status_t status)
{
    int index;

    if (status >= 0) {
        return "Operation completed successfully";
    }

    if (status >= -13) {
        index = -(status + 1);
    } else if (status >= -110) {
        index = 13 + -(status + 100);
    } else if (status >= -206) {
        index = 23 + -(status + 200);
    } else if (status >= -303) {
        index = 29 + -(status + 300);
    } else {
        return "An unknown error occurred";
    }

    if (index >= 0 && index < (int)(sizeof(g_error_descriptions) / sizeof(g_error_descriptions[0]))) {
        return g_error_descriptions[index];
    }

    return "An unknown error occurred";
}

JW_API const char *jw_error_category_name(jw_error_category_t category)
{
    switch (category) {
        case JW_ERROR_CATEGORY_SUCCESS:
            return "Success";
        case JW_ERROR_CATEGORY_GENERAL:
            return "General";
        case JW_ERROR_CATEGORY_VECDB:
            return "VectorDatabase";
        case JW_ERROR_CATEGORY_STORAGE:
            return "Storage";
        case JW_ERROR_CATEGORY_CONCURRENCY:
            return "Concurrency";
        default:
            return "Unknown";
    }
}

JW_API jw_size_t jw_error_format(char *buffer,
                                   jw_size_t buffer_size,
                                   jw_status_t status,
                                   const char *format, ...)
{
    int len = 0;
    jw_error_category_t category = jw_error_get_category(status);

    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }

    len = jw_snprintf(buffer, buffer_size,
                      "[%s] %s",
                      jw_error_category_name(category),
                      jw_error_message(status));

    if (format != NULL && len > 0 && (jw_size_t)len < buffer_size) {
        va_list args;
        va_start(args, format);
        int additional = jw_vsnprintf(buffer + len, buffer_size - len, format, args);
        va_end(args);
        if (additional > 0) {
            len += additional;
        }
    }

    return (jw_size_t)len;
}

JW_API void jw_error_set(jw_status_t status,
                           const char *file,
                           int line,
                           const char *function,
                           const char *details)
{
    g_thread_error.status = status;
    g_thread_error.category = jw_error_get_category(status);
    g_thread_error.message = jw_error_message(status);
    g_thread_error.details = details;
    g_thread_error.file = file;
    g_thread_error.line = line;
    g_thread_error.function = function;
    g_thread_error.timestamp = jw_time_now();
}

JW_API void jw_error_get(jw_error_details_t *details)
{
    if (details != NULL) {
        *details = g_thread_error;
    }
}

JW_API void jw_error_clear(void)
{
    g_thread_error.status = JW_SUCCESS;
    g_thread_error.category = JW_ERROR_CATEGORY_SUCCESS;
    g_thread_error.message = "Success";
    g_thread_error.details = NULL;
    g_thread_error.file = NULL;
    g_thread_error.line = 0;
    g_thread_error.function = NULL;
}

JW_API jw_bool_t jw_error_has_error(void)
{
    return (g_thread_error.status != JW_SUCCESS) ? JW_TRUE : JW_FALSE;
}

JW_API jw_status_t jw_error_get_last_status(void)
{
    return g_thread_error.status;
}
