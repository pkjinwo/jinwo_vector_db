/*
 * jw_config.c - JinWo VecDB 配置管理实现
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

#include "jw_config.h"
#include "jw_arena.h"
#include "jw_string.h"
#include "jw_file.h"
#include "jw_log.h"

/* 内部配置节点结构 */
typedef struct jw_config_node {
    char *key;
    jw_config_type_t type;
    union {
        jw_int64_t int_val;
        jw_float64_t float_val;
        char *string_val;
        jw_bool_t bool_val;
        struct jw_config_node *object_val;
        struct jw_config_array *array_val;
    } value;
    struct jw_config_node *next;
    struct jw_config_node *children;
} jw_config_node_t;

/* 内部配置数组结构 */
typedef struct jw_config_array {
    jw_config_node_t **items;
    jw_size_t count;
    jw_size_t capacity;
} jw_config_array_t;

/* 内部配置结构 */
typedef struct jw_config_impl {
    jw_arena_t *arena;
    jw_config_node_t *root;
    char *config_file;
} jw_config_impl_t;

/*
 * =============================================================================
 * 内部函数
 * =============================================================================
 */

static jw_config_node_t *config_node_create(jw_arena_t *arena, const char *key)
{
    jw_config_node_t *node = jw_arena_calloc(arena, 1, sizeof(jw_config_node_t));
    if (node) {
        node->key = jw_arena_strdup(arena, key);
        node->type = JW_CONFIG_TYPE_INT;
        node->value.int_val = 0;
        node->next = NULL;
        node->children = NULL;
    }
    return node;
}

