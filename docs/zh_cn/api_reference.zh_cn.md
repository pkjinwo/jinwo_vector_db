# JinWo VecDB C API 参考

> 本文档根据仓库 `include/` 下的真实头文件（`jw_vecdb.h`、`jw_collection.h`、`jw_types.h`）整理，
> 确保与当前动态库的 ABI 一致。旧版本文档里的一部分函数签名（如
> `jw_vecdb_open(db**, char*, int)`、`jw_collection_create(&col, db, name, dim)`、
> `jw_collection_search(col, q, k, &results, &count)`）以及若干并不存在的函数
> （`jw_collection_open` / `jw_collection_close` / `jw_collection_list` /
> `jw_collection_batch_insert` / `jw_collection_create_index` / `jw_distance_*` /
> `jw_vector_*` / `jw_thread_pool_*` / 旧版 `jw_config_*`）已按真实实现更正或移除，见文末说明。

---

## 1. 基础类型与约定

### 1.1 句柄（不透明指针）

```c
typedef struct jw_vecdb_t      jw_vecdb_t;       // 数据库句柄
typedef struct jw_collection_t jw_collection_t;  // 集合句柄
typedef struct jw_arena_t      jw_arena_t;       // 内存竞技场（底层使用）
```

二者均为不透明类型，用户无需（也不应）访问内部结构，统一通过下列函数操作。

### 1.2 状态与布尔

```c
typedef int32_t  jw_status_t;   // 返回值：0 表示成功，负数表示错误
typedef int32_t  jw_bool_t;     // JW_TRUE(1) / JW_FALSE(0)
typedef uint64_t jw_size_t;
typedef uint32_t jw_dim_t;      // 向量维度
typedef uint64_t jw_vid_t;      // 向量 id（插入时返回）
typedef uint64_t jw_vec_id_t;   // 搜索结果里的向量 id
typedef float    jw_score_t;    // 相似度得分
typedef float*          jw_vec_t;   // 可读写向量
typedef const float*    jw_cvec_t;  // 只读向量
```

成功码为 `JW_SUCCESS`（值为 `0`），所有错误码均为负数（见第 8 节）。

### 1.3 字符串类型 `jw_str_t`

路径、集合名等一律用 `jw_str_t`（指针 + 长度），**不是**以 `\0` 结尾的 `char*`：

```c
typedef struct {
    const char *ptr;   // 字节内容（不一定以 \0 结尾）
    jw_size_t   slen;  // 字节长度
} jw_str_t;
```

便捷宏：

```c
#define jw_str(s)    ((jw_str_t){(char *)(s), sizeof(s) - 1})  // 仅适用于字符串字面量
#define JW_STR_NULL  {NULL, 0}
#define JW_STR_SET(str, s)  do { (str).ptr = (char *)(s); (str).slen = strlen(s); } while(0)
```

> ⚠️ `jw_str("字面量")` 依赖 `sizeof`，**只能用于字符串字面量**；运行期字符串请用
> `JW_STR_SET(str, s)` 或手动赋值 `{ ptr, slen }`。

### 1.4 搜索结果：`jw_search_result_t`（数据库级 `jw_vecdb_search` 使用）

```c
typedef struct {
    jw_vec_id_t   id;             // 向量 id
    jw_score_t    score;          // 相似度得分（cosine 距离下，越大越相似）
    jw_float32_t *vector;         // 命中向量（库内指针，仅本次调用有效）
    jw_size_t     dimension;      // 向量维度
    void         *metadata;       // 元数据（若有）
    jw_size_t     metadata_size;  // 元数据长度
} jw_search_result_t;
```

### 1.5 搜索选项与扩展结果：`jw_search_options_t` / `jw_search_result_ex_t`（集合级 `jw_collection_search` 使用）

