/*
 * jw_vecdb.c - JinWo VecDB 主接口实现
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_vecdb.h"
#include "jw_string.h"
#include "jw_stdio.h"
#include "jw_file.h"
#include <stdio.h>

/*
 * =============================================================================
 * 内部结构定义
 * =============================================================================
 */

/* 数据库结构 */
struct jw_vecdb_t {
    /* 配置 */
    jw_vecdb_config_t config;
    
    /* 内存池 */
    jw_arena_t *arena;
    
    /* Collection管理 */
    jw_collection_t **collections;
    jw_size_t collection_count;
    jw_size_t collection_capacity;
    
    /* 存储 */
    jw_storage_t *storage;
    
    /* 统计 */
    jw_vecdb_stats_t stats;
    
    /* 状态 */
    jw_bool_t is_open;
    jw_uint64_t open_time;
    
    /* 错误信息 */
    jw_status_t last_error;
    char error_msg[256];
    
    /* 锁 */
    jw_rwlock_t *lock;
};

/* 全局日志回调 */
static jw_log_callback g_log_callback = NULL;
static void *g_log_user_data = NULL;

/*
 * =============================================================================
 * 版本与构建信息
 * =============================================================================
 */

JW_API jw_str_t jw_vecdb_version(void)
{
    static jw_str_t version_str = JW_STR_NULL;
    static const char *version_buf = "JinWo VecDB " JW_VERSION_STRING;
    if (version_str.ptr == NULL) {
        version_str.ptr = (char*)version_buf;
        version_str.slen = sizeof("JinWo VecDB " JW_VERSION_STRING) - 1;
    }
    return version_str;
}

JW_API jw_str_t jw_vecdb_build_info(void)
{
    static jw_str_t build_info_str = JW_STR_NULL;
    static char build_info_buf[512];
    if (build_info_str.ptr == NULL) {
#if defined(_WIN32) || defined(_WIN64)
        const char *platform = "Windows";
#elif defined(JW_LINUX)
        const char *platform = "Linux";
#elif defined(JW_ANDROID)
        const char *platform = "Android";
#elif defined(JW_IOS)
        const char *platform = "iOS";
#elif defined(JW_MACOS)
        const char *platform = "macOS";
#elif defined(JW_EMSCRIPTEN)
        const char *platform = "WebAssembly";
#else
        const char *platform = "Unknown";
#endif
#if defined(__GNUC__)
        const char *compiler = "GCC";
#elif defined(__clang__)
        const char *compiler = "Clang";
#elif defined(_MSC_VER)
        const char *compiler = "MSVC";
#else
        const char *compiler = "Unknown";
#endif
        int len = snprintf(build_info_buf, sizeof(build_info_buf),
            "JinWo VecDB " JW_VERSION_STRING "\n"
            "Platform: %s\n"
            "Compiler: %s\n"
            "Build Date: " __DATE__ " " __TIME__,
            platform, compiler);
        build_info_str.ptr = build_info_buf;
        build_info_str.slen = (jw_size_t)len;
    }
    return build_info_str;
}

