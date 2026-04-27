/*
 * jw_error.h - JinWo VecDB 错误处理扩展
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef JW_ERROR_H
#define JW_ERROR_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 错误类别
 * =============================================================================
 */

typedef enum jw_error_category {
    JW_ERROR_CATEGORY_SUCCESS = 0,
    JW_ERROR_CATEGORY_GENERAL,      /* 通用错误 (1-99) */
    JW_ERROR_CATEGORY_VECDB,       /* 向量数据库错误 (100-199) */
    JW_ERROR_CATEGORY_STORAGE,     /* 存储相关错误 (200-299) */
    JW_ERROR_CATEGORY_CONCURRENCY, /* 并发相关错误 (300-399) */
    JW_ERROR_CATEGORY_UNKNOWN
} jw_error_category_t;

/*
 * =============================================================================
 * 详细错误信息结构
 * =============================================================================
 */

typedef struct jw_error_details {
    jw_status_t status;           /* 错误码 */
    jw_error_category_t category; /* 错误类别 */
    const char *message;         /* 简短错误消息 */
    const char *details;         /* 详细错误描述 */
    const char *file;            /* 发生错误的文件 */
    int line;                    /* 发生错误的行号 */
    const char *function;         /* 发生错误的函数 */
    jw_uint64_t timestamp;        /* 错误发生时间戳 */
} jw_error_details_t;

/*
 * =============================================================================
 * 错误处理 API
 * =============================================================================
 */

/**
 * 获取错误类别
 * @param status 错误码
 * @return 错误类别
 */
JW_API jw_error_category_t jw_error_get_category(jw_status_t status);

/**
 * 检查是否为成功状态
 * @param status 状态码
 * @return JW_TRUE 如果成功
 */
JW_API jw_bool_t jw_error_is_success(jw_status_t status);

/**
 * 检查是否为致命错误
 * @param status 状态码
 * @return JW_TRUE 如果是致命错误
 */
JW_API jw_bool_t jw_error_is_fatal(jw_status_t status);

/**
 * 获取错误的简短描述
 * @param status 错误码
 * @return 错误消息字符串
 */
JW_API const char *jw_error_message(jw_status_t status);

/**
 * 获取错误的详细描述
 * @param status 错误码
 * @return 详细错误描述字符串
 */
JW_API const char *jw_error_description(jw_status_t status);

/**
 * 格式化错误消息
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param status 错误码
 * @param format 附加格式字符串 (可为NULL)
 * @return 格式化的字符串长度
 */
JW_API jw_size_t jw_error_format(char *buffer,
                                  jw_size_t buffer_size,
                                  jw_status_t status,
                                  const char *format, ...);

/**
 * 获取错误类别名称
 * @param category 错误类别
 * @return 类别名称字符串
 */
JW_API const char *jw_error_category_name(jw_error_category_t category);

/**
 * 设置线程局部错误信息 (用于调试)
 * @param status 错误码
 * @param file 文件名
 * @param line 行号
 * @param function 函数名
 * @param details 详细错误信息 (可为NULL)
 */
JW_API void jw_error_set(jw_status_t status,
                          const char *file,
                          int line,
                          const char *function,
                          const char *details);

/**
 * 获取线程局部错误信息
 * @param details 输出错误详情
 */
JW_API void jw_error_get(jw_error_details_t *details);

/**
 * 清除线程局部错误信息
 */
JW_API void jw_error_clear(void);

/**
 * 检查是否设置了错误
 * @return JW_TRUE 如果有错误
 */
JW_API jw_bool_t jw_error_has_error(void);

/**
 * 获取上一个错误的 status (便捷宏)
 */
#define jw_error_status() (jw_error_has_error() ? \
    (jw_error_get_last_status()) : JW_SUCCESS)

JW_API jw_status_t jw_error_get_last_status(void);

/*
 * =============================================================================
 * 错误码辅助宏
 * =============================================================================
 */

/**
 * JW_ERROR_IF - 条件错误设置宏
 * 用法: JW_ERROR_IF(status != JW_SUCCESS, status, "insert failed");
 */
#define JW_ERROR_IF(cond, status, msg) \
    do { \
        if (cond) { \
            jw_error_set((status), __FILE__, __LINE__, __func__, (msg)); \
        } \
    } while(0)

/**
 * JW_ERROR_IF_NULL - 空指针检查宏
 * 用法: JW_ERROR_IF_NULL(ptr, JW_INVALID_PARAM, "ptr cannot be NULL");
 */
#define JW_ERROR_IF_NULL(ptr, status, msg) \
    JW_ERROR_IF(((ptr) == NULL), (status), (msg))

/**
 * JW_PROPAGATE_ERROR - 错误传播宏
 * 用法: JW_PROPAGATE_ERROR(status);
 *       如果status不是JW_SUCCESS，设置当前错误并跳转到error标签
 */
#define JW_PROPAGATE_ERROR(status) \
    do { \
        jw_status_t _jw_err = (status); \
        if (_jw_err != JW_SUCCESS) { \
            jw_error_set(_jw_err, __FILE__, __LINE__, __func__, NULL); \
            goto error; \
        } \
    } while(0)

/**
 * JW_RETURN_IF_ERROR - 错误返回宏
 * 用法: JW_RETURN_IF_ERROR(status);
 *       如果status不是JW_SUCCESS，立即返回
 */
#define JW_RETURN_IF_ERROR(status) \
    do { \
        if ((status) != JW_SUCCESS) { \
            jw_error_set((status), __FILE__, __LINE__, __func__, NULL); \
            return (status); \
        } \
    } while(0)

JW_END_DECL

#endif /* JW_ERROR_H */