```c
typedef struct {
    jw_size_t            k;               // 返回 Top-K
    const jw_filter_t   *filter;          // 过滤条件（可为 NULL）
    jw_bool_t            include_vectors; // 结果是否带回向量
    jw_bool_t            include_metadata;// 结果是否带回元数据
    jw_size_t            nprobe;          // 量化索引的探测桶数
    jw_size_t            ef_search;       // HNSW 搜索参数
} jw_search_options_t;

typedef struct {
    jw_vid_t       vid;          // 向量 id
    jw_score_t     score;        // 相似度得分
    jw_float32_t  *vec;          // 向量（若 include_vectors）
    jw_field_t    *fields;       // 字段（若 include_metadata）
    jw_size_t      field_count;  // 字段数量
} jw_search_result_ex_t;
```

### 1.6 打开标志（`jw_vecdb_open` 的 `flags` 参数）

| 宏 | 值 | 含义 |
|---|---|---|
| `JW_VECDB_READONLY`  | `0x01` | 只读 |
| `JW_VECDB_READWRITE` | `0x02` | 读写 |
| `JW_VECDB_CREATE`    | `0x04` | 不存在则创建 |
| `JW_VECDB_TRUNCATE`  | `0x08` | 打开时清空 |
| `JW_VECDB_MEMORY`    | `0x10` | 纯内存模式（不落盘） |
| `JW_VECDB_NOMMAP`    | `0x20` | 禁用 mmap |
| `JW_VECDB_NOCACHE`   | `0x40` | 禁用缓存 |
| `JW_VECDB_SYNC`      | `0x80` | 每次写入同步落盘 |

---

## 2. 数据库（Database）

```c
// 打开/创建数据库：path 为 jw_str_t*，flags 见 1.6，db 输出句柄
jw_status_t jw_vecdb_open(const jw_str_t *path, jw_uint32_t flags, jw_vecdb_t **db);

// 用配置结构体打开（高级用法）
jw_status_t jw_vecdb_open_ex(const jw_vecdb_config_t *config, jw_vecdb_t **db);

// 关闭数据库（会先同步落盘）
jw_status_t jw_vecdb_close(jw_vecdb_t *db);

// 手动触发落盘
jw_status_t jw_vecdb_sync(jw_vecdb_t *db);

// 是否已打开
jw_bool_t   jw_vecdb_is_open(const jw_vecdb_t *db);
```

> 返回 `JW_SUCCESS`（0）表示成功；非 0 表示失败，可用 `jw_vecdb_strerror()` 取错误信息（见第 8 节）。

---

## 3. 集合管理（数据库级，按集合名操作）

```c
// 创建集合（dim 为向量维度）
jw_status_t jw_vecdb_create_collection(jw_vecdb_t *db, const jw_str_t *name,
                                       jw_dim_t dim, jw_collection_t **coll);

// 用完整配置创建集合
jw_status_t jw_vecdb_create_collection_ex(jw_vecdb_t *db, const jw_str_t *name,
                                          const jw_collection_config_t *config,
                                          jw_collection_t **coll);

// 按名获取已存在集合的句柄（不存在返回 NULL）
jw_collection_t *jw_vecdb_get_collection(jw_vecdb_t *db, const jw_str_t *name);

// 删除集合
jw_status_t jw_vecdb_drop_collection(jw_vecdb_t *db, const jw_str_t *name);

// 集合是否存在
jw_bool_t   jw_vecdb_has_collection(const jw_vecdb_t *db, const jw_str_t *name);

// 列举集合名（names 为长度 capacity 的 jw_str_t 数组，返回实际数量）
jw_size_t   jw_vecdb_list_collections(const jw_vecdb_t *db,
                                      jw_str_t *names, jw_size_t capacity);

// 重命名集合
jw_status_t jw_vecdb_rename_collection(jw_vecdb_t *db,
                                       const jw_str_t *old_name,
                                       const jw_str_t *new_name);
```

---

## 4. 便捷读写（数据库级，直接按集合名读写）

无需先持有 `jw_collection_t*` 句柄，传集合名即可：