static jw_config_node_t *config_find_node(jw_config_node_t *parent, const char *key)
{
    if (!parent) return NULL;
    
    jw_config_node_t *node = parent->children;
    while (node) {
        jw_str_t s1 = { node->key, 0 };
        jw_str_t s2 = { (char *)key, 0 };
        s1.slen = jw_strlen(&s1);
        s2.slen = jw_strlen(&s2);
        if (jw_strcmp(&s1, &s2) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static jw_config_node_t *config_get_or_create_node(jw_config_impl_t *impl, const char *key)
{
    jw_str_t key_str;
    key_str.ptr = (char *)key;
    key_str.slen = jw_strlen(&key_str);
    
    jw_str_t delim;
    delim.ptr = ".";
    delim.slen = 1;
    
    jw_str_t **tokens = jw_strsplit(impl->arena, &key_str, &delim);
    if (!tokens) {
        return NULL;
    }
    
    jw_config_node_t *current = impl->root;
    jw_str_t **token_ptr = tokens;
    
    while (*token_ptr) {
        jw_config_node_t *node = config_find_node(current, (*token_ptr)->ptr);
        if (!node) {
            node = config_node_create(impl->arena, (*token_ptr)->ptr);
            if (!node) {
                return NULL;
            }
            
            /* 添加到链表 */
            node->next = current->children;
            current->children = node;
        }
        current = node;
        token_ptr++;
    }
    
    return current;
}

static jw_config_node_t *config_get_node(jw_config_impl_t *impl, const char *key)
{
    jw_str_t key_str;
    key_str.ptr = (char *)key;
    key_str.slen = jw_strlen(&key_str);
    
    jw_str_t delim;
    delim.ptr = ".";
    delim.slen = 1;
    
    jw_str_t **tokens = jw_strsplit(impl->arena, &key_str, &delim);
    if (!tokens) {
        return NULL;
    }
    
    jw_config_node_t *current = impl->root;
    jw_str_t **token_ptr = tokens;
    
    while (*token_ptr) {
        current = config_find_node(current, (*token_ptr)->ptr);
        if (!current) {
            return NULL;
        }
        token_ptr++;
    }
    
    return current;
}

static jw_status_t config_parse_json(jw_config_impl_t *impl, const char *json_str)
{
    /* 简单的JSON解析器实现 */
    /* 注意：这是一个简化版本，只支持基本的JSON格式 */
    
    /* TODO: 实现完整的JSON解析 */
    
    /* 暂时返回成功，实际实现需要解析JSON */
    return JW_SUCCESS;
}

static jw_status_t config_write_json(jw_os_handle_t handle, jw_config_node_t *node, int indent)
{
    (void)handle;
    (void)node;
    (void)indent;
    /* 简单的JSON生成器实现 */
    /* 注意：这是一个简化版本，只支持基本的JSON格式 */
    
    /* TODO: 实现完整的JSON生成 */
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 配置API实现
 * =============================================================================
 */

JW_API jw_config_t *jw_config_create(jw_arena_t *arena)
{
    if (!arena) {
        return NULL;
    }
    
    jw_config_t *config = jw_arena_calloc(arena, 1, sizeof(jw_config_t));
    if (!config) {
        return NULL;
    }
    
    jw_config_impl_t *impl = jw_arena_calloc(arena, 1, sizeof(jw_config_impl_t));
    if (!impl) {
        return NULL;
    }
    
    impl->arena = arena;
    impl->root = config_node_create(arena, "root");
    if (!impl->root) {
        return NULL;
    }
    
    config->arena = arena;
    config->data = impl;
    
    return config;
}

JW_API void jw_config_destroy(jw_config_t *config)
{
    (void)config;
}

JW_API jw_status_t jw_config_load(jw_config_t *config, jw_str_t *filename)
{
    if (!config || !filename) {
        return JW_INVALID_PARAM;
    }

    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;

    /* 读取文件内容 */
    jw_size_t size = 0;
    char *content = jw_file_read_all(filename, &size);
    if (!content) {
        return JW_IO_ERROR;
    }

    /* 解析JSON */
    jw_status_t status = config_parse_json(impl, content);

    /* 保存配置文件路径 */
    impl->config_file = jw_arena_strdup(impl->arena, filename->ptr);

    /* 释放文件内容 */
    jw_free(content);

    return status;
}

JW_API jw_status_t jw_config_load_string(jw_config_t *config, jw_str_t *json_str)
{
    if (!config || !json_str) {
        return JW_INVALID_PARAM;
    }

    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    return config_parse_json(impl, json_str->ptr);
}

JW_API jw_status_t jw_config_save(jw_config_t *config, jw_str_t *filename)
{
    if (!config || !filename) {
        return JW_INVALID_PARAM;
    }

    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;

    /* 打开文件 */
    jw_str_t mode_str = jw_str("w");
    jw_os_handle_t handle = jw_file_open(filename, &mode_str);
    if (handle == JW_INVALID_OS_HANDLE) {
        return JW_IO_ERROR;
    }

    /* 写入JSON */
    jw_status_t status = config_write_json(handle, impl->root, 0);

    /* 关闭文件 */
    jw_file_close(handle);

    return status;
}

JW_API jw_int64_t jw_config_get_int(jw_config_t *config, const jw_str_t *key, jw_int64_t default_val)
{
    if (!config || !key) {
        return default_val;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    jw_config_node_t *node = config_get_node(impl, key->ptr);
    
    if (node && node->type == JW_CONFIG_TYPE_INT) {
        return node->value.int_val;
    }
    
    return default_val;
}

JW_API jw_float64_t jw_config_get_float(jw_config_t *config, const jw_str_t *key, jw_float64_t default_val)
{
    if (!config || !key) {
        return default_val;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    jw_config_node_t *node = config_get_node(impl, key->ptr);
    
    if (node && node->type == JW_CONFIG_TYPE_FLOAT) {
        return node->value.float_val;
    }
    
    return default_val;
}

JW_API const jw_str_t *jw_config_get_string(jw_config_t *config, const jw_str_t *key, const jw_str_t *default_val)
{
    if (!config || !key) {
        return default_val;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    jw_config_node_t *node = config_get_node(impl, key->ptr);
    
    if (node && node->type == JW_CONFIG_TYPE_STRING) {
        static jw_str_t result;
        result.ptr = node->value.string_val;
        jw_str_t temp_str = { node->value.string_val, 0 };
        result.slen = jw_strlen(&temp_str);
        return &result;
    }
    
    return default_val;
}

JW_API jw_bool_t jw_config_get_bool(jw_config_t *config, const jw_str_t *key, jw_bool_t default_val)
{
    if (!config || !key) {
        return default_val;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    jw_config_node_t *node = config_get_node(impl, key->ptr);
    
    if (node && node->type == JW_CONFIG_TYPE_BOOL) {
        return node->value.bool_val;
    }
    
    return default_val;
}

JW_API jw_status_t jw_config_set(jw_config_t *config, const jw_str_t *key, const void *value, jw_config_type_t type)
{
    if (!config || !key || !value) {
        return JW_INVALID_PARAM;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    jw_config_node_t *node = config_get_or_create_node(impl, key->ptr);
    if (!node) {
        return JW_OUT_OF_MEMORY;
    }
    
    node->type = type;
    
    switch (type) {
        case JW_CONFIG_TYPE_INT:
            node->value.int_val = *(const jw_int64_t *)value;
            break;
        case JW_CONFIG_TYPE_FLOAT:
            node->value.float_val = *(const jw_float64_t *)value;
            break;
        case JW_CONFIG_TYPE_STRING:
            node->value.string_val = jw_arena_strdup(impl->arena, (const char *)value);
            break;
        case JW_CONFIG_TYPE_BOOL:
            node->value.bool_val = *(const jw_bool_t *)value;
            break;
        default:
            return JW_INVALID_PARAM;
    }
    
    return JW_SUCCESS;
}

JW_API jw_bool_t jw_config_has_key(jw_config_t *config, const jw_str_t *key)
{
    if (!config || !key) {
        return JW_FALSE;
    }
    
    jw_config_impl_t *impl = (jw_config_impl_t *)config->data;
    return (config_get_node(impl, key->ptr) != NULL) ? JW_TRUE : JW_FALSE;
}

JW_API jw_status_t jw_config_to_db_config(jw_config_t *config, jw_vecdb_config_t *db_config)
{
    if (!config || !db_config) {
        return JW_INVALID_PARAM;
    }
    
    /* 设置默认值 */
    *db_config = (jw_vecdb_config_t){
        .db_path = "./data",
        .storage_mode = JW_STORAGE_DISK,
        .arena_size = 104857600,  /* 100MB */
        .cache_size = 52428800,   /* 50MB */
        .read_only = JW_FALSE,
        .create_if_missing = JW_TRUE
    };
    
    /* 从配置加载 */
    jw_str_t key_path;
    key_path.ptr = "database.path";
    jw_str_t temp_path = { "database.path", 0 };
    key_path.slen = jw_strlen(&temp_path);
    
    jw_str_t default_path;
    default_path.ptr = NULL;
    default_path.slen = 0;
    
    const jw_str_t *path = jw_config_get_string(config, &key_path, &default_path);
    if (path && path->ptr) {
        db_config->db_path = *path;
    }
    
    jw_str_t key_mode;
    key_mode.ptr = "database.storage_mode";
    jw_str_t temp_mode = { "database.storage_mode", 0 };
    key_mode.slen = jw_strlen(&temp_mode);
    
    jw_str_t default_mode;
    default_mode.ptr = NULL;
    default_mode.slen = 0;
    
    const jw_str_t *mode = jw_config_get_string(config, &key_mode, &default_mode);
    if (mode && mode->ptr) {
        jw_str_t s1 = { mode->ptr, mode->slen };
        jw_str_t s2 = { "memory", 0 };
        s2.slen = jw_strlen(&s2);
        if (jw_strcmp(&s1, &s2) == 0) {
            db_config->storage_mode = JW_STORAGE_MEMORY;
        } else {
            jw_str_t s3 = { "persistent", 0 };
            s3.slen = jw_strlen(&s3);
            if (jw_strcmp(&s1, &s3) == 0) {
                db_config->storage_mode = JW_STORAGE_DISK;
            }
        }
    }
    
    jw_str_t key_arena_size;
    key_arena_size.ptr = "database.arena_size";
    jw_str_t temp_arena_size = { "database.arena_size", 0 };
    key_arena_size.slen = jw_strlen(&temp_arena_size);
    
    jw_int64_t arena_size = jw_config_get_int(config, &key_arena_size, 0);
    if (arena_size > 0) {
        db_config->arena_size = (jw_size_t)arena_size;
    }
    
    jw_str_t key_cache_size;
    key_cache_size.ptr = "database.cache_size";
    jw_str_t temp_cache_size = { "database.cache_size", 0 };
    key_cache_size.slen = jw_strlen(&temp_cache_size);
    
    jw_int64_t cache_size = jw_config_get_int(config, &key_cache_size, 0);
    if (cache_size > 0) {
        db_config->cache_size = (jw_size_t)cache_size;
    }
    
    jw_str_t key_read_only;
    key_read_only.ptr = "database.read_only";
    jw_str_t temp_read_only = { "database.read_only", 0 };
    key_read_only.slen = jw_strlen(&temp_read_only);
    
    db_config->read_only = jw_config_get_bool(config, &key_read_only, JW_FALSE);
    
    jw_str_t key_create_if_missing;
    key_create_if_missing.ptr = "database.create_if_missing";
    jw_str_t temp_create_if_missing = { "database.create_if_missing", 0 };
    key_create_if_missing.slen = jw_strlen(&temp_create_if_missing);
    
    db_config->create_if_missing = jw_config_get_bool(config, &key_create_if_missing, JW_TRUE);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_to_collection_config(jw_config_t *config, jw_collection_config_t *coll_config)
{
    if (!config || !coll_config) {
        return JW_INVALID_PARAM;
    }
    
    /* 设置默认值 */
    *coll_config = (jw_collection_config_t){
        .dimension = 128,
        .metric = JW_METRIC_L2,
        .index_type = JW_INDEX_HNSW,
        .auto_resize = JW_TRUE
    };
    
    /* 从配置加载 */
    jw_str_t key_name;
    key_name.ptr = "collection.name";
    jw_str_t temp_name = { "collection.name", 0 };
    key_name.slen = jw_strlen(&temp_name);
    
    jw_str_t default_name;
    default_name.ptr = NULL;
    default_name.slen = 0;
    
    const jw_str_t *name = jw_config_get_string(config, &key_name, &default_name);
    if (name && name->ptr) {
        coll_config->name = *name;
    }
    
    jw_str_t key_dimension;
    key_dimension.ptr = "collection.dimension";
    jw_str_t temp_dimension = { "collection.dimension", 0 };
    key_dimension.slen = jw_strlen(&temp_dimension);
    
    jw_int64_t dim = jw_config_get_int(config, &key_dimension, 0);
    if (dim > 0) {
        coll_config->dimension = (jw_dim_t)dim;
    }
    
    jw_str_t key_metric;
    key_metric.ptr = "collection.metric";
    jw_str_t temp_metric = { "collection.metric", 0 };
    key_metric.slen = jw_strlen(&temp_metric);
    
    jw_str_t default_metric;
    default_metric.ptr = NULL;
    default_metric.slen = 0;
    
    const jw_str_t *metric = jw_config_get_string(config, &key_metric, &default_metric);
    if (metric && metric->ptr) {
        jw_str_t s1 = { metric->ptr, metric->slen };
        jw_str_t s2 = { "l2", 0 };
        s2.slen = jw_strlen(&s2);
        if (jw_strcmp(&s1, &s2) == 0) {
            coll_config->metric = JW_METRIC_L2;
        } else {
            jw_str_t s3 = { "ip", 0 };
            s3.slen = jw_strlen(&s3);
            if (jw_strcmp(&s1, &s3) == 0) {
                coll_config->metric = JW_METRIC_IP;
            } else {
                jw_str_t s4 = { "cosine", 0 };
                s4.slen = jw_strlen(&s4);
                if (jw_strcmp(&s1, &s4) == 0) {
                    coll_config->metric = JW_METRIC_COSINE;
                }
            }
        }
    }
    
    jw_str_t key_index_type;
    key_index_type.ptr = "collection.index_type";
    jw_str_t temp_index_type = { "collection.index_type", 0 };
    key_index_type.slen = jw_strlen(&temp_index_type);
    
    jw_str_t default_index_type;
    default_index_type.ptr = NULL;
    default_index_type.slen = 0;
    
    const jw_str_t *index_type = jw_config_get_string(config, &key_index_type, &default_index_type);
    if (index_type && index_type->ptr) {
        jw_str_t s1 = { index_type->ptr, index_type->slen };
        jw_str_t s2 = { "hnsw", 0 };
        s2.slen = jw_strlen(&s2);
        if (jw_strcmp(&s1, &s2) == 0) {
            coll_config->index_type = JW_INDEX_HNSW;
        } else {
            jw_str_t s3 = { "ivf", 0 };
            s3.slen = jw_strlen(&s3);
            if (jw_strcmp(&s1, &s3) == 0) {
                coll_config->index_type = JW_INDEX_IVF;
            } else {
                jw_str_t s4 = { "flat", 0 };
                s4.slen = jw_strlen(&s4);
                if (jw_strcmp(&s1, &s4) == 0) {
                    coll_config->index_type = JW_INDEX_FLAT;
                }
            }
        }
    }
    
    jw_str_t key_auto_resize;
    key_auto_resize.ptr = "collection.auto_resize";
    jw_str_t temp_auto_resize = { "collection.auto_resize", 0 };
    key_auto_resize.slen = jw_strlen(&temp_auto_resize);
    
    coll_config->auto_resize = jw_config_get_bool(config, &key_auto_resize, JW_TRUE);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_parse_args(int argc, char *argv[], jw_config_t *config)
{
    if (!config) {
        return JW_INVALID_PARAM;
    }
    
    /* 简单的命令行参数解析 */
    /* TODO: 实现完整的命令行参数解析 */
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_load_env(jw_config_t *config)
{
    if (!config) {
        return JW_INVALID_PARAM;
    }

    /* 从环境变量加载配置 */
    /* 环境变量格式: JW_<SECTION>_<KEY> */

    /* TODO: 实现环境变量加载 */

    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_apply_preset_mobile_low(jw_config_t *config)
{
    if (!config) {
        return JW_INVALID_PARAM;
    }

    jw_str_t key;
    jw_int64_t value;

    key.ptr = "index.hnsw.M";
    key.slen = strlen(key.ptr);
    value = 16;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_construction";
    key.slen = strlen(key.ptr);
    value = 100;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_search";
    key.slen = strlen(key.ptr);
    value = 50;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_apply_preset_mobile_high(jw_config_t *config)
{
    if (!config) {
        return JW_INVALID_PARAM;
    }

    jw_str_t key;
    jw_int64_t value;

    key.ptr = "index.hnsw.M";
    key.slen = strlen(key.ptr);
    value = 32;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_construction";
    key.slen = strlen(key.ptr);
    value = 200;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_search";
    key.slen = strlen(key.ptr);
    value = 100;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    return JW_SUCCESS;
}

JW_API jw_status_t jw_config_apply_preset_embedded(jw_config_t *config)
{
    if (!config) {
        return JW_INVALID_PARAM;
    }

    jw_str_t key;
    jw_int64_t value;

    key.ptr = "index.hnsw.M";
    key.slen = strlen(key.ptr);
    value = 8;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_construction";
    key.slen = strlen(key.ptr);
    value = 50;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    key.ptr = "index.hnsw.ef_search";
    key.slen = strlen(key.ptr);
    value = 32;
    jw_config_set(config, &key, &value, JW_CONFIG_TYPE_INT);

    return JW_SUCCESS;
}