/*
 * =============================================================================
 * 数据库生命周期
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_open(const jw_str_t *path,
                                  jw_uint32_t flags,
                                  jw_vecdb_t **db)
{
    jw_vecdb_config_t config = JW_VECDB_CONFIG_DEFAULT;
    if (path && path->ptr) {
        config.db_path = *path;
    }
    config.create_if_missing = (flags & JW_VECDB_CREATE) != 0;
    config.read_only = (flags & JW_VECDB_READONLY) != 0;
    
    return jw_vecdb_open_ex(&config, db);
}

JW_API jw_status_t jw_vecdb_open_ex(const jw_vecdb_config_t *config,
                                     jw_vecdb_t **db)
{
    if (db == NULL) {
        return JW_INVALID_PARAM;
    }
    
    *db = NULL;
    
    /* 分配数据库结构 */
    jw_vecdb_t *database = (jw_vecdb_t *)jw_calloc(1, sizeof(jw_vecdb_t));
    if (database == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    /* 复制配置 */
    if (config != NULL) {
        database->config = *config;
    } else {
        jw_vecdb_config_t default_config = JW_VECDB_CONFIG_DEFAULT;
        database->config = default_config;
    }
    
    /* 创建内存池 */
    jw_size_t arena_size = database->config.arena_size;
    if (arena_size == 0) {
        arena_size = 64 * 1024 * 1024;  /* 默认64MB */
    }

    jw_arena_t *arena = NULL;
    jw_status_t arena_status = jw_arena_create(arena_size, &arena);
    if (arena_status != JW_SUCCESS || arena == NULL) {
        jw_free(database);
        return JW_OUT_OF_MEMORY;
    }
    database->arena = arena;

    /* 初始化读写锁 */
    jw_rwlock_t *rwlock = NULL;
    jw_status_t status = jw_rwlock_create(NULL, NULL, &rwlock);
    if (status != JW_SUCCESS || rwlock == NULL) {
        jw_arena_destroy(database->arena);
        jw_free(database);
        return status;
    }
    database->lock = rwlock;
    
    /* 初始化Collection数组 */
    database->collection_capacity = 16;
    database->collections = jw_arena_calloc(database->arena,
                                            database->collection_capacity,
                                            sizeof(jw_collection_t*));
    
    if (database->collections == NULL) {
        jw_rwlock_destroy(database->lock);
        jw_arena_destroy(database->arena);
        jw_free(database);
        return JW_OUT_OF_MEMORY;
    }
    
    /* 如果是文件数据库，打开存储 */
    if (database->config.db_path.ptr != NULL &&
        database->config.storage_mode != JW_STORAGE_MEMORY) {

        jw_storage_config_t storage_config = JW_STORAGE_CONFIG_DEFAULT;
        storage_config.path = database->config.db_path;
        storage_config.mode = database->config.read_only
                            ? JW_STORAGE_READ
                            : JW_STORAGE_READWRITE;

        if (database->config.create_if_missing) {
            storage_config.mode = JW_STORAGE_CREATE;
        }
        
        database->storage = jw_storage_create(database->arena, &storage_config);
        if (database->storage == NULL) {
            /* 内存数据库可以没有存储 */
        }
    }
    
    /* 如果是文件数据库，从磁盘加载已有collections */
    if (database->config.db_path.ptr != NULL &&
        database->config.db_path.slen > 0 &&
        database->config.storage_mode != JW_STORAGE_MEMORY &&
        !database->config.create_if_missing) {
        
        char meta_path[1024];
        jw_snprintf(meta_path, sizeof(meta_path), "%s/.jwmeta",
                   database->config.db_path.ptr);
        
        jw_str_t meta_path_str = {meta_path, strlen(meta_path)};
        jw_size_t meta_size = 0;
        char *meta_content = jw_file_read_all(&meta_path_str, &meta_size);
        
        if (meta_content != NULL && meta_size > 0) {
            /* 逐行解析collection名称 */
            char *line_start = meta_content;
            for (jw_size_t pos = 0; pos < meta_size; pos++) {
                if (meta_content[pos] == '\n' || meta_content[pos] == '\0') {
                    meta_content[pos] = '\0';
                    if (line_start[0] != '\0') {
                        /* 构建collection文件路径并加载 */
                        char coll_path[1024];
                        jw_snprintf(coll_path, sizeof(coll_path), "%s/%s.jwcol",
                                   database->config.db_path.ptr, line_start);
                        
                        jw_collection_t *coll = jw_collection_load(database->arena, coll_path);
                        if (coll != NULL) {
                            /* 添加到collections数组 */
                            if (database->collection_count >= database->collection_capacity) {
                                jw_size_t new_cap = database->collection_capacity * 2;
                                jw_collection_t **new_arr = jw_arena_calloc(
                                    database->arena, new_cap, sizeof(jw_collection_t*));
                                if (new_arr != NULL) {
                                    jw_memcpy(new_arr, database->collections,
                                           database->collection_count * sizeof(jw_collection_t*));
                                    database->collections = new_arr;
                                    database->collection_capacity = new_cap;
                                }
                            }
                            if (database->collection_count < database->collection_capacity) {
                                database->collections[database->collection_count++] = coll;
                            }
                        }
                    }
                    line_start = meta_content + pos + 1;
                }
            }
            jw_free(meta_content);
        }
    }
    
    /* 设置状态 */
    database->is_open = JW_TRUE;
    database->open_time = jw_time_now();
    
    jw_memset(&database->stats, 0, sizeof(database->stats));
    
    *db = database;
    return JW_SUCCESS;
}