```c
// 插入单条，返回 vid
jw_status_t jw_vecdb_insert(jw_vecdb_t *db, const jw_str_t *collection,
                            jw_cvec_t vector, jw_dim_t dim, jw_vid_t *vid);

// 批量插入，vectors 为 dim*count 的连续数组，vids 输出长度为 count
jw_status_t jw_vecdb_insert_batch(jw_vecdb_t *db, const jw_str_t *collection,
                                  jw_cvec_t vectors, jw_dim_t dim,
                                  jw_size_t count, jw_vid_t *vids);

// 相似度搜索：在集合中查找 query 的最近 k 条，结果写入 results（至少 k 个元素）
// 返回实际命中数量；results 用 jw_search_result_t（见 1.4）
jw_size_t   jw_vecdb_search(jw_vecdb_t *db, const jw_str_t *collection,
                            jw_cvec_t query, jw_dim_t dim, jw_size_t k,
                            jw_search_result_t *results);
```

---

## 5. 集合级向量操作（需 `jw_collection_t*` 句柄）

句柄来自 `jw_vecdb_create_collection` / `jw_vecdb_get_collection`，或用底层
`jw_collection_create(arena, config)`（详见源码头文件）自行在内存竞技场中创建。

```c
// 插入（返回 vid）
jw_status_t jw_collection_insert(jw_collection_t *coll, jw_cvec_t vec, jw_vid_t *vid);
jw_status_t jw_collection_insert_batch(jw_collection_t *coll, jw_cvec_t vectors,
                                       jw_size_t count, jw_vid_t *vids);
jw_status_t jw_collection_insert_with_meta(jw_collection_t *coll, jw_cvec_t vec,
                                           const jw_meta_field_t *fields,
                                           jw_size_t field_count, jw_vid_t *vid);

// 更新（按 vid 覆盖）
jw_status_t jw_collection_upsert(jw_collection_t *coll, jw_vid_t vid, jw_cvec_t vec);

// 删除
jw_status_t jw_collection_delete(jw_collection_t *coll, jw_vid_t vid);
jw_status_t jw_collection_delete_batch(jw_collection_t *coll, const jw_vid_t *vids, jw_size_t count);
jw_status_t jw_collection_clear(jw_collection_t *coll);

// 读取
jw_status_t jw_collection_get(jw_collection_t *coll, jw_vid_t vid, jw_vec_t vec);

// 搜索（集合级，支持选项与扩展结果）
// 返回实际命中数量；results 用 jw_search_result_ex_t（见 1.5）
jw_size_t   jw_collection_search(jw_collection_t *coll, jw_cvec_t query,
                                 const jw_search_options_t *options,
                                 jw_search_result_ex_t *results);
```

---

## 6. 索引

```c
// 构建/重建索引（插入较多数据后调用可加速搜索）
jw_status_t jw_collection_build_index(jw_collection_t *coll);
jw_status_t jw_collection_rebuild_index(jw_collection_t *coll);

// 删除索引
jw_status_t jw_collection_drop_index(jw_collection_t *coll);

// 是否已建索引
jw_bool_t   jw_collection_has_index(const jw_collection_t *coll);

// 更新索引（增量）
jw_status_t jw_collection_update_index(jw_collection_t *coll);
```

---

## 7. 统计 / 迭代 / 持久化（集合级，节选）

```c
// 统计信息
jw_status_t jw_collection_get_stats(jw_collection_t *coll, jw_stats_t *stats);
const char *jw_collection_get_name(const jw_collection_t *coll);

// 迭代器：create → valid → next → get_id / get_vector → destroy
jw_iterator_t *jw_collection_create_iterator(jw_collection_t *coll, const jw_filter *filter);
jw_bool_t      jw_collection_iterator_valid(const jw_iterator_t *it);
jw_bool_t      jw_collection_iterator_next(jw_iterator_t *it);
jw_vid_t       jw_collection_iterator_get_id(const jw_iterator_t *it);
jw_cvec_t      jw_collection_iterator_get_vector(const jw_iterator_t *it);
void           jw_collection_destroy_iterator(jw_iterator_t *it);

// 持久化（单集合保存/加载到文件）
jw_status_t    jw_collection_save(jw_collection_t *coll, const char *filepath);
jw_collection_t *jw_collection_load(jw_arena_t *arena, const char *filepath);

// 底层句柄释放（非数据库级集合无需手动；竞技场创建的集合用此释放）
void jw_collection_destroy(jw_collection_t *coll);
```

