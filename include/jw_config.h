/*
 * jw_config.h - JinWo VecDB 配置管理
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
 * 配置管理说明:
 * 
 * 负责从配置文件加载参数，支持:
 *   - JSON格式配置文件
 *   - 环境变量覆盖
 *   - 命令行参数覆盖
 * 
 * 版本: 0.1.0
 * 作者: 灵活就业码农
 */

#ifndef JW_CONFIG_H
#define JW_CONFIG_H

#include "jw_types.h"
#include "jw_arena.h"
#include "jw_vecdb.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 配置结构
 * =============================================================================
 */

/**
 * 配置对象
 */
typedef struct jw_config_t {
    jw_arena_t *arena;
    void *data;
} jw_config_t;

/**
 * 配置项类型
 */
typedef enum jw_config_type {
    JW_CONFIG_TYPE_INT = 0,
    JW_CONFIG_TYPE_FLOAT,
    JW_CONFIG_TYPE_STRING,
    JW_CONFIG_TYPE_BOOL,
    JW_CONFIG_TYPE_OBJECT,
    JW_CONFIG_TYPE_ARRAY
} jw_config_type_t;

/*
 * =============================================================================
 * 配置API
 * =============================================================================
 */

/**
 * 创建配置对象
 * 
 * @param arena 内存池
 * @return 配置对象
 */
JW_API jw_config_t *jw_config_create(jw_arena_t *arena);

/**
 * 销毁配置对象
 * 
 * @param config 配置对象
 */
JW_API void jw_config_destroy(jw_config_t *config);

/**
 * 加载配置文件
 * 
 * @param config 配置对象
 * @param filename 配置文件路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_load(jw_config_t *config, jw_str_t *filename);

/**
 * 从字符串加载配置
 * 
 * @param config 配置对象
 * @param json_str JSON字符串
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_load_string(jw_config_t *config, jw_str_t *json_str);

/**
 * 保存配置到文件
 *
 * @param config 配置对象
 * @param filename 配置文件路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_save(jw_config_t *config, jw_str_t *filename);

/**
 * 获取整数配置
 * 
 * @param config 配置对象
 * @param key 配置键
 * @param default_val 默认值
 * @return 配置值
 */
JW_API jw_int64_t jw_config_get_int(jw_config_t *config, const jw_str_t *key, jw_int64_t default_val);

/**
 * 获取浮点数配置
 * 
 * @param config 配置对象
 * @param key 配置键
 * @param default_val 默认值
 * @return 配置值
 */
JW_API jw_float64_t jw_config_get_float(jw_config_t *config, const jw_str_t *key, jw_float64_t default_val);

/**
 * 获取字符串配置
 * 
 * @param config 配置对象
 * @param key 配置键
 * @param default_val 默认值
 * @return 配置值 (内部字符串，不要释放)
 */
JW_API const jw_str_t *jw_config_get_string(jw_config_t *config, const jw_str_t *key, const jw_str_t *default_val);

/**
 * 获取布尔配置
 * 
 * @param config 配置对象
 * @param key 配置键
 * @param default_val 默认值
 * @return 配置值
 */
JW_API jw_bool_t jw_config_get_bool(jw_config_t *config, const jw_str_t *key, jw_bool_t default_val);

/**
 * 设置配置值
 * 
 * @param config 配置对象
 * @param key 配置键
 * @param value 值
 * @param type 值类型
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_set(jw_config_t *config, const jw_str_t *key, const void *value, jw_config_type_t type);

/**
 * 检查配置键是否存在
 * 
 * @param config 配置对象
 * @param key 配置键
 * @return JW_TRUE 存在
 */
JW_API jw_bool_t jw_config_has_key(jw_config_t *config, const jw_str_t *key);

/**
 * 从配置创建数据库配置
 * 
 * @param config 配置对象
 * @param db_config 数据库配置
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_to_db_config(jw_config_t *config, jw_vecdb_config_t *db_config);

/**
 * 从配置创建集合配置
 * 
 * @param config 配置对象
 * @param coll_config 集合配置
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_to_collection_config(jw_config_t *config, jw_collection_config_t *coll_config);

/*
 * =============================================================================
 * 工具函数
 * =============================================================================
 */

/**
 * 解析命令行参数
 * 
 * @param argc 参数数量
 * @param argv 参数数组
 * @param config 配置对象
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_parse_args(int argc, char *argv[], jw_config_t *config);

/**
 * 从环境变量加载配置
 * 
 * @param config 配置对象
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_load_env(jw_config_t *config);

/*
 * =============================================================================
 * 平台配置预设
 * =============================================================================
 */

/**
 * 获取移动端低资源配置预设
 *
 * 适用于内存<2GB的移动设备，自动调整以下参数:
 *   - HNSW M: 16 (减少内存占用)
 *   - HNSW ef_construction: 100 (减少建索引时间)
 *   - HNSW ef_search: 50 (减少搜索时间)
 *
 * @param config 配置对象
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_apply_preset_mobile_low(jw_config_t *config);

/**
 * 获取移动端高资源配置预设
 *
 * 适用于内存>4GB的高端移动设备:
 *   - HNSW M: 32 (更高的搜索精度)
 *   - HNSW ef_construction: 200
 *   - HNSW ef_search: 100
 *
 * @param config 配置对象
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_apply_preset_mobile_high(jw_config_t *config);

/**
 * 获取嵌入式设备配置预设
 *
 * 适用于树莓派等嵌入式设备:
 *   - HNSW M: 8 (最小内存占用)
 *   - HNSW ef_construction: 50
 *   - HNSW ef_search: 32
 *
 * @param config 配置对象
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_config_apply_preset_embedded(jw_config_t *config);

/*
 * =============================================================================
 * 配置文件格式说明
 * =============================================================================
 */

/*
配置文件示例 (JSON格式):

{
    "database": {
        "path": "./data",
        "storage_mode": "persistent",
        "arena_size": 104857600,  // 100MB
        "cache_size": 52428800,   // 50MB
        "read_only": false,
        "create_if_missing": true
    },
    "collection": {
        "name": "default",
        "dimension": 128,
        "metric": "l2",
        "index_type": "hnsw",
        "auto_id": true,
        "persist": true,
        "index": {
            "hnsw": {
                "M": 16,
                "ef_construction": 200,
                "ef_search": 100
            },
            "ivf": {
                "nlist": 100,
                "nprobe": 10
            }
        }
    },
    "logging": {
        "level": "info",
        "file": "./logs/jw_vecdb.log",
        "stdout": true
    }
}
*/

JW_END_DECL

#endif /* JW_CONFIG_H */
