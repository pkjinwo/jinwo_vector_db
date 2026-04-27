/*
 * jw_vecdb.h - JinWo VecDB 主接口
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
 * JinWo VecDB - 嵌入式向量数据库
 * 
 * 设计理念:
 *   - 类似SQLite的使用体验，无需独立服务进程
 *   - 零配置、零依赖、纯C实现
 *   - 跨平台支持: Linux, Android, iOS, macOS, Windows
 *   - Apache 2.0开源协议，可商用
 * 
 * 快速开始:
 *   // 1. 创建/打开数据库
 *   jw_vecdb_t *db;
 *   jw_vecdb_open("my_vecs.db", JW_VECDB_CREATE, &db);
 *   
 *   // 2. 创建Collection
 *   jw_collection_t *coll;
 *   jw_vecdb_create_collection(db, "documents", 1536, &coll);
 *   
 *   // 3. 插入向量
 *   float vec[1536] = {...};
 *   jw_vid_t vid;
 *   jw_collection_insert(coll, vec, &vid);
 *   
 *   // 4. 搜索
 *   jw_search_result_t results[10];
 *   jw_collection_search(coll, query_vec, 10, results);
 *   
 *   // 5. 关闭数据库
 *   jw_vecdb_close(db);
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_VECDB_H
#define JW_VECDB_H

#include "jw_types.h"
#include "jw_arena.h"
#include "jw_vector.h"
#include "jw_index.h"
#include "jw_collection.h"
#include "jw_storage.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 版本信息
 * =============================================================================
 */

/* 版本宏 */
#define JW_VECDB_VERSION_MAJOR      0
#define JW_VECDB_VERSION_MINOR      1
#define JW_VECDB_VERSION_PATCH      0
#define JW_VECDB_VERSION_STRING     "0.1.0"

/**
 * 获取版本字符串
 * @return 版本字符串，如 "JinWo VecDB 0.1.0"
 */
JW_API jw_str_t jw_vecdb_version(void);

/**
 * 获取构建信息
 * @return 构建信息字符串
 */
JW_API jw_str_t jw_vecdb_build_info(void);

/*
 * =============================================================================
 * 数据库配置
 * =============================================================================
 */

/**
 * 数据库打开标志
 */
typedef enum jw_vecdb_flag {
    JW_VECDB_READONLY    = 0x01,    /* 只读模式 */
    JW_VECDB_READWRITE   = 0x02,    /* 读写模式 */
    JW_VECDB_CREATE      = 0x04,    /* 不存在则创建 */
    JW_VECDB_TRUNCATE    = 0x08,    /* 存在则清空 */
    JW_VECDB_MEMORY      = 0x10,    /* 内存数据库 */
    JW_VECDB_NOMMAP      = 0x20,    /* 禁用mmap */
    JW_VECDB_NOCACHE     = 0x40,    /* 禁用缓存 */
    JW_VECDB_SYNC        = 0x80,    /* 同步写入 */
} jw_vecdb_flag_t;

/**
 * 数据库配置参数
 */
typedef struct jw_vecdb_config {
    /* 基本配置 */
    jw_str_t           db_path;            /* 数据库路径 */
    jw_storage_mode_t  storage_mode;       /* 存储模式 */

    /* 内存配置 */
    jw_size_t          arena_size;          /* 内存池大小 (字节) */
    jw_size_t          cache_size;         /* 缓存大小 (字节) */

    /* 模式配置 */
    jw_bool_t          read_only;          /* 只读模式 */
    jw_bool_t          create_if_missing;  /* 不存在时创建 */
} jw_vecdb_config_t;

/*
 * =============================================================================
 * 数据库句柄
 * =============================================================================
 */

/*
 * =============================================================================
 * 数据库生命周期 API
 * =============================================================================
 */

