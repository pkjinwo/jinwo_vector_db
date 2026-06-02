/*
 * jw_log.h - JinWo VecDB 日志系统
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
 * 
 * =============================================================================
 * 
 * 日志系统设计说明:
 * 
 * 提供可配置的日志输出，支持:
 *   - 多级别日志: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
 *   - 多输出目标: 控制台、文件、自定义回调
 *   - 日志格式化: 时间戳、级别、源码位置
 *   - 运行时级别调整
 *   - 线程安全
 * 
 * 使用示例:
 *   jw_log_set_level(JW_LOG_INFO);
 *   jw_log_set_callback(my_log_handler, NULL);
 *   
 *   JW_LOG_INFO("Collection created: %s", name);
 *   JW_LOG_ERROR("Failed to open file: %s, error: %d", path, err);
 * 
 * 版本: 0.1.30
 * 作者: 灵活就业码农
 */

#ifndef JW_LOG_H
#define JW_LOG_H

#include "jw_types.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 日志级别定义
 * =============================================================================
 */

/**
 * 日志级别枚举
 */
typedef enum jw_log_level {
    JW_LOG_TRACE = 0,       /* 最详细，用于跟踪程序执行流程 */
    JW_LOG_DEBUG,           /* 调试信息，开发阶段使用 */
    JW_LOG_INFO,            /* 常规信息，生产环境默认级别 */
    JW_LOG_WARN,            /* 警告，不影响运行但需要关注 */
    JW_LOG_ERROR,           /* 错误，影响功能但程序可继续 */
    JW_LOG_FATAL,           /* 致命错误，程序无法继续 */
    JW_LOG_NONE = 100       /* 关闭所有日志 */
} jw_log_level_t;

/**
 * 日志级别名称
 */
#define JW_LOG_LEVEL_TRACE_STR  "TRACE"
#define JW_LOG_LEVEL_DEBUG_STR  "DEBUG"
#define JW_LOG_LEVEL_INFO_STR   "INFO"
#define JW_LOG_LEVEL_WARN_STR   "WARN"
#define JW_LOG_LEVEL_ERROR_STR  "ERROR"
#define JW_LOG_LEVEL_FATAL_STR  "FATAL"

/*
 * =============================================================================
 * 日志回调函数类型
 * =============================================================================
 */

/**
 * 日志回调函数类型
 * 
 * @param level     日志级别
 * @param file      源文件名
 * @param line      行号
 * @param func      函数名
 * @param timestamp 时间戳 (毫秒)
 * @param message   日志消息
 * @param user_data 用户数据
 */
typedef void (*jw_log_callback_t)(
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    jw_uint64_t timestamp,
    const char *message,
    void *user_data
);

/*
 * =============================================================================
 * 日志配置函数
 * =============================================================================
 */

/**
 * 设置日志级别
 * 
 * @param level 最低输出级别，低于此级别的日志不输出
 */
JW_API void jw_log_set_level(jw_log_level_t level);

/**
 * 获取当前日志级别
 * 
 * @return 当前日志级别
 */
JW_API jw_log_level_t jw_log_get_level(void);

/**
 * 设置日志回调函数
 * 
 * @param callback  回调函数，NULL表示使用默认输出
 * @param user_data 传递给回调函数的用户数据
 */
JW_API void jw_log_set_callback(jw_log_callback_t callback, void *user_data);

/**
 * 设置日志文件
 * 
 * @param filepath  日志文件路径，NULL表示关闭文件输出
 * @param append    是否追加模式，JW_TRUE追加，JW_FALSE覆盖
 * @return JW_SUCCESS成功，其他失败
 */
JW_API jw_status_t jw_log_set_file(const char *filepath, jw_bool_t append);

/**
 * 设置控制台输出
 * 
 * @param enable JW_TRUE启用，JW_FALSE禁用
 */
JW_API void jw_log_set_console(jw_bool_t enable);

/**
 * 设置日志格式
 * 
 * @param format 格式字符串
 *   支持的占位符:
 *   - %T: 时间戳 (YYYY-MM-DD HH:MM:SS.mmm)
 *   - %L: 日志级别
 *   - %F: 源文件名
 *   - %N: 行号
 *   - %M: 函数名
 *   - %m: 日志消息
 *   默认: "[%T] [%L] %F:%N %M: %m"
 */
JW_API void jw_log_set_format(const char *format);

/*
 * =============================================================================
 * 核心日志函数
 * =============================================================================
 */

/**
 * 输出日志 (内部使用，建议使用宏)
 * 
 * @param level 日志级别
 * @param file  源文件名
 * @param line  行号
 * @param func  函数名
 * @param fmt   格式字符串
 * @param ...   可变参数
 */
JW_API void jw_log_write(
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    const char *fmt,
    ...
);

/**
 * 输出日志 (va_list版本)
 */
JW_API void jw_log_write_v(
    jw_log_level_t level,
    const char *file,
    int line,
    const char *func,
    const char *fmt,
    va_list args
);

/*
 * =============================================================================
 * 日志宏 (推荐使用)
 * =============================================================================
 */

#define JW_LOG_TRACE(fmt, ...) \
    jw_log_write(JW_LOG_TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define JW_LOG_DEBUG(fmt, ...) \
    jw_log_write(JW_LOG_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define JW_LOG_INFO(fmt, ...) \
    jw_log_write(JW_LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define JW_LOG_WARN(fmt, ...) \
    jw_log_write(JW_LOG_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define JW_LOG_ERROR(fmt, ...) \
    jw_log_write(JW_LOG_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define JW_LOG_FATAL(fmt, ...) \
    jw_log_write(JW_LOG_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/*
 * =============================================================================
 * 条件日志宏 (性能优化)
 * =============================================================================
 */

#define JW_LOG_IF(cond, level, fmt, ...) \
    do { \
        if (cond) { \
            jw_log_write(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/* 调试模式下启用TRACE/DEBUG日志 */
#ifdef JW_DEBUG
    #define JW_LOG_TRACE_DEBUG(fmt, ...) JW_LOG_DEBUG(fmt, ##__VA_ARGS__)
#else
    #define JW_LOG_TRACE_DEBUG(fmt, ...) ((void)0)
#endif

/*
 * =============================================================================
 * 工具函数
 * =============================================================================
 */

/**
 * 获取日志级别名称字符串
 * 
 * @param level 日志级别
 * @return 级别名称字符串
 */
JW_API const char *jw_log_level_string(jw_log_level_t level);

/**
 * 从字符串解析日志级别
 * 
 * @param str 级别名称字符串
 * @return 日志级别，无效则返回 JW_LOG_INFO
 */
JW_API jw_log_level_t jw_log_level_from_string(const char *str);

/**
 * 获取当前时间戳 (毫秒)
 * 
 * @return 时间戳
 */
JW_API jw_uint64_t jw_log_timestamp(void);

/**
 * 刷新日志缓冲区
 */
JW_API void jw_log_flush(void);

/**
 * 关闭日志系统
 */
JW_API void jw_log_shutdown(void);

JW_END_DECL

#endif /* JW_LOG_H */