/* 内部: 保存所有collection到磁盘 */
static jw_status_t jw_vecdb_save_all(jw_vecdb_t *db)
{
    if (db == NULL || !db->is_open) return JW_INVALID_PARAM;
    if (db->config.db_path.ptr == NULL || db->config.db_path.slen == 0) {
        return JW_SUCCESS;  /* 内存数据库，不需要保存 */
    }
    
    char coll_path[1024];
    char meta_path[1024];
    char meta_content[4096];
    jw_size_t meta_len = 0;
    
    /* 构建元数据文件路径 */
    jw_snprintf(meta_path, sizeof(meta_path), "%s/.jwmeta",
               db->config.db_path.ptr);
    
    meta_content[0] = '\0';
    meta_len = 0;
    
    /* 保存每个collection */
    for (jw_size_t i = 0; i < db->collection_count; i++) {
        jw_collection_t *coll = db->collections[i];
        if (coll == NULL || coll->name == NULL) continue;
        
        /* 构建collection文件路径: {db_path}/{name}.jwcol */
        jw_snprintf(coll_path, sizeof(coll_path), "%s/%s.jwcol",
                   db->config.db_path.ptr, coll->name);
        
        jw_collection_save(coll, coll_path);
        
        /* 追加到元数据 */
        meta_len += jw_snprintf(meta_content + meta_len,
                               sizeof(meta_content) - meta_len,
                               "%s\n", coll->name);
    }
    
    /* 写入元数据文件 */
    if (meta_len > 0) {
        jw_str_t mp = {meta_path, strlen(meta_path)};
        jw_file_write_all(&mp, meta_content, meta_len);
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_vecdb_close(jw_vecdb_t *db)
{
    if (db == NULL || !db->is_open) {
        return JW_INVALID_PARAM;
    }
    
    /* 关闭前保存所有collection */
    jw_vecdb_save_all(db);
    
    /* 获取写锁 */
    jw_rwlock_wrlock(db->lock);
    
    /* 关闭所有Collection */
    for (jw_size_t i = 0; i < db->collection_count; i++) {
        if (db->collections[i] != NULL) {
            jw_collection_destroy(db->collections[i]);
            db->collections[i] = NULL;
        }
    }
    
    /* 关闭存储 */
    if (db->storage != NULL) {
        jw_storage_close(db->storage);
        db->storage = NULL;
    }
    
    db->is_open = JW_FALSE;
    
    jw_rwlock_wrunlock(db->lock);
    jw_rwlock_destroy(db->lock);
    
    /* 销毁内存池 */
    if (db->arena != NULL) {
        jw_arena_destroy(db->arena);
    }
    
    jw_free(db);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_vecdb_sync(jw_vecdb_t *db)
{
    if (db == NULL || !db->is_open) {
        return JW_INVALID_PARAM;
    }
    
    /* 保存所有collection到磁盘 */
    jw_vecdb_save_all(db);
    
    if (db->storage != NULL) {
        return jw_storage_sync(db->storage);
    }
    
    return JW_SUCCESS;
}

JW_API jw_bool_t jw_vecdb_is_open(const jw_vecdb_t *db)
{
    return (db != NULL && db->is_open);
}

/*
 * =============================================================================
 * Collection 管理
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_create_collection(jw_vecdb_t *db,
                                               const jw_str_t *name,
                                               jw_dim_t dim,
                                               jw_collection_t **coll)
{
    jw_collection_config_t config = JW_COLLECTION_CONFIG_DEFAULT;
    config.name = *name;
    config.dimension = dim;
    
    return jw_vecdb_create_collection_ex(db, &config, coll);
}

JW_API jw_status_t jw_vecdb_create_collection_ex(jw_vecdb_t *db,
                                                  const jw_collection_config_t *config,
                                                  jw_collection_t **coll)
{
    if (db == NULL || !db->is_open || config == NULL || config->name.ptr == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(db->lock);
    
    /* 检查是否已存在 — 在锁内直接遍历，不调用 get_collection（避免重入锁） */
    jw_bool_t exists = JW_FALSE;
    for (jw_size_t i = 0; i < db->collection_count; i++) {
        const char *coll_name = jw_collection_get_name(db->collections[i]);
        jw_str_t cn_str;
        JW_STR_SET(cn_str, coll_name);
        if (jw_strcmp(&cn_str, &config->name) == 0) {
            exists = JW_TRUE;
            break;
        }
    }
    if (exists) {
        jw_rwlock_wrunlock(db->lock);
        return JW_ALREADY_EXISTS;
    }
    
    /* 扩展数组 */
    if (db->collection_count >= db->collection_capacity) {
        jw_size_t new_capacity = db->collection_capacity * 2;
        jw_collection_t **new_collections = jw_arena_calloc(db->arena,
                                                            new_capacity,
                                                            sizeof(jw_collection_t*));
        if (new_collections == NULL) {
            jw_rwlock_wrunlock(db->lock);
            return JW_OUT_OF_MEMORY;
        }
        
        jw_memcpy(new_collections, db->collections,
               db->collection_count * sizeof(jw_collection_t*));
        db->collections = new_collections;
        db->collection_capacity = new_capacity;
    }
    
    /* 创建Collection */
    jw_collection_t *collection = jw_collection_create(db->arena, config);
    if (collection == NULL) {
        jw_rwlock_wrunlock(db->lock);
        return JW_UNKNOWN_ERROR;
    }
    
    db->collections[db->collection_count++] = collection;
    db->stats.collection_count = db->collection_count;
    
    jw_rwlock_wrunlock(db->lock);
    
    if (coll != NULL) {
        *coll = collection;
    }
    
    return JW_SUCCESS;
}

JW_API jw_collection_t *jw_vecdb_get_collection(jw_vecdb_t *db,
                                                 const jw_str_t *name)
{
    if (db == NULL || !db->is_open || name == NULL || name->ptr == NULL) {
        return NULL;
    }
    
    jw_rwlock_rdlock(db->lock);
    
    jw_collection_t *found = NULL;
    for (jw_size_t i = 0; i < db->collection_count; i++) {
        const char *coll_name = jw_collection_get_name(db->collections[i]);
        jw_str_t coll_name_str;
        JW_STR_SET(coll_name_str, coll_name);
        if (jw_strcmp(&coll_name_str, name) == 0) {
            found = db->collections[i];
            break;
        }
    }
    
    jw_rwlock_rdunlock(db->lock);
    
    return found;
}

JW_API jw_status_t jw_vecdb_drop_collection(jw_vecdb_t *db,
                                             const jw_str_t *name)
{
    if (db == NULL || !db->is_open || name == NULL || name->ptr == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(db->lock);
    
    jw_status_t status = JW_NOT_FOUND;
    
    for (jw_size_t i = 0; i < db->collection_count; i++) {
        const char *coll_name = jw_collection_get_name(db->collections[i]);
        jw_str_t coll_name_str;
        JW_STR_SET(coll_name_str, coll_name);
        if (jw_strcmp(&coll_name_str, name) == 0) {
            jw_collection_destroy(db->collections[i]);
            
            /* 移动后面的元素 */
            for (jw_size_t j = i; j < db->collection_count - 1; j++) {
                db->collections[j] = db->collections[j + 1];
            }
            db->collections[db->collection_count - 1] = NULL;
            db->collection_count--;
            db->stats.collection_count = db->collection_count;
            
            status = JW_SUCCESS;
            break;
        }
    }
    
    jw_rwlock_wrunlock(db->lock);
    
    return status;
}

JW_API jw_bool_t jw_vecdb_has_collection(const jw_vecdb_t *db,
                                          const jw_str_t *name)
{
    return jw_vecdb_get_collection((jw_vecdb_t *)db, name) != NULL;
}

JW_API jw_size_t jw_vecdb_list_collections(const jw_vecdb_t *db,
                                            jw_str_t *names,
                                            jw_size_t capacity)
{
    if (db == NULL || !db->is_open) {
        return 0;
    }
    
    jw_rwlock_rdlock(db->lock);
    
    jw_size_t count = db->collection_count;
    if (names != NULL && capacity > 0) {
        count = (count < capacity) ? count : capacity;
        for (jw_size_t i = 0; i < count; i++) {
            const char *coll_name = jw_collection_get_name(db->collections[i]);
            jw_str_t tmp;
            JW_STR_SET(tmp, coll_name);
            names[i] = tmp;
        }
    }
    
    jw_rwlock_rdunlock(db->lock);
    
    return count;
}

/*
 * =============================================================================
 * 便捷操作 API
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_insert(jw_vecdb_t *db,
                                    const jw_str_t *coll_name,
                                    jw_cvec_t vec,
                                    jw_dim_t dim,
                                    jw_vid_t *vid)
{
    jw_collection_t *coll = jw_vecdb_get_collection(db, coll_name);
    if (coll == NULL) {
        /* 自动创建 */
        jw_status_t status = jw_vecdb_create_collection(db, coll_name, dim, &coll);
        if (status != JW_SUCCESS) {
            return status;
        }
    }
    
    return jw_collection_insert(coll, vec, vid);
}

JW_API jw_size_t jw_vecdb_search(jw_vecdb_t *db,
                                  const jw_str_t *coll_name,
                                  jw_cvec_t query,
                                  jw_dim_t dim,
                                  jw_size_t k,
                                  jw_search_result_t *results)
{
    jw_collection_t *coll = jw_vecdb_get_collection(db, coll_name);
    if (coll == NULL) {
        return 0;
    }
    
    jw_search_options_t options = {
        .k = k,
        .filter = NULL,
        .include_vectors = JW_FALSE,
        .include_meta = JW_FALSE
    };
    
    jw_search_result_ex_t *ex_results = jw_arena_alloc(db->arena, 
                                                       k * sizeof(jw_search_result_ex_t));
    if (ex_results == NULL) {
        return 0;
    }
    
    jw_size_t count = jw_collection_search(coll, query, &options, ex_results);
    
    /* 转换为简单结果 */
    for (jw_size_t i = 0; i < count && i < k; i++) {
        results[i].id = ex_results[i].vid;
        results[i].score = ex_results[i].score;
    }
    
    return count;
}

/*
 * =============================================================================
 * 统计与诊断
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_get_stats(const jw_vecdb_t *db,
                                       jw_vecdb_stats_t *stats)
{
    if (db == NULL || stats == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_rdlock(db->lock);
    *stats = db->stats;
    jw_rwlock_rdunlock(db->lock);
    
    return JW_SUCCESS;
}

JW_API void jw_vecdb_reset_stats(jw_vecdb_t *db)
{
    if (db != NULL) {
        jw_memset(&db->stats, 0, sizeof(db->stats));
        db->stats.collection_count = db->collection_count;
    }
}

JW_API void jw_vecdb_print_info(const jw_vecdb_t *db)
{
    if (db == NULL) {
        jw_printf("Database: NULL\n");
        return;
    }
    
    jw_printf("JinWo VecDB Information\n");
    jw_printf("=======================\n");
    jw_printf("Version:     %s\n", JW_VERSION_STRING);
    jw_printf("Path:        %s\n", db->config.db_path.ptr ? db->config.db_path.ptr : "(memory)");
    jw_printf("Status:      %s\n", db->is_open ? "Open" : "Closed");
    jw_printf("Collections: %zu\n", db->collection_count);
    jw_printf("Memory:      %zu bytes\n", jw_arena_get_used_size(db->arena));
    
    if (db->collection_count > 0) {
        jw_printf("\nCollections:\n");
        for (jw_size_t i = 0; i < db->collection_count; i++) {
            jw_collection_stats_t coll_stats;
            jw_collection_get_stats(db->collections[i], &coll_stats);
            jw_printf("  - %s: %zu vectors, %d dims\n",
                   jw_collection_get_name(db->collections[i]),
                   coll_stats.count,
                   coll_stats.dim);
        }
    }
}

/*
 * =============================================================================
 * 错误处理
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_get_last_error(const jw_vecdb_t *db)
{
    return (db != NULL) ? db->last_error : JW_UNKNOWN_ERROR;
}

JW_API const char *jw_vecdb_get_error_message(const jw_vecdb_t *db)
{
    if (db != NULL && db->error_msg[0] != '\0') {
        return db->error_msg;
    }
    return jw_strerror(db->last_error);
}

JW_API const char *jw_vecdb_strerror(jw_status_t status)
{
    return jw_strerror(status);
}

/*
 * =============================================================================
 * 批量插入
 * =============================================================================
 */

JW_API jw_status_t jw_vecdb_insert_batch(jw_vecdb_t *db,
                                          const jw_str_t *coll_name,
                                          jw_cvec_t vectors,
                                          jw_dim_t dim,
                                          jw_size_t count,
                                          jw_vid_t *vids)
{
    jw_collection_t *coll;
    jw_status_t status;
    jw_size_t i;

    if (db == NULL || coll_name == NULL || vectors == NULL) {
        return JW_INVALID_PARAM;
    }

    coll = jw_vecdb_get_collection(db, coll_name);
    if (coll == NULL) {
        /* 自动创建 */
        status = jw_vecdb_create_collection(db, coll_name, dim, &coll);
        if (status != JW_SUCCESS) {
            return status;
        }
    }

    for (i = 0; i < count; i++) {
        jw_vid_t vid;
        status = jw_collection_insert(coll,
            (jw_cvec_t)((const jw_float32_t*)vectors + i * (jw_size_t)dim),
            vids ? &vids[i] : &vid);
        if (status != JW_SUCCESS) {
            return status;
        }
    }

    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 全局配置
 * =============================================================================
 */

JW_API void jw_vecdb_set_log_callback(jw_log_callback callback, void *user_data)
{
    g_log_callback = callback;
    g_log_user_data = user_data;
}

JW_API jw_status_t jw_vecdb_set_allocator(jw_alloc_func alloc,
                                           jw_realloc_func realloc,
                                           jw_free_func free)
{
    jw_allocator_t allocator = {
        .alloc = (void* (*)(jw_size_t, void*))alloc,
        .realloc = (void* (*)(void*, jw_size_t, void*))realloc,
        .free = (void (*)(void*, void*))free,
        .user_data = NULL
    };
    
    return jw_set_allocator(&allocator);
}

JW_API void jw_vecdb_set_simd_enabled(jw_bool_t enable)
{
    jw_set_simd_enabled(enable);
}

JW_API jw_bool_t jw_vecdb_is_simd_available(void)
{
    return jw_is_simd_available();
}