/**
 * 打开数据库
 * 
 * 简化接口，使用默认配置
 * 
 * @param path 数据库路径 (空字符串表示内存数据库)
 * @param flags 打开标志
 * @param db    输出数据库句柄
 * @return JW_SUCCESS 成功
 * 
 * 示例:
 *   jw_vecdb_t *db;
 *   jw_status_t status = jw_vecdb_open(jw_str("mydb.jwv"), JW_VECDB_CREATE, &db);
 */
JW_API jw_status_t jw_vecdb_open(const jw_str_t *path,
                                  jw_uint32_t flags,
                                  jw_vecdb_t **db);

/**
 * 使用详细配置打开数据库
 * 
 * @param config 配置参数
 * @param db     输出数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_open_ex(const jw_vecdb_config_t *config,
                                     jw_vecdb_t **db);

/**
 * 关闭数据库
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 * 
 * 注意: 关闭前会自动同步所有数据
 */
JW_API jw_status_t jw_vecdb_close(jw_vecdb_t *db);

/**
 * 同步数据库到磁盘
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_sync(jw_vecdb_t *db);

/**
 * 检查数据库是否已打开
 * 
 * @param db 数据库句柄
 * @return JW_TRUE 已打开
 */
JW_API jw_bool_t jw_vecdb_is_open(const jw_vecdb_t *db);

/*
 * =============================================================================
 * Collection 管理 API
 * =============================================================================
 */

/**
 * 创建Collection
 * 
 * @param db    数据库句柄
 * @param name  Collection名称
 * @param dim   向量维度
 * @param coll  输出Collection指针 (可为NULL)
 * @return JW_SUCCESS 成功
 * 
 * 示例:
 *   jw_collection_t *coll;
 *   jw_vecdb_create_collection(db, "documents", 1536, &coll);
 */
JW_API jw_status_t jw_vecdb_create_collection(jw_vecdb_t *db,
                                               const jw_str_t *name,
                                               jw_dim_t dim,
                                               jw_collection_t **coll);

/**
 * 使用详细配置创建Collection
 * 
 * @param db     数据库句柄
 * @param config Collection配置
 * @param coll   输出Collection指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_create_collection_ex(jw_vecdb_t *db,
                                                  const jw_collection_config_t *config,
                                                  jw_collection_t **coll);

/**
 * 获取Collection
 * 
 * @param db   数据库句柄
 * @param name Collection名称
 * @return Collection指针，不存在返回NULL
 */
JW_API jw_collection_t *jw_vecdb_get_collection(jw_vecdb_t *db,
                                                 const jw_str_t *name);

/**
 * 删除Collection
 * 
 * @param db   数据库句柄
 * @param name Collection名称
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_drop_collection(jw_vecdb_t *db,
                                             const jw_str_t *name);

/**
 * 检查Collection是否存在
 * 
 * @param db   数据库句柄
 * @param name Collection名称
 * @return JW_TRUE 存在
 */
JW_API jw_bool_t jw_vecdb_has_collection(const jw_vecdb_t *db,
                                          const jw_str_t *name);

/**
 * 列出所有Collection
 * 
 * @param db      数据库句柄
 * @param names   输出名称数组
 * @param capacity 数组容量
 * @return Collection数量
 */
JW_API jw_size_t jw_vecdb_list_collections(const jw_vecdb_t *db,
                                            jw_str_t *names,
                                            jw_size_t capacity);

/**
 * 重命名Collection
 * 
 * @param db      数据库句柄
 * @param old_name 旧名称
 * @param new_name 新名称
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_rename_collection(jw_vecdb_t *db,
                                               const jw_str_t *old_name,
                                               const jw_str_t *new_name);

/*
 * =============================================================================
 * 便捷操作 API (无需显式创建Collection)
 * =============================================================================
 */

/**
 * 快速插入向量
 * 
 * 自动创建或获取Collection，然后插入向量
 * 
 * @param db        数据库句柄
 * @param coll_name Collection名称
 * @param vec       向量数据
 * @param dim       向量维度
 * @param vid       输出向量ID
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_insert(jw_vecdb_t *db,
                                    const jw_str_t *coll_name,
                                    jw_cvec_t vec,
                                    jw_dim_t dim,
                                    jw_vid_t *vid);

/**
 * 快速搜索
 * 
 * @param db        数据库句柄
 * @param coll_name Collection名称
 * @param query     查询向量
 * @param dim       向量维度
 * @param k         返回结果数
 * @param results   结果数组
 * @return 实际结果数量
 */