> 数据库级操作的集合生命周期由数据库管理，关闭数据库即可；一般不需要手动 `destroy`。

---

## 8. 版本与错误处理

```c
// 版本信息
jw_str_t    jw_vecdb_version(void);     // 返回 "JinWo VecDB x.y.z"（结构体）
const char *jw_version_string(void);    // 同上，C 字符串形式（便于打印）
jw_int32_t  jw_version_number(void);    // 形如 0x000104 的整型版本号

// 错误信息
jw_status_t  jw_vecdb_get_last_error(const jw_vecdb_t *db);  // 取最近错误码
const char  *jw_vecdb_strerror(jw_status_t code);            // 错误码 → 文本
const char  *jw_vecdb_get_error_message(const jw_vecdb_t *db); // 最近错误详情
```

错误码（`jw_status_t` 为负表示失败）：

| 宏 | 值 | 含义 |
|---|---|---|
| `JW_SUCCESS` | `0` | 成功 |
| `JW_UNKNOWN_ERROR` | `-1` | 未知错误 |
| `JW_INVALID_PARAM` | `-2` | 无效参数 |
| `JW_OUT_OF_MEMORY` | `-3` | 内存不足 |
| `JW_NOT_FOUND` | `-4` | 未找到 |
| `JW_ALREADY_EXISTS` | `-5` | 已存在 |
| `JW_BUFFER_TOO_SMALL` | `-6` | 缓冲区太小 |
| `JW_NOT_SUPPORTED` | `-7` | 不支持 |
| `JW_PERMISSION_DENIED` | `-8` | 权限拒绝 |
| `JW_TIMEOUT` | `-9` | 超时 |
| `JW_BUSY` | `-10` | 忙碌 |
| `JW_EMPTY` | `-11` | 空对象 |
| `JW_TOO_BIG` | `-12` | 过大 |
| `JW_CANCELLED` | `-13` | 已取消 |
| `JW_INVALID_VECTOR` | `-100` | 向量维度不匹配/数据无效 |
| `JW_INDEX_CORRUPTED` | `-101` | 索引文件损坏 |
| `JW_COLLECTION_FULL` | `-102` | 集合已满 |
| `JW_INVALID_DIMENSION` | `-103` | 向量维度无效 |
| `JW_INDEX_NOT_READY` | `-104` | 索引未就绪 |
| `JW_VECTOR_EXISTS` | `-105` | 向量 ID 已存在 |
| `JW_VECTOR_NOT_FOUND` | `-106` | 向量不存在 |
| `JW_COLLECTION_EXISTS` | `-107` | 集合已存在 |
| `JW_COLLECTION_NOT_FOUND` | `-108` | 集合不存在 |
| `JW_INDEX_TYPE_MISMATCH` | `-109` | 索引类型不匹配 |
| `JW_QUANTIZATION_ERROR` | `-110` | 量化错误 |
| `JW_FILE_NOT_FOUND` | `-200` | 文件未找到 |
| `JW_FILE_CORRUPTED` | `-201` | 文件损坏 |
| `JW_DISK_FULL` | `-202` | 磁盘已满 |
| `JW_IO_ERROR` | `-203` | IO 错误 |
| `JW_FILE_LOCKED` | `-204` | 文件被锁定 |
| `JW_PATH_TOO_LONG` | `-205` | 路径过长 |
| `JW_READ_ONLY` | `-206` | 只读模式 |
| `JW_LOCK_TIMEOUT` | `-300` | 锁超时 |
| `JW_DEADLOCK` | `-301` | 死锁 |
| `JW_THREAD_ERROR` | `-302` | 线程错误 |
| `JW_MUTEX_ERROR` | `-303` | 互斥锁错误 |

---

## 9. 完整示例（C）

