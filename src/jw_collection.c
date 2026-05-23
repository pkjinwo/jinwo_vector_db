/*
 * jw_collection.c - JinWo VecDB 向量集合实现
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_collection.h"
#include "jw_stdio.h"
#include "jw_string.h"
#include "jw_storage.h"
#include "jw_hash.h"
#include "jw_math.h"

/*
 * =============================================================================
 * 辅助函数
 * =============================================================================
 */

/* 创建默认索引配置 */
static void init_default_index_config(jw_index_config_t *config, jw_dim_t dim)
{
    jw_memset(config, 0, sizeof(jw_index_config_t));
    config->type = JW_INDEX_IVF;
    config->metric = JW_METRIC_L2;
    config->dim = dim;
    config->params.ivf.nlist = 5;
    config->params.ivf.nprobe = 3;
}

/* 扩展记录数组 */
static jw_status_t expand_records(jw_collection_t *coll)
{
    if (coll->count < coll->capacity) {
        return JW_SUCCESS;
    }
    
    jw_size_t new_capacity = coll->capacity * 2;
    if (new_capacity < 1024) {
        new_capacity = 1024;
    }
    
    jw_record_t *new_records = jw_arena_calloc(coll->arena, new_capacity, sizeof(jw_record_t));
    if (new_records == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    if (coll->records != NULL) {
        jw_memcpy(new_records, coll->records, coll->count * sizeof(jw_record_t));
    }
    
    coll->records = new_records;
    coll->capacity = new_capacity;
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * Collection 生命周期
 * =============================================================================
 */

JW_API jw_collection_t *jw_collection_create(jw_arena_t *arena,
                                              const jw_collection_config_t *config)
{
    if (config == NULL || config->dimension == 0) {
        return NULL;
    }
    
    jw_arena_t *local_arena = arena;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
    }
    
    jw_collection_t *coll = jw_arena_calloc(local_arena, 1, sizeof(jw_collection_t));
    if (coll == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }
    
    coll->arena = local_arena;
    coll->owns_arena = (arena == NULL) ? JW_TRUE : JW_FALSE;  /* 只有自己创建的arena才负责销毁 */
    coll->dim = config->dimension;
    coll->metric = config->metric;
    coll->config = *config;
    
    /* 复制名称 */
    if (config->name.ptr != NULL) {
        coll->name = jw_arena_strdup(local_arena, (const char*)config->name.ptr);
    }
    
    /* 初始化记录存储 */
    coll->capacity = 4096;
    coll->records = jw_arena_calloc(local_arena, coll->capacity, sizeof(jw_record_t));
    if (coll->records == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }
    coll->count = 0;
    coll->next_vid = 1;  /* ID从1开始，0表示无效 */
    
    /* 创建索引 */
    jw_index_config_t index_config;
    if (config->index_type != JW_INDEX_NONE) {
        index_config.type = config->index_type;
        index_config.dim = config->dimension;
        index_config.metric = config->metric;
        index_config.quant = config->quant;
        index_config.pq.nsub = config->pq.nsub;
        index_config.pq.nbits = config->pq.nbits;
        index_config.pq.max_iter = 20; /* 默认值 */
    } else {
        init_default_index_config(&index_config, config->dimension);
        index_config.quant = config->quant;
        index_config.pq.nsub = config->pq.nsub;
        index_config.pq.nbits = config->pq.nbits;
        index_config.pq.max_iter = 20; /* 默认值 */
    }
    
    coll->index = jw_index_create(local_arena, &index_config);
    coll->index_enabled = JW_TRUE;
    coll->index_threshold = 100;  /* 100个向量后建索引 */
    
    /* 初始化锁 */
    jw_rwlock_create(local_arena, NULL, &coll->lock);
    
    /* 设置时间戳 */
    coll->create_time = jw_time_now();
    coll->update_time = coll->create_time;
    coll->created = JW_TRUE;
    
    return coll;
}

JW_API void jw_collection_destroy(jw_collection_t *coll)
{
    if (coll == NULL) {
        return;
    }
    
    jw_rwlock_destroy(coll->lock);
    
    if (coll->index != NULL) {
        jw_index_destroy(coll->index);
    }
    
    /* 释放arena - 只有当arena是内部创建的时候才释放 */
    if (coll->arena != NULL && coll->owns_arena) {
        jw_arena_destroy(coll->arena);
    }
}

JW_API const char *jw_collection_get_name(const jw_collection_t *coll)
{
    return (coll != NULL) ? coll->name : NULL;
}

JW_API jw_status_t jw_collection_get_stats(const jw_collection_t *coll,
                                            jw_collection_stats_t *stats)
{
    if (coll == NULL || stats == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_rdlock(coll->lock);
    
    stats->count = coll->count;
    stats->capacity = coll->capacity;
    stats->dim = coll->dim;
    stats->index_type = coll->index ? coll->index->type : JW_INDEX_NONE;
    stats->index_ready = coll->index ? jw_index_is_trained(coll->index) : JW_FALSE;
    stats->memory_used = jw_arena_get_used_size(coll->arena);
    
    jw_rwlock_rdunlock(coll->lock);
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 向量插入
 * =============================================================================
 */

JW_API jw_status_t jw_collection_insert(jw_collection_t *coll,
                                         jw_cvec_t vec,
                                         jw_vid_t *vid)
{
    return jw_collection_insert_with_meta(coll, vec, NULL, 0, vid);
}

JW_API jw_status_t jw_collection_insert_with_meta(jw_collection_t *coll,
                                                   jw_cvec_t vec,
                                                   const jw_meta_field_t *fields,
                                                   jw_size_t field_count,
                                                   jw_vid_t *vid)
{
    if (coll == NULL || vec == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    /* 扩展记录数组 */
    jw_status_t status = expand_records(coll);
    if (status != JW_SUCCESS) {
        jw_rwlock_wrunlock(coll->lock);
        return status;
    }
    
    /* 分配ID */
    jw_vid_t new_vid = coll->next_vid++;
    
    /* 创建记录 */
    jw_record_t *record = &coll->records[coll->count];
    record->vid = new_vid;
    record->vec = jw_vec_copy(coll->arena, vec, coll->dim);
    
    if (record->vec == NULL) {
        jw_rwlock_wrunlock(coll->lock);
        return JW_OUT_OF_MEMORY;
    }
    
    /* 复制元数据 */
    if (fields != NULL && field_count > 0) {
        record->fields = jw_arena_alloc(coll->arena, field_count * sizeof(jw_meta_field_t));
        if (record->fields != NULL) {
            jw_memcpy(record->fields, fields, field_count * sizeof(jw_meta_field_t));
            record->field_count = field_count;
        }
    }
    
    coll->count++;
    coll->update_time = jw_time_now();
    
    /* 添加到索引 */
    if (coll->index != NULL && coll->index_enabled && coll->count >= coll->index_threshold) {
        jw_status_t add_status = jw_index_add(coll->index, new_vid, vec);
        if (add_status == JW_INDEX_NOT_READY) {
            /* 索引未训练，自动训练 */
            jw_cvec_t vectors = jw_arena_alloc(coll->arena, coll->count * coll->dim * sizeof(jw_float32_t));
            jw_vid_t *vids = jw_arena_alloc(coll->arena, coll->count * sizeof(jw_vid_t));
            
            if (vectors != NULL && vids != NULL) {
                for (jw_size_t i = 0; i < coll->count; i++) {
                    jw_memcpy((void*)(vectors + i * coll->dim), coll->records[i].vec,
                           coll->dim * sizeof(jw_float32_t));
                    vids[i] = coll->records[i].vid;
                }
                
                jw_status_t train_status = jw_index_train(coll->index, vectors, coll->count);
                if (train_status == JW_SUCCESS) {
                    /* 训练成功后重新添加 */
                    jw_index_add_batch(coll->index, vids, vectors, coll->count);
                }
            }
        }
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    if (vid != NULL) {
        *vid = new_vid;
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_collection_insert_record(jw_collection_t *coll,
                                                const jw_record_t *record)
{
    if (coll == NULL || record == NULL) {
        return JW_INVALID_PARAM;
    }
    
    return jw_collection_insert_with_meta(coll, record->vec,
                                           record->fields, record->field_count, NULL);
}

JW_API jw_status_t jw_collection_insert_batch(jw_collection_t *coll,
                                               jw_cvec_t vectors,
                                               jw_size_t count,
                                               jw_vid_t *vids)
{
    if (coll == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_vid_t *new_vids = jw_malloc(count * sizeof(jw_vid_t));
    if (new_vids == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    for (jw_size_t i = 0; i < count; i++) {
        jw_status_t status = jw_collection_insert(coll, vectors + i * coll->dim, &new_vids[i]);
        if (status != JW_SUCCESS) {
            jw_free(new_vids);
            return status;
        }
    }
    
    if (vids != NULL) {
        jw_memcpy(vids, new_vids, count * sizeof(jw_vid_t));
    }
    
    jw_free(new_vids);
    return JW_SUCCESS;
}

JW_API jw_status_t jw_collection_upsert(jw_collection_t *coll,
                                         jw_vid_t vid,
                                         jw_cvec_t vec)
{
    if (coll == NULL || vec == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    /* 查找是否已存在 */
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            /* 更新向量 */
            jw_memcpy(coll->records[i].vec, vec, coll->dim * sizeof(jw_float32_t));
            coll->update_time = jw_time_now();
            jw_rwlock_wrunlock(coll->lock);
            return JW_SUCCESS;
        }
    }
    
    /* 不存在则插入 */
    jw_rwlock_wrunlock(coll->lock);
    
    jw_vid_t new_vid;
    return jw_collection_insert(coll, vec, &new_vid);
}

/*
 * =============================================================================
 * 向量删除
 * =============================================================================
 */

JW_API jw_status_t jw_collection_delete(jw_collection_t *coll, jw_vid_t vid)
{
    if (coll == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    jw_status_t status = JW_NOT_FOUND;
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            /* 从索引中删除 */
            if (coll->index != NULL) {
                jw_index_remove(coll->index, vid);
            }
            
            /* 移动后面的记录 */
            if (i < coll->count - 1) {
                jw_memmove(&coll->records[i], &coll->records[i + 1],
                        (coll->count - i - 1) * sizeof(jw_record_t));
            }
            
            coll->count--;
            coll->update_time = jw_time_now();
            status = JW_SUCCESS;
            break;
        }
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    return status;
}

JW_API jw_status_t jw_collection_delete_batch(jw_collection_t *coll,
                                               const jw_vid_t *vids,
                                               jw_size_t count)
{
    if (coll == NULL || vids == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    for (jw_size_t i = 0; i < count; i++) {
        jw_status_t status = jw_collection_delete(coll, vids[i]);
        if (status != JW_SUCCESS && status != JW_NOT_FOUND) {
            return status;
        }
    }
    
    return JW_SUCCESS;
}

/* 检查记录是否匹配过滤器 */
static jw_bool_t match_filter(const jw_record_t *record, const jw_filter *filter)
{
    if (filter == NULL) {
        return JW_TRUE;
    }
    
    /* 处理逻辑操作符 */
    if (filter->logic == JW_LOGIC_AND) {
        for (jw_size_t i = 0; i < filter->child_count; i++) {
            if (!match_filter(record, filter->children[i])) {
                return JW_FALSE;
            }
        }
        return JW_TRUE;
    } else if (filter->logic == JW_LOGIC_OR) {
        for (jw_size_t i = 0; i < filter->child_count; i++) {
            if (match_filter(record, filter->children[i])) {
                return JW_TRUE;
            }
        }
        return JW_FALSE;
    } else if (filter->logic == JW_LOGIC_NOT) {
        return !match_filter(record, filter->children[0]);
    }
    
    /* 处理简单条件 */
    const char *field_name = filter->field;
    if (field_name != NULL) {
        jw_bool_t found = JW_FALSE;
        
        /* 在记录的元数据中查找对应的字段 */
        for (jw_size_t j = 0; j < record->field_count; j++) {
            const jw_meta_field_t *field = &record->fields[j];
            
            if (jw_strcasecmp(&field->name, field_name) == 0) {
                found = JW_TRUE;
                
                /* 根据字段类型进行比较 */
                switch (field->type) {
                    case JW_META_STRING:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (jw_strcasecmp(&field->value.str, &filter->value.str) != 0) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (jw_strcasecmp(&field->value.str, &filter->value.str) == 0) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GT:
                            case JW_CMP_LT:
                            case JW_CMP_GE:
                            case JW_CMP_LE:
                                /* 字符串不支持比较操作 */
                                return JW_FALSE;
                        }
                        break;
                        
                    case JW_META_INT32:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (field->value.i32 != filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (field->value.i32 == filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GT:
                                if (field->value.i32 <= filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LT:
                                if (field->value.i32 >= filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GE:
                                if (field->value.i32 < filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LE:
                                if (field->value.i32 > filter->value.i32) {
                                    return JW_FALSE;
                                }
                                break;
                        }
                        break;
                        
                    case JW_META_INT64:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (field->value.i64 != filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (field->value.i64 == filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GT:
                                if (field->value.i64 <= filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LT:
                                if (field->value.i64 >= filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GE:
                                if (field->value.i64 < filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LE:
                                if (field->value.i64 > filter->value.i64) {
                                    return JW_FALSE;
                                }
                                break;
                        }
                        break;
                        
                    case JW_META_FLOAT:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (jw_math_abs_f32(field->value.f32 - filter->value.f32) > 1e-6) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (jw_math_abs_f32(field->value.f32 - filter->value.f32) <= 1e-6) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GT:
                                if (field->value.f32 <= filter->value.f32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LT:
                                if (field->value.f32 >= filter->value.f32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GE:
                                if (field->value.f32 < filter->value.f32) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LE:
                                if (field->value.f32 > filter->value.f32) {
                                    return JW_FALSE;
                                }
                                break;
                        }
                        break;
                        
                    case JW_META_DOUBLE:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (jw_math_abs_f64(field->value.f64 - filter->value.f64) > 1e-12) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (jw_math_abs_f64(field->value.f64 - filter->value.f64) <= 1e-12) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GT:
                                if (field->value.f64 <= filter->value.f64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LT:
                                if (field->value.f64 >= filter->value.f64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_GE:
                                if (field->value.f64 < filter->value.f64) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_LE:
                                if (field->value.f64 > filter->value.f64) {
                                    return JW_FALSE;
                                }
                                break;
                        }
                        break;
                        
                    case JW_META_BOOL:
                        switch (filter->cmp) {
                            case JW_CMP_EQ:
                                if (field->value.b != filter->value.b) {
                                    return JW_FALSE;
                                }
                                break;
                            case JW_CMP_NE:
                                if (field->value.b == filter->value.b) {
                                    return JW_FALSE;
                                }
                                break;
                            default:
                                /* 布尔值不支持比较操作 */
                                return JW_FALSE;
                        }
                        break;
                }
                
                break;
            }
        }
        
        /* 如果字段不存在且操作不是NE，则不匹配 */
        if (!found && filter->cmp != JW_CMP_NE) {
            return JW_FALSE;
        }
    }
    
    return JW_TRUE;
}

JW_API jw_ssize_t jw_collection_delete_by_filter(jw_collection_t *coll, 
                                                  const jw_filter *filter)
{
    if (coll == NULL) {
        return -1;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    jw_ssize_t deleted = 0;
    jw_size_t i = 0;
    
    while (i < coll->count) {
        jw_record_t *record = &coll->records[i];
        
        if (match_filter(record, filter)) {
            /* 从索引中删除 */
            if (coll->index != NULL) {
                jw_index_remove(coll->index, record->vid);
            }
            
            /* 移动后面的记录 */
            if (i < coll->count - 1) {
                jw_memmove(&coll->records[i], &coll->records[i + 1],
                        (coll->count - i - 1) * sizeof(jw_record_t));
            }
            
            coll->count--;
            deleted++;
        } else {
            i++;
        }
    }
    
    if (deleted > 0) {
        coll->update_time = jw_time_now();
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    return deleted;
}

JW_API jw_status_t jw_collection_clear(jw_collection_t *coll)
{
    if (coll == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    coll->count = 0;
    coll->next_vid = 1;
    coll->update_time = jw_time_now();
    
    /* 重建索引 */
    if (coll->index != NULL) {
        jw_index_destroy(coll->index);
        
        jw_index_config_t config;
        init_default_index_config(&config, coll->dim);
        coll->index = jw_index_create(coll->arena, &config);
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 向量查询
 * =============================================================================
 */

JW_API jw_status_t jw_collection_get(const jw_collection_t *coll,
                                      jw_vid_t vid,
                                      jw_vec_t vec)
{
    if (coll == NULL || vec == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_rdlock(coll->lock);
    
    jw_status_t status = JW_NOT_FOUND;
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            jw_memcpy(vec, coll->records[i].vec, coll->dim * sizeof(jw_float32_t));
            status = JW_SUCCESS;
            break;
        }
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return status;
}

JW_API jw_status_t jw_collection_get_record(const jw_collection_t *coll,
                                             jw_vid_t vid,
                                             const jw_record_t **record)
{
    if (coll == NULL || record == NULL) {
        return JW_INVALID_PARAM;
    }
    
    *record = NULL;
    
    jw_rwlock_rdlock(coll->lock);
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            *record = &coll->records[i];
            jw_rwlock_rdunlock(coll->lock);
            return JW_SUCCESS;
        }
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return JW_NOT_FOUND;
}

JW_API jw_size_t jw_collection_get_batch(const jw_collection_t *coll,
                                          const jw_vid_t *vids,
                                          jw_size_t count,
                                          jw_vec_t vectors)
{
    if (coll == NULL || vids == NULL || vectors == NULL || count == 0) {
        return 0;
    }
    
    jw_rwlock_rdlock(coll->lock);
    
    jw_size_t found = 0;
    
    for (jw_size_t i = 0; i < count; i++) {
        for (jw_size_t j = 0; j < coll->count; j++) {
            if (coll->records[j].vid == vids[i]) {
                jw_memcpy(vectors + i * coll->dim, coll->records[j].vec,
                       coll->dim * sizeof(jw_float32_t));
                found++;
                break;
            }
        }
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return found;
}

/*
 * =============================================================================
 * 向量搜索
 * =============================================================================
 */

JW_API jw_size_t jw_collection_search(const jw_collection_t *coll,
                                       jw_cvec_t query,
                                       const jw_search_options_t *options,
                                       jw_search_result_ex_t *results)
{
    if (coll == NULL || query == NULL || results == NULL) {
        return 0;
    }
    
    jw_size_t k = (options != NULL) ? options->k : 10;
    
    jw_rwlock_rdlock(coll->lock);
    
    jw_size_t result_count = 0;
    
    /* 使用索引搜索（只有索引有足够数据时才使用） */
    if (coll->index != NULL && coll->index_enabled && coll->count >= coll->index_threshold) {
        jw_search_result_t *index_results = jw_arena_alloc(coll->arena,
                                                           k * sizeof(jw_search_result_t));
        if (index_results != NULL) {
            result_count = jw_index_search(coll->index, query, k, index_results);
            
            /* 填充详细结果 */
            for (jw_size_t i = 0; i < result_count; i++) {
                results[i].vid = index_results[i].id;
                results[i].score = index_results[i].score;
                
                /* 查找完整记录 */
                if (options != NULL && (options->include_vectors || options->include_meta)) {
                    for (jw_size_t j = 0; j < coll->count; j++) {
                        if (coll->records[j].vid == results[i].vid) {
                            if (options->include_vectors) {
                                results[i].vec = coll->records[j].vec;
                            }
                            if (options->include_meta) {
                                results[i].fields = coll->records[j].fields;
                                results[i].field_count = coll->records[j].field_count;
                            }
                            break;
                        }
                    }
                }
            }
        }
    } else {
        /* 暴力搜索 */
        jw_size_t actual_k = (k < coll->count) ? k : coll->count;
        
        /* 计算所有距离 */
        typedef struct { jw_vid_t vid; jw_float32_t dist; } vec_dist_t;
        vec_dist_t *distances = jw_arena_alloc(coll->arena, coll->count * sizeof(vec_dist_t));
        
        if (distances != NULL) {
            for (jw_size_t i = 0; i < coll->count; i++) {
                distances[i].vid = coll->records[i].vid;
                distances[i].dist = jw_vec_distance(query, coll->records[i].vec,
                                                     coll->dim, coll->metric);
            }
            
            /* 部分排序 */
            for (jw_size_t i = 0; i < actual_k && i < coll->count; i++) {
                jw_size_t min_j = i;
                for (jw_size_t j = i + 1; j < coll->count; j++) {
                    if (distances[j].dist < distances[min_j].dist) {
                        min_j = j;
                    }
                }
                vec_dist_t tmp = distances[i];
                distances[i] = distances[min_j];
                distances[min_j] = tmp;
            }
            
            /* 填充结果 */
            for (jw_size_t i = 0; i < actual_k; i++) {
                results[i].vid = distances[i].vid;
                results[i].score = distances[i].dist;
                results[i].vec = NULL;
                results[i].fields = NULL;
                results[i].field_count = 0;
            }
            
            result_count = actual_k;
        }
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return result_count;
}

JW_API jw_size_t jw_collection_search_by_id(const jw_collection_t *coll,
                                             jw_vid_t vid,
                                             const jw_search_options_t *options,
                                             jw_search_result_ex_t *results)
{
    if (coll == NULL || results == NULL) {
        return 0;
    }
    
    jw_vec_t vec = jw_arena_alloc(coll->arena, coll->dim * sizeof(jw_float32_t));
    if (vec == NULL) {
        return 0;
    }
    
    jw_status_t status = jw_collection_get(coll, vid, vec);
    if (status != JW_SUCCESS) {
        return 0;
    }
    
    return jw_collection_search(coll, vec, options, results);
}

JW_API jw_size_t jw_collection_search_filtered(const jw_collection_t *coll,
                                                jw_cvec_t query,
                                                const jw_filter *filter,
                                                jw_size_t k,
                                                jw_search_result_ex_t *results)
{
    jw_search_options_t options = {
        .k = k,
        .filter = filter,
        .include_vectors = JW_FALSE,
        .include_meta = JW_FALSE
    };
    
    return jw_collection_search(coll, query, &options, results);
}

/*
 * =============================================================================
 * 索引管理
 * =============================================================================
 */

JW_API jw_status_t jw_collection_build_index(jw_collection_t *coll)
{
    if (coll == NULL || coll->count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    /* 收集所有向量 */
    jw_cvec_t vectors = jw_arena_alloc(coll->arena, coll->count * coll->dim * sizeof(jw_float32_t));
    jw_vid_t *vids = jw_arena_alloc(coll->arena, coll->count * sizeof(jw_vid_t));
    
    if (vectors == NULL || vids == NULL) {
        jw_rwlock_wrunlock(coll->lock);
        return JW_OUT_OF_MEMORY;
    }
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        jw_memcpy((void*)(vectors + i * coll->dim), coll->records[i].vec,
               coll->dim * sizeof(jw_float32_t));
        vids[i] = coll->records[i].vid;
    }
    
    /* 训练索引 */
    jw_status_t status = jw_index_train(coll->index, vectors, coll->count);
    if (status != JW_SUCCESS) {
        jw_rwlock_wrunlock(coll->lock);
        return status;
    }
    
    /* 添加向量到索引 */
    status = jw_index_add_batch(coll->index, vids, vectors, coll->count);
    
    jw_rwlock_wrunlock(coll->lock);
    
    return status;
}

JW_API jw_status_t jw_collection_rebuild_index(jw_collection_t *coll)
{
    if (coll == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* 删除旧索引 */
    jw_rwlock_wrlock(coll->lock);
    
    if (coll->index != NULL) {
        jw_index_destroy(coll->index);
        
        jw_index_config_t config;
        init_default_index_config(&config, coll->dim);
        coll->index = jw_index_create(coll->arena, &config);
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    /* 重建 */
    return jw_collection_build_index(coll);
}

JW_API jw_status_t jw_collection_drop_index(jw_collection_t *coll)
{
    if (coll == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    if (coll->index != NULL) {
        jw_index_destroy(coll->index);
        coll->index = NULL;
        coll->index_enabled = JW_FALSE;
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    return JW_SUCCESS;
}

JW_API jw_bool_t jw_collection_has_index(const jw_collection_t *coll)
{
    if (coll == NULL || coll->index == NULL) {
        return JW_FALSE;
    }
    
    return jw_index_is_trained(coll->index);
}

/*
 * =============================================================================
 * 元数据管理
 * =============================================================================
 */

JW_API jw_status_t jw_collection_set_meta(jw_collection_t *coll,
                                           jw_vid_t vid,
                                           const jw_meta_field_t *fields,
                                           jw_size_t field_count)
{
    if (coll == NULL || fields == NULL || field_count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_wrlock(coll->lock);
    
    jw_status_t status = JW_NOT_FOUND;
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            coll->records[i].fields = jw_arena_memdup(coll->arena, fields,
                                                      field_count * sizeof(jw_meta_field_t));
            coll->records[i].field_count = field_count;
            coll->update_time = jw_time_now();
            status = JW_SUCCESS;
            break;
        }
    }
    
    jw_rwlock_wrunlock(coll->lock);
    
    return status;
}

JW_API jw_status_t jw_collection_get_meta(const jw_collection_t *coll,
                                           jw_vid_t vid,
                                           jw_meta_field_t *fields,
                                           jw_size_t *field_count)
{
    if (coll == NULL || fields == NULL || field_count == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_rdlock(coll->lock);
    
    jw_status_t status = JW_NOT_FOUND;
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        if (coll->records[i].vid == vid) {
            jw_size_t count = (coll->records[i].field_count < *field_count) 
                            ? coll->records[i].field_count : *field_count;
            
            jw_memcpy(fields, coll->records[i].fields, count * sizeof(jw_meta_field_t));
            *field_count = coll->records[i].field_count;
            status = JW_SUCCESS;
            break;
        }
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return status;
}

/*
 * =============================================================================
 * 持久化
 * =============================================================================
 */

/* Collection 文件魔数 */
#define JW_COLLECTION_MAGIC  0x4A57434F  /* "JWCO" */
#define JW_COLLECTION_VERSION 0x00010000

JW_API jw_status_t jw_collection_save(const jw_collection_t *coll,
                                       const char *filepath)
{
    if (coll == NULL || filepath == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_rwlock_rdlock(coll->lock);
    
    /* 创建存储 */
    jw_storage_config_t storage_config = JW_STORAGE_CONFIG_DEFAULT;
    storage_config.path.ptr = (char *)filepath;
    storage_config.path.slen = strlen(filepath);
    storage_config.mode = JW_STORAGE_CREATE;
    storage_config.sync_on_write = JW_TRUE;
    
    jw_storage_t *storage = jw_storage_create(NULL, &storage_config);
    if (storage == NULL) {
        jw_rwlock_rdunlock(coll->lock);
        return JW_UNKNOWN_ERROR;
    }
    
    jw_status_t status = JW_SUCCESS;
    
    /* 准备头部 */
    jw_collection_header_fixed_t header;
    jw_memset(&header, 0, sizeof(header));
    
    header.magic = JW_COLLECTION_MAGIC;
    header.version = JW_COLLECTION_VERSION;
    header.create_time = coll->create_time;
    header.update_time = coll->update_time;
    
    /* 复制名称 */
    if (coll->name != NULL) {
        strncpy(header.name, coll->name, sizeof(header.name) - 1);
        header.name[sizeof(header.name) - 1] = '\0';
    }
    
    header.dim = coll->dim;
    header.metric = coll->metric;
    header.index_type = coll->index ? coll->index->type : JW_INDEX_NONE;
    header.vector_count = coll->count;
    header.next_vid = coll->next_vid;
    
    /* 索引配置 */
    if (coll->index != NULL && 
        (coll->index->type == JW_INDEX_HNSW || coll->index->type == JW_INDEX_HNSW_PQ)) {
        jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)coll->index->impl;
        if (hnsw != NULL) {
            header.hnsw_m = hnsw->config.M;
            header.hnsw_ef_construction = hnsw->config.ef_construction;
            header.hnsw_ef_search = hnsw->config.ef_search;
        }
    } else if (coll->index != NULL &&
               (coll->index->type == JW_INDEX_IVF || coll->index->type == JW_INDEX_IVF_PQ)) {
        jw_ivf_index_t *ivf = (jw_ivf_index_t *)coll->index->impl;
        if (ivf != NULL) {
            header.ivf_nlist = ivf->nlist;
            header.ivf_nprobe = ivf->config.nprobe;
        }
    }
    
    /* 写入头部 (预留空间，后面更新偏移量) */
    jw_uint64_t header_offset = 0;
    jw_uint64_t current_offset = 0;
    
    /* 先写入一个空的头部占位 */
    status = jw_storage_write(storage, &header, sizeof(header), &header_offset);
    if (status != JW_SUCCESS) {
        jw_storage_close(storage);
        jw_rwlock_rdunlock(coll->lock);
        return status;
    }
    
    current_offset = header_offset + sizeof(header);
    
    /* 写入向量数据 */
    header.vectors_offset = current_offset;
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        /* 写入向量记录头 */
        jw_vector_record_fixed_t vec_header;
        vec_header.vid = coll->records[i].vid;
        vec_header.dim = coll->dim;
        vec_header.flags = 0;
        vec_header.meta_offset = 0;
        vec_header.meta_size = 0;
        vec_header.checksum = 0;
        
        status = jw_storage_write(storage, &vec_header, sizeof(vec_header), &current_offset);
        if (status != JW_SUCCESS) {
            jw_storage_close(storage);
            jw_rwlock_rdunlock(coll->lock);
            return status;
        }
        
        /* 写入向量数据 (小端序) */
        status = jw_storage_write_f32_array(storage, coll->records[i].vec,
                                             coll->dim, &current_offset);
        if (status != JW_SUCCESS) {
            jw_storage_close(storage);
            jw_rwlock_rdunlock(coll->lock);
            return status;
        }
    }
    
    header.vectors_size = current_offset - header.vectors_offset;
    
    /* 更新头部 */
    header.update_time = jw_time_now();
    
    /* 计算校验和 */
    header.checksum = jw_hash_murmur3_32(&header, sizeof(header) - 4, 0);
    
    /* 回写头部 */
    status = jw_storage_write_collection_header(storage, &header, &header_offset);
    
    jw_storage_close(storage);
    jw_rwlock_rdunlock(coll->lock);
    
    return status;
}

JW_API jw_collection_t *jw_collection_load(jw_arena_t *arena, 
                                            const char *filepath)
{
    if (filepath == NULL) {
        return NULL;
    }
    
    /* 打开存储 */
    jw_storage_config_t storage_config = JW_STORAGE_CONFIG_DEFAULT;
    storage_config.path.ptr = (char *)filepath;
    storage_config.path.slen = strlen(filepath);
    storage_config.mode = JW_STORAGE_READ;
    
    jw_storage_t *storage = jw_storage_open(NULL, filepath, JW_STORAGE_READ);
    if (storage == NULL) {
        return NULL;
    }
    
    /* 读取头部 */
    jw_collection_header_fixed_t header;
    jw_status_t status = jw_storage_read_collection_header(storage, 0, &header);
    
    if (status != JW_SUCCESS || header.magic != JW_COLLECTION_MAGIC) {
        jw_storage_close(storage);
        return NULL;
    }
    
    /* 创建 Collection */
    jw_arena_t *local_arena = arena;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
    }
    
    jw_collection_t *coll = jw_arena_calloc(local_arena, 1, sizeof(jw_collection_t));
    if (coll == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        jw_storage_close(storage);
        return NULL;
    }
    
    coll->arena = local_arena;
    coll->owns_arena = (arena == NULL) ? JW_TRUE : JW_FALSE;
    coll->dim = header.dim;
    coll->metric = header.metric;
    coll->create_time = header.create_time;
    coll->update_time = header.update_time;
    
    /* 复制名称 */
    if (header.name[0] != '\0') {
        coll->name = jw_arena_strdup(local_arena, header.name);
    }
    
    /* 初始化记录存储 */
    coll->capacity = (header.vector_count > 0) ? header.vector_count * 2 : 1024;
    coll->records = jw_arena_calloc(local_arena, coll->capacity, sizeof(jw_record_t));
    coll->count = 0;
    coll->next_vid = header.next_vid;
    
    /* 初始化锁 */
    jw_rwlock_t *lock;
    jw_rwlock_create(local_arena, NULL, &lock);
    coll->lock = lock;
    
    /* 创建索引 */
    if (header.index_type != JW_INDEX_NONE) {
        jw_index_config_t index_config;
        jw_memset(&index_config, 0, sizeof(index_config));
        index_config.type = header.index_type;
        index_config.dim = header.dim;
        index_config.metric = header.metric;
        
        if (header.index_type == JW_INDEX_HNSW || header.index_type == JW_INDEX_HNSW_PQ) {
            index_config.params.hnsw.M = header.hnsw_m;
            index_config.params.hnsw.ef_construction = header.hnsw_ef_construction;
            index_config.params.hnsw.ef_search = header.hnsw_ef_search;
        } else if (header.index_type == JW_INDEX_IVF || header.index_type == JW_INDEX_IVF_PQ) {
            index_config.params.ivf.nlist = header.ivf_nlist;
            index_config.params.ivf.nprobe = header.ivf_nprobe;
        }
        
        coll->index = jw_index_create(local_arena, &index_config);
        coll->index_enabled = JW_TRUE;
    }
    
    /* 读取向量数据 */
    jw_uint64_t offset = header.vectors_offset;
    jw_uint64_t end_offset = offset + header.vectors_size;
    
    while (offset < end_offset && coll->count < header.vector_count) {
        /* 读取向量记录头 */
        jw_vector_record_fixed_t vec_header;
        jw_ssize_t bytes = jw_storage_read(storage, offset, &vec_header, sizeof(vec_header));
        
        if (bytes != sizeof(vec_header)) {
            break;
        }
        
        /* 转换字节序 */
        vec_header.vid = jw_letoh64(vec_header.vid);
        vec_header.dim = jw_letoh32(vec_header.dim);
        vec_header.flags = jw_letoh32(vec_header.flags);
        vec_header.meta_offset = jw_letoh64(vec_header.meta_offset);
        vec_header.meta_size = jw_letoh32(vec_header.meta_size);
        vec_header.checksum = jw_letoh32(vec_header.checksum);
        
        offset += sizeof(vec_header);
        
        /* 读取向量数据 */
        jw_record_t *record = &coll->records[coll->count];
        record->vid = vec_header.vid;
        record->vec = jw_vec_create(local_arena, coll->dim);
        record->field_count = 0;
        record->fields = NULL;
        
        if (record->vec == NULL) {
            break;
        }
        
        /* 读取向量 (小端序转换) */
        status = jw_storage_read_f32_array(storage, offset, record->vec, coll->dim);
        if (status != JW_SUCCESS) {
            break;
        }
        
        offset += coll->dim * sizeof(jw_float32_t);
        
        /* 添加到索引 */
        if (coll->index != NULL) {
            jw_index_add(coll->index, record->vid, record->vec);
        }
        
        coll->count++;
    }
    
    jw_storage_close(storage);
    
    coll->created = JW_TRUE;
    
    return coll;
}

/*
 * =============================================================================
 * 迭代器实现
 * =============================================================================
 */

JW_API jw_iterator_t *jw_collection_create_iterator(jw_collection_t *coll, 
                                                   const jw_filter *filter)
{
    if (coll == NULL) {
        return NULL;
    }
    
    jw_arena_t *arena = coll->arena;
    jw_iterator_t *iter = jw_arena_calloc(arena, 1, sizeof(jw_iterator_t));
    if (iter == NULL) {
        return NULL;
    }
    
    iter->collection = coll;
    iter->current = 0;
    iter->total = coll->count;
    iter->filter = (jw_filter *)filter;
    iter->valid = JW_TRUE;
    iter->arena = arena;
    
    /* 找到第一个匹配的元素 */
    if (filter != NULL) {
        while (iter->current < iter->total) {
            jw_record_t *record = &coll->records[iter->current];
            if (match_filter(record, filter)) {
                break;
            }
            iter->current++;
        }
        
        if (iter->current >= iter->total) {
            iter->valid = JW_FALSE;
        }
    }
    
    return iter;
}

JW_API void jw_collection_destroy_iterator(jw_iterator_t *iter)
{
    if (iter != NULL) {
        jw_free(iter);
    }
}

JW_API jw_bool_t jw_collection_iterator_valid(const jw_iterator_t *iter)
{
    return (iter != NULL) ? iter->valid : JW_FALSE;
}

JW_API jw_bool_t jw_collection_iterator_next(jw_iterator_t *iter)
{
    if (iter == NULL || !iter->valid) {
        return JW_FALSE;
    }
    
    jw_collection_t *coll = (jw_collection_t *)iter->collection;
    
    /* 移动到下一个元素 */
    iter->current++;
    
    /* 如果有过滤器，找到下一个匹配的元素 */
    if (iter->filter != NULL) {
        while (iter->current < iter->total) {
            jw_record_t *record = &coll->records[iter->current];
            if (match_filter(record, iter->filter)) {
                break;
            }
            iter->current++;
        }
    }
    
    /* 检查是否到达末尾 */
    if (iter->current >= iter->total) {
        iter->valid = JW_FALSE;
        return JW_FALSE;
    }
    
    return JW_TRUE;
}

JW_API jw_vid_t jw_collection_iterator_get_id(const jw_iterator_t *iter)
{
    if (iter == NULL || !iter->valid) {
        return 0;
    }
    
    jw_collection_t *coll = (jw_collection_t *)iter->collection;
    return coll->records[iter->current].vid;
}

JW_API jw_cvec_t jw_collection_iterator_get_vector(const jw_iterator_t *iter)
{
    if (iter == NULL || !iter->valid) {
        return NULL;
    }
    
    jw_collection_t *coll = (jw_collection_t *)iter->collection;
    return coll->records[iter->current].vec;
}

JW_API jw_status_t jw_collection_iterator_get_record(const jw_iterator_t *iter, 
                                                    const jw_record_t **record)
{
    if (iter == NULL || !iter->valid || record == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_collection_t *coll = (jw_collection_t *)iter->collection;
    *record = &coll->records[iter->current];
    return JW_SUCCESS;
}

JW_API jw_size_t jw_collection_foreach(jw_collection_t *coll, 
                                       const jw_filter *filter, 
                                       jw_iterator_callback_t callback, 
                                       void *user_data)
{
    if (coll == NULL || callback == NULL) {
        return 0;
    }
    
    jw_size_t count = 0;
    
    jw_rwlock_rdlock(coll->lock);
    
    for (jw_size_t i = 0; i < coll->count; i++) {
        jw_record_t *record = &coll->records[i];
        
        /* 检查过滤条件 */
        if (filter != NULL && !match_filter(record, filter)) {
            continue;
        }
        
        /* 调用回调 */
        jw_bool_t continue_iteration = callback(record->vid, record->vec, coll->dim, user_data);
        if (!continue_iteration) {
            break;
        }
        
        count++;
    }
    
    jw_rwlock_rdunlock(coll->lock);
    
    return count;
}