JW_API jw_size_t jw_vecdb_search(jw_vecdb_t *db,
                                  const jw_str_t *coll_name,
                                  jw_cvec_t query,
                                  jw_dim_t dim,
                                  jw_size_t k,
                                  jw_search_result_t *results);

/**
 * 快速批量插入
 * 
 * @param db        数据库句柄
 * @param coll_name Collection名称
 * @param vectors   向量数组
 * @param dim       向量维度
 * @param count     向量数量
 * @param vids      输出ID数组 (可为NULL)
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_insert_batch(jw_vecdb_t *db,
                                          const jw_str_t *coll_name,
                                          jw_cvec_t vectors,
                                          jw_dim_t dim,
                                          jw_size_t count,
                                          jw_vid_t *vids);

/*
 * =============================================================================
 * 事务 API
 * =============================================================================
 */

/**
 * 开始事务
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_begin_transaction(jw_vecdb_t *db);

/**
 * 提交事务
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_commit(jw_vecdb_t *db);

/**
 * 回滚事务
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_rollback(jw_vecdb_t *db);

/**
 * 检查是否在事务中
 * 
 * @param db 数据库句柄
 * @return JW_TRUE 在事务中
 */
JW_API jw_bool_t jw_vecdb_in_transaction(const jw_vecdb_t *db);

/*
 * =============================================================================
 * 备份与恢复 API
 * =============================================================================
 */

/**
 * 备份数据库
 * 
 * @param db       数据库句柄
 * @param dest_path 目标路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_backup(jw_vecdb_t *db,
                                    const jw_str_t *dest_path);

/**
 * 从备份恢复
 * 
 * @param db        数据库句柄 (会被关闭并重新打开)
 * @param src_path  备份文件路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_restore(jw_vecdb_t **db,
                                     const jw_str_t *src_path);

/**
 * 导出为其他格式
 * 
 * @param db     数据库句柄
 * @param path   导出路径
 * @param format 格式 ("json", "csv", "binary")
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_export(jw_vecdb_t *db,
                                    const jw_str_t *path,
                                    const jw_str_t *format);

/**
 * 从文件导入
 * 
 * @param db     数据库句柄
 * @param path   导入文件路径
 * @param format 格式
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_import(jw_vecdb_t *db,
                                    const jw_str_t *path,
                                    const jw_str_t *format);

/*
 * =============================================================================
 * 统计与诊断 API
 * =============================================================================
 */

/**
 * 数据库统计信息
 */
typedef struct jw_vecdb_stats {
    /* 基本信息 */
    jw_size_t collection_count;     /* Collection数量 */
    jw_size_t total_vectors;        /* 总向量数 */
    jw_uint64_t database_size;      /* 数据库文件大小 */
    
    /* 内存使用 */
    jw_uint64_t memory_used;        /* 内存使用量 */
    jw_uint64_t cache_used;         /* 缓存使用量 */
    jw_uint64_t arena_used;          /* 内存池使用量 */
    
    /* 性能指标 */
    jw_uint64_t read_count;         /* 读取次数 */
    jw_uint64_t write_count;        /* 写入次数 */
    jw_uint64_t cache_hits;         /* 缓存命中 */
    jw_uint64_t cache_misses;       /* 缓存未命中 */
    jw_float32_t cache_hit_rate;    /* 缓存命中率 */
    
    /* 存储指标 */
    jw_uint64_t fragmentation;      /* 碎片率 */
    jw_uint64_t last_vacuum_time;   /* 上次清理时间 */
} jw_vecdb_stats_t;