```c
#include <jw_vecdb.h>
#include <stdio.h>

int main(void) {
    jw_vecdb_t *db = NULL;
    // 1) 打开/创建数据库（路径用 jw_str 字面量，flags 传 CREATE）
    if (jw_vecdb_open(jw_str("my_vecs.jwv"), JW_VECDB_CREATE, &db) != JW_SUCCESS) {
        return 1;
    }

    // 2) 创建集合（维度 4），直接拿到集合句柄
    jw_collection_t *coll = NULL;
    if (jw_vecdb_create_collection(db, jw_str("docs"), 4, &coll) != JW_SUCCESS) {
        jw_vecdb_close(db);
        return 1;
    }

    // 3) 插入单条向量，返回 vid
    float vec[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    jw_vid_t id = 0;
    if (jw_collection_insert(coll, vec, &id) != JW_SUCCESS) {
        jw_vecdb_close(db);
        return 1;
    }
    printf("inserted id=%llu\n", (unsigned long long)id);

    // 4) 相似度搜索（Top-5），结果写入 jw_search_result_t 数组
    jw_search_result_t results[5];
    jw_size_t count = jw_vecdb_search(db, jw_str("docs"), vec, 4, 5, results);
    for (jw_size_t i = 0; i < count; i++) {
        printf("  id=%llu score=%.4f\n",
               (unsigned long long)results[i].id, results[i].score);
    }

    // 5) 关闭（自动落盘）
    jw_vecdb_close(db);
    return 0;
}
```

编译（库已 `cmake --install` 到系统目录，或指向本地 `build/`）：

```bash
gcc demo.c -o demo -ljinwo -lm -I/usr/local/include -L/usr/local/lib
# 本地未安装时：
gcc demo.c -o demo build/libjinwo.dylib -I include        # macOS
gcc demo.c -o demo build/libjinwo.so   -I include         # Linux
```

> ⚠️ C/C++ 本地开发库当前名为 `libjinwo`（已发布）。面向 C#/PHP 等尚未发布语言的
> 跨平台 FFI 底座统一命名为 `libjinwo_vecdb`，由 `.github/workflows/build_native_lib.yml`
> 跨平台编译发布，计划在 0.1.35 统一切换。

---

## 10. 关于旧文档的更正说明

旧版本本文档存在以下**与实际库不符**的内容，已更正或移除：

- `jw_vecdb_open(jw_vecdb_t** db, const char* path, bool create)` —— 实际为
  `jw_vecdb_open(const jw_str_t *path, jw_uint32_t flags, jw_vecdb_t **db)`（参数顺序、类型均不同）。
- `jw_collection_create(&col, db, name, dim)` —— 数据库级正确函数是
  `jw_vecdb_create_collection(db, name*, dim, &coll)`；底层 `jw_collection_create` 的签名为
  `jw_collection_create(jw_arena_t*, const jw_collection_config_t*)`（需先有竞技场）。
- `jw_collection_search(col, vec, k, &results, &count)` —— 实际为
  `jw_collection_search(coll, query, options*, results*)`（返回 `jw_size_t`）；数据库级便捷函数
  `jw_vecdb_search(db, name*, query, dim, k, results*)` 同样返回数量、就地写入结果数组。
- 搜索结果字段为 `.id` / `.score`（不是 `.distance`）。
- 版本接口：`jw_version()` 不存在，请用 `jw_version_string()` / `jw_vecdb_version()`。
- **已移除/不存在**的函数（旧文档误列）：`jw_collection_open`、`jw_collection_close`、
  `jw_collection_list`、`jw_collection_batch_insert`、`jw_collection_create_index`、
  `jw_distance_l2` / `jw_distance_cosine` / `jw_distance_dot`、`jw_vector_normalize` /
  `jw_vector_copy` / `jw_vector_add`、`jw_thread_pool_create` / `jw_thread_pool_submit` /
  `jw_thread_pool_destroy`，以及旧签名的 `jw_config_set` / `jw_config_get`。
  请以 `include/` 下头文件为准。