/**
 * 获取数据库统计信息
 * 
 * @param db    数据库句柄
 * @param stats 统计信息输出
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_get_stats(const jw_vecdb_t *db,
                                       jw_vecdb_stats_t *stats);

/**
 * 重置统计计数器
 * 
 * @param db 数据库句柄
 */
JW_API void jw_vecdb_reset_stats(jw_vecdb_t *db);

/**
 * 执行数据库维护
 * 
 * 包括: 清理碎片、优化索引、回收空间
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_vecdb_vacuum(jw_vecdb_t *db);

/**
 * 校验数据库完整性
 * 
 * @param db 数据库句柄
 * @return JW_SUCCESS 校验通过
 */
JW_API jw_status_t jw_vecdb_verify(const jw_vecdb_t *db);

/**
 * 打印数据库信息 (调试用)
 * 
 * @param db 数据库句柄
 */
JW_API void jw_vecdb_print_info(const jw_vecdb_t *db);

/*
 * =============================================================================
 * 全局配置 API
 * =============================================================================
 */

/**
 * 设置全局日志回调
 * 
 * @param callback 日志回调函数
 * @param user_data 用户数据
 */
typedef void (*jw_log_callback)(int level, const char *msg, void *user_data);
JW_API void jw_vecdb_set_log_callback(jw_log_callback callback, void *user_data);

/**
 * 设置全局内存分配器
 * 
 * @param alloc   分配函数
 * @param realloc 重分配函数
 * @param free    释放函数
 * @return JW_SUCCESS 成功
 */
typedef void *(*jw_alloc_func)(size_t size);
typedef void *(*jw_realloc_func)(void *ptr, size_t size);
typedef void (*jw_free_func)(void *ptr);
JW_API jw_status_t jw_vecdb_set_allocator(jw_alloc_func alloc,
                                           jw_realloc_func realloc,
                                           jw_free_func free);

/**
 * 启用/禁用SIMD加速
 * 
 * @param enable JW_TRUE 启用
 */
JW_API void jw_vecdb_set_simd_enabled(jw_bool_t enable);

/**
 * 检查SIMD是否可用
 * 
 * @return JW_TRUE 可用
 */
JW_API jw_bool_t jw_vecdb_is_simd_available(void);

/*
 * =============================================================================
 * 错误处理 API
 * =============================================================================
 */

/**
 * 获取最后错误码
 * 
 * @param db 数据库句柄
 * @return 错误码
 */
JW_API jw_status_t jw_vecdb_get_last_error(const jw_vecdb_t *db);

/**
 * 获取错误信息
 * 
 * @param status 错误码
 * @return 错误信息字符串
 */
JW_API const char *jw_vecdb_strerror(jw_status_t status);

/**
 * 获取详细错误信息
 * 
 * @param db 数据库句柄
 * @return 详细错误信息
 */
JW_API const char *jw_vecdb_get_error_message(const jw_vecdb_t *db);

/*
 * =============================================================================
 * 默认配置宏
 * =============================================================================
 */

#define JW_VECDB_CONFIG_DEFAULT { \
    .db_path = {0}, \
    .storage_mode = JW_STORAGE_HYBRID, \
    .arena_size = 64 * 1024 * 1024, \
    .cache_size = 128 * 1024 * 1024, \
    .read_only = JW_FALSE, \
    .create_if_missing = JW_TRUE \
}

/*
 * =============================================================================
 * 便捷宏定义
 * =============================================================================
 */

/* 安全关闭数据库 */
#define JW_SAFE_CLOSE(db) \
    do { \
        if ((db) != NULL) { \
            jw_vecdb_close(db); \
            (db) = NULL; \
        } \
    } while(0)

/* 内存数据库快速创建 */
#define JW_VECDB_MEMORY() \
    jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, &db)

/* 文件数据库快速创建 */
#define JW_VECDB_FILE(path) \
    jw_vecdb_open(path, JW_VECDB_CREATE | JW_VECDB_READWRITE, &db)

JW_END_DECL

#endif /* JW_VECDB_H */
