/*
 * jw_types.h - JinWo VecDB 基础类型定义
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
 * 项目名称: JinWo VecDB (金幄向量数据库)
 * 公司名称: 北京金幄科技有限公司
 * 
 * 本文件定义了JinWo VecDB所需的所有基础类型，确保跨平台兼容性。
 * 支持平台: Linux, Android, iOS, macOS, Windows, WebAssembly (Emscripten)
 * 
 * 版本: 0.1.31
 * 作者: 灵活就业码农
 * 创建日期: 2026-04-23
 */

#ifndef JW_TYPES_H
#define JW_TYPES_H

/*
 * =============================================================================
 * 平台检测
 * =============================================================================
 */

/* 检测操作系统 */
#if defined(_WIN32) || defined(_WIN64)
    #define JW_WIN32           1
    #if defined(_WIN64)
        #define JW_WIN64       1
    #endif
#elif defined(__ANDROID__)
    #define JW_ANDROID         1
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define JW_IOS         1
    #elif TARGET_OS_MAC
        #define JW_MACOS       1
    #endif
#elif defined(__linux__)
    #define JW_LINUX           1
#elif defined(__EMSCRIPTEN__)
    #define JW_EMSCRIPTEN      1
#else
    #error "Unsupported platform"
#endif

/* 基础C库头文件 */
#include <stddef.h>      /* 定义 size_t, NULL 等 */
#include <stdint.h>      /* 定义标准整数类型 */
#include <string.h>      /* 定义 memcpy 等 */

/*
 * =============================================================================
 * 平台无关的文件句柄类型
 * =============================================================================
 *
 * 说明: 类似于PJSIP的pj_os_handle_t，平台无关的文件句柄抽象。
 * 在POSIX系统上对应int，在Windows上对应HANDLE。
 */
#if defined(JW_WIN32) || defined(JW_WIN64)
    typedef void *jw_os_handle_t;
#else
    typedef int jw_os_handle_t;
#endif

#define JW_INVALID_OS_HANDLE ((jw_os_handle_t)-1)

/*
 * =============================================================================
 * C++ 兼容性
 * =============================================================================
 */
#ifdef __cplusplus
    #define JW_BEGIN_DECL      extern "C" {
    #define JW_END_DECL        }
#else
    #define JW_BEGIN_DECL
    #define JW_END_DECL
#endif

/*
 * =============================================================================
 * 导出符号定义
 * =============================================================================
 */
#if defined(_WIN32) || defined(_WIN64)
    // ============================================================
    // 2026-05-21: 修复 Windows STATIC 库编译错误
    // 原因: STATIC 库不应该使用 dllexport/dllimport
    // 备份:
    // #define JW_API __declspec(dllimport)
    // ============================================================
    #ifdef JW_STATIC
        #define JW_API
    #elif defined(JW_EXPORTS)
        #define JW_API          __declspec(dllexport)
    #else
        #define JW_API          __declspec(dllimport)
    #endif
    #define JW_CALL             __cdecl
#else
    #define JW_API              __attribute__((visibility("default")))
    #define JW_CALL
#endif

JW_BEGIN_DECL

/*
 * =============================================================================
 * 版本信息
 * =============================================================================
 */

#define JW_VERSION_MAJOR       0
#define JW_VERSION_MINOR       1
#define JW_VERSION_PATCH       31
#define JW_VERSION_STRING      "0.1.31"
#define JW_VERSION_NUM         (JW_VERSION_MAJOR * 10000 + \
                                 JW_VERSION_MINOR * 100 + \
                                 JW_VERSION_PATCH)

/*
 * =============================================================================
 * 基础整数类型
 * =============================================================================
 * 
 * 说明: 为确保跨平台一致性，显式定义所有整数类型的位数。
 * 确保在所有平台上都有相同的位宽。
 */

/* 8位整数 */
typedef signed char          jw_int8_t;
typedef unsigned char        jw_uint8_t;

/* 16位整数 */
typedef short                jw_int16_t;
typedef unsigned short       jw_uint16_t;

/* 32位整数 */
typedef int                  jw_int32_t;
typedef unsigned int         jw_uint32_t;

/* 64位整数 */
#if defined(_WIN32) && !defined(__MINGW32__)
    typedef __int64          jw_int64_t;
    typedef unsigned __int64 jw_uint64_t;
#else
    typedef long long        jw_int64_t;
    typedef unsigned long long jw_uint64_t;
#endif

/* 大整数类型 - 用于表示大小和偏移量 */
typedef size_t               jw_size_t;
typedef long                 jw_ssize_t;    /* 有符号版本 */
typedef jw_ssize_t           jw_off_t;      /* 文件偏移量 */

/*
 * =============================================================================
 * 浮点类型 (向量数据库核心)
 * =============================================================================
 * 
 * 说明: 向量数据库大量使用浮点运算，明确定义浮点类型至关重要。
 * 
 * jw_float32_t: 单精度浮点 (32位)，适用于存储向量数据
 *               - 移动端推荐使用，内存占用小
 *               - 精度足够大多数AI应用
 * 
 * jw_float64_t: 双精度浮点 (64位)，适用于高精度计算
 *               - 服务器端可使用
 *               - 需要高精度距离计算时使用
 */
typedef float                jw_float32_t;
typedef double               jw_float64_t;

/*
 * =============================================================================
 * 浮点数常量
 * =============================================================================
 */

#define JW_FLT_MAX           3.402823466e+38f
#define JW_FLT_EPSILON       1.192092896e-07f
#define JW_DBL_MAX           1.797693134862315708e+308
#define JW_DBL_EPSILON       2.2204460492503131e-16

/*
 * =============================================================================
 * 布尔类型
 * =============================================================================
 */
typedef int                  jw_bool_t;
#define JW_TRUE              1
#define JW_FALSE             0

/*
 * =============================================================================
 * 状态码定义
 * =============================================================================
 * 
 * 说明: 所有API函数返回jw_status_t类型的状态码。
 * 成功返回 JW_SUCCESS (0)，失败返回负的错误码。
 * 正数返回值通常用于表示实际数量或大小。
 * 
 */
typedef int                  jw_status_t;

/* 成功状态 */
#define JW_SUCCESS           0

/* 通用错误码 (1-99) */
#define JW_UNKNOWN_ERROR     -1      /* 未知错误 */
#define JW_INVALID_PARAM     -2      /* 无效参数 */
#define JW_OUT_OF_MEMORY     -3      /* 内存不足 */
#define JW_NOT_FOUND         -4      /* 未找到 */
#define JW_ALREADY_EXISTS    -5      /* 已存在 */
#define JW_BUFFER_TOO_SMALL  -6      /* 缓冲区太小 */
#define JW_NOT_SUPPORTED     -7      /* 不支持 */
#define JW_PERMISSION_DENIED -8      /* 权限拒绝 */
#define JW_TIMEOUT           -9      /* 超时 */
#define JW_BUSY              -10     /* 忙碌 */
#define JW_EMPTY             -11     /* 空对象 */
#define JW_TOO_BIG           -12     /* 过大 */
#define JW_CANCELLED         -13     /* 已取消 */

/* 向量数据库专用错误码 (100-199) */
#define JW_INVALID_VECTOR    -100    /* 向量维度不匹配或数据无效 */
#define JW_INDEX_CORRUPTED   -101    /* 索引文件损坏 */
#define JW_COLLECTION_FULL   -102    /* 集合已满 */
#define JW_INVALID_DIMENSION -103    /* 向量维度无效 */
#define JW_INDEX_NOT_READY   -104    /* 索引未就绪 */
#define JW_VECTOR_EXISTS     -105    /* 向量ID已存在 */
#define JW_VECTOR_NOT_FOUND  -106    /* 向量不存在 */
#define JW_COLLECTION_EXISTS -107    /* 集合已存在 */
#define JW_COLLECTION_NOT_FOUND -108 /* 集合不存在 */
#define JW_INDEX_TYPE_MISMATCH -109  /* 索引类型不匹配 */
#define JW_QUANTIZATION_ERROR -110   /* 量化错误 */

/* 存储相关错误码 (200-299) */
#define JW_FILE_NOT_FOUND    -200    /* 文件未找到 */
#define JW_FILE_CORRUPTED    -201    /* 文件损坏 */
#define JW_DISK_FULL         -202    /* 磁盘已满 */
#define JW_IO_ERROR          -203    /* IO错误 */
#define JW_FILE_LOCKED       -204    /* 文件被锁定 */
#define JW_PATH_TOO_LONG     -205    /* 路径过长 */
#define JW_READ_ONLY         -206    /* 只读模式 */

/* 并发相关错误码 (300-399) */
#define JW_LOCK_TIMEOUT      -300    /* 锁超时 */
#define JW_DEADLOCK          -301    /* 死锁 */
#define JW_THREAD_ERROR      -302    /* 线程错误 */
#define JW_MUTEX_ERROR       -303    /* 互斥锁错误 */

/*
 * =============================================================================
 * 向量数据库专用类型
 * =============================================================================
 */

/* 向量维度类型 - 表示向量的维度 */
typedef jw_uint32_t          jw_dim_t;

/* 向量ID类型 - 唯一标识一个向量 (64位，支持海量数据) */
typedef jw_uint64_t          jw_vec_id_t;

/* 集合ID类型 - 唯一标识一个向量集合 */
typedef jw_uint32_t          jw_col_id_t;

/* 相似度分数类型 (通常范围: 0.0 - 1.0 或距离值) */
typedef jw_float32_t         jw_score_t;

/* 向量数据指针 */
typedef jw_float32_t*        jw_vec_t;
typedef const jw_float32_t*  jw_cvec_t;

/* 量化类型枚举 */
#ifndef JW_QUANT_TYPE_DEFINED
typedef enum jw_quant_type {
    JW_QUANT_NONE = 0,          /* 不量化 */
    JW_QUANT_UINT8,             /* 8位无符号整数量化 */
    JW_QUANT_INT8,              /* 8位有符号整数量化 */
    JW_QUANT_FLOAT16,           /* 16位浮点量化 */
    JW_QUANT_PQ                 /* 乘积量化 */
} jw_quant_type_t;
#define JW_QUANT_TYPE_DEFINED
#endif

/* 索引类型枚举 */
typedef enum jw_index_type {
    JW_INDEX_NONE = -1,         /* 无索引 */
    JW_INDEX_FLAT = 0,          /* 暴力搜索，精确但慢，适合小数据集 */
    JW_INDEX_IVF,               /* 倒排索引，快速近似，适合中等规模 */
    JW_INDEX_HNSW,              /* 图索引，高召回率，适合大规模 */
    JW_INDEX_IVF_PQ,            /* IVF + 乘积量化，低内存占用 */
    JW_INDEX_HNSW_PQ,           /* HNSW + 乘积量化，内存效率高 */
    JW_INDEX_IVF_SQ,            /* IVF + 标量量化，低内存占用 */
    JW_INDEX_HNSW_SQ,           /* HNSW + 标量量化，内存效率高 */
    JW_INDEX_AUTO                /* 自动选择最优索引类型 */
} jw_index_type_t;

/* 距离度量类型 */
typedef enum jw_metric {
    JW_METRIC_L2 = 0,           /* 欧氏距离 (L2) */
    JW_METRIC_IP,               /* 内积 (Inner Product) */
    JW_METRIC_COSINE,           /* 余弦相似度 */
    JW_METRIC_MANHATTAN,        /* 曼哈顿距离 (L1) */
    JW_METRIC_HAMMING           /* 汉明距离 (二进制向量) */
} jw_metric_t;

/* 存储模式 */
typedef enum jw_storage_mode {
    JW_STORAGE_MEMORY = 0,      /* 纯内存模式，速度快但不持久 */
    JW_STORAGE_DISK,            /* 磁盘持久化，类似SQLite */
    JW_STORAGE_HYBRID           /* 混合模式，热数据在内存 */
} jw_storage_mode_t;

/*
 * =============================================================================
 * 字符串类型
 * =============================================================================
 * 
 * 说明: 字符串不带'\0'结尾。
 * 好处: 
 *   1. 避免频繁的内存拷贝
 *   2. 可以直接指向大缓冲区的一部分
 *   3. 提高性能，减少内存分配
 */
typedef struct jw_str_t {
    char      *ptr;             /* 字符串指针 */
    jw_size_t slen;             /* 字符串长度 (不包含'\0') */
} jw_str_t;

/* 字符串初始化宏 */
#define JW_STR_INIT(ptr, len)   {ptr, len}

/* 字符串字面量宏 */
#define jw_str(s)              ((jw_str_t){(char *)(s), sizeof(s) - 1})
#define JW_STR_NULL             {NULL, 0}
#define JW_STR_SET(str, s)      do { (str).ptr = (char *)(s); (str).slen = strlen(s); } while(0)

/*
 * =============================================================================
 * 时间类型
 * =============================================================================
 */

/* 时间值结构 (秒 + 毫秒) */
typedef struct jw_time_val {
    jw_int32_t sec;             /* 秒 */
    jw_int32_t msec;            /* 毫秒 (0-999) */
} jw_time_val_t;

/* 高精度时间戳 (64位，用于性能测量) */
typedef union jw_timestamp {
    jw_uint64_t u64;            /* 64位值 */
    struct {
        jw_uint32_t lo;         /* 低32位 */
        jw_uint32_t hi;         /* 高32位 */
    } u32;
} jw_timestamp_t;

/* 时间类型 (64位，用于时间戳) */
typedef jw_int64_t jw_time_t;

/*
 * =============================================================================
 * 句柄类型 (不透明指针)
 * =============================================================================
 * 
 * 说明: 对外暴露的结构体使用不透明指针，隐藏实现细节。
 * 好处: 
 *   1. ABI兼容性更好
 *   2. 用户代码不依赖具体实现
 *   3. 可以自由修改内部结构
 */

/* 向量数据库实例 */
typedef struct jw_vecdb_t       jw_vecdb_t;

/* 向量集合 - 在 jw_collection.h 中定义 */

/* 向量索引 - 在 jw_index.h 中定义 */

/* 锁对象 - 抽象锁接口 */
typedef struct jw_lock_t        jw_lock_t;

/* 线程句柄 */
typedef struct jw_thread_t      jw_thread_t;

/* 原子变量 */
typedef struct jw_atomic_t      jw_atomic_t;

/* 内存分配器 */
typedef struct jw_allocator_t {
    void* (*alloc)(jw_size_t size, void* user_data);
    void* (*realloc)(void* ptr, jw_size_t size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* user_data;
} jw_allocator_t;

/* 获取/设置内存分配器 */
JW_API jw_allocator_t *jw_get_allocator(void);
JW_API jw_status_t jw_set_allocator(const jw_allocator_t *allocator);

/* 内存分配函数 */
JW_API void *jw_malloc(jw_size_t size);
JW_API void *jw_realloc(void *ptr, jw_size_t size);
JW_API void jw_free(void *ptr);
JW_API void *jw_calloc(jw_size_t count, jw_size_t size);
JW_API void *jw_alloc(jw_size_t size);

/* 时间函数 */
JW_API void jw_sleep(jw_uint32_t ms);

/* 配置对象 */

/* 过滤器 - 前向声明 (使用struct标签避免typedef重定义) */

/* 迭代器 */
typedef struct jw_iterator_t    jw_iterator_t;

/* 迭代器结构 */
struct jw_iterator_t {
    void *collection;
    jw_size_t current;
    jw_size_t total;
    struct jw_filter *filter;
    jw_bool_t valid;
    void *arena;  /* 内存池指针 */
};

/* 搜索结果 */
typedef struct jw_search_result jw_search_result_t;

/*
 * =============================================================================
 * 配置结构体
 * =============================================================================
 */

/* 向量集合配置 */
typedef struct jw_collection_config {
    jw_str_t        name;           /* 集合名称 */
    jw_dim_t        dimension;      /* 向量维度 */
    jw_metric_t     metric;         /* 距离度量 */
    jw_index_type_t index_type;     /* 索引类型 */
    jw_size_t       capacity;       /* 初始容量 */
    jw_bool_t       auto_resize;    /* 自动扩容 */
    jw_quant_type_t quant;          /* 量化类型 */
    struct {
        jw_uint32_t nsub;           /* 子空间数量 */
        jw_uint32_t nbits;          /* 每个子空间的位数 */
    } pq;                           /* PQ量化参数 */
} jw_collection_config_t;

/* 搜索结果结构 */
struct jw_search_result {
    jw_vec_id_t    id;             /* 向量ID */
    jw_score_t     score;          /* 相似度分数 */
    jw_cvec_t      vector;         /* 向量数据 (可选) */
    jw_size_t      dimension;      /* 向量维度 */
    void          *metadata;       /* 元数据指针 (可选) */
    jw_size_t      metadata_size;  /* 元数据大小 */
};

/*
 * =============================================================================
 * 回调函数类型
 * =============================================================================
 */

/* 搜索结果回调 */
typedef void (*jw_search_callback_t)(
    jw_vec_id_t id,             /* 向量ID */
    jw_score_t score,           /* 相似度分数 */
    jw_cvec_t vector,           /* 向量数据 */
    void *user_data             /* 用户数据 */
);

/* 错误回调 */
typedef void (*jw_error_callback_t)(
    jw_status_t status,         /* 错误码 */
    const char *message,        /* 错误信息 */
    void *user_data             /* 用户数据 */
);

/* 进度回调 */
typedef void (*jw_progress_callback_t)(
    jw_size_t current,          /* 当前进度 */
    jw_size_t total,            /* 总数 */
    const char *message,        /* 进度信息 */
    void *user_data             /* 用户数据 */
);

/* 迭代回调 */
typedef jw_bool_t (*jw_iterator_callback_t)(
    jw_vec_id_t id,             /* 向量ID */
    jw_cvec_t vector,           /* 向量数据 */
    jw_size_t dimension,        /* 维度 */
    void *user_data             /* 用户数据 */
    /* 返回 JW_TRUE 继续，JW_FALSE 停止迭代 */
);

/*
 * =============================================================================
 * 常用宏定义
 * =============================================================================
 */

/* 数组大小计算 */
#define JW_ARRAY_SIZE(a)        (sizeof(a)/sizeof(a[0]))

/* 对象名称最大长度 */
#define JW_MAX_OBJ_NAME         32

/* 集合名称最大长度 */
#define JW_MAX_COLLECTION_NAME  64

/* 最大路径长度 */
#define JW_MAX_PATH             260

/* 向量最大维度 */
#define JW_MAX_VECTOR_DIM       4096

/* 最大集合数量 */
#define JW_MAX_COLLECTIONS      256

/* 默认向量维度 */
#define JW_DEFAULT_VECTOR_DIM   128

/* 默认搜索返回数量 */
#define JW_DEFAULT_TOP_K        10

/* 默认内存池大小 (4MB) */
#define JW_DEFAULT_POOL_SIZE    (4 * 1024 * 1024)

/* 默认缓存大小 (16MB) */
#define JW_DEFAULT_CACHE_SIZE   (16 * 1024 * 1024)

/* 字节对齐宏 */
#define JW_ALIGN(x, a)          (((x) + (a) - 1) & ~((a) - 1))
#define JW_ALIGN_4(x)           JW_ALIGN(x, 4)
#define JW_ALIGN_8(x)           JW_ALIGN(x, 8)
#define JW_ALIGN_16(x)          JW_ALIGN(x, 16)
#define JW_ALIGN_64(x)          JW_ALIGN(x, 64)

/* 最小/最大值 */
#define JW_MIN(a, b)            ((a) < (b) ? (a) : (b))
#define JW_MAX(a, b)            ((a) > (b) ? (a) : (b))

/* 绝对值 */
#define JW_ABS(x)               ((x) < 0 ? -(x) : (x))

/* 交换值 */
#define JW_SWAP(a, b, type)     do { type tmp = a; a = b; b = tmp; } while(0)

/* 安全释放宏 */
#define JW_SAFE_FREE(p)         do { if(p) { free(p); (p) = NULL; } } while(0)

/*
 * =============================================================================
 * 平台相关宏定义
 * =============================================================================
 */

/* 内联函数定义 */
#ifndef JW_INLINE
    #if defined(_MSC_VER)
        #define JW_INLINE       __inline
    #else
        #define JW_INLINE       static inline
    #endif
#endif

/* 强制内联 */
#if defined(_MSC_VER)
    #define JW_FORCE_INLINE     __forceinline
#elif defined(__GNUC__)
    #define JW_FORCE_INLINE     __attribute__((always_inline)) inline
#else
    #define JW_FORCE_INLINE     inline
#endif

/* 空指针定义 */
#ifndef NULL
    #define NULL                ((void*)0)
#endif

/* 未使用参数 */
#define JW_UNUSED(x)            ((void)(x))

/* 分支预测提示 */
#if defined(__GNUC__) || defined(__clang__)
    #define JW_LIKELY(x)        __builtin_expect(!!(x), 1)
    #define JW_UNLIKELY(x)      __builtin_expect(!!(x), 0)
#else
    #define JW_LIKELY(x)        (x)
    #define JW_UNLIKELY(x)      (x)
#endif

/*
 * =============================================================================
 * 字节序处理
 * =============================================================================
 */

/* 检测字节序 */
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define JW_LITTLE_ENDIAN    1
        #define JW_BIG_ENDIAN       0
    #else
        #define JW_LITTLE_ENDIAN    0
        #define JW_BIG_ENDIAN       1
    #endif
#elif defined(_WIN32)
    #define JW_LITTLE_ENDIAN        1
    #define JW_BIG_ENDIAN           0
#else
    /* 默认假设小端序 (x86, ARM) */
    #define JW_LITTLE_ENDIAN        1
    #define JW_BIG_ENDIAN           0
#endif

/* 字节序转换函数 */
JW_INLINE jw_uint16_t jw_swap16(jw_uint16_t val) {
    return (jw_uint16_t)((val >> 8) | (val << 8));
}

JW_INLINE jw_uint32_t jw_swap32(jw_uint32_t val) {
    return ((val >> 24) & 0x000000FFU) |
           ((val >>  8) & 0x0000FF00U) |
           ((val <<  8) & 0x00FF0000U) |
           ((val << 24) & 0xFF000000U);
}

JW_INLINE jw_uint64_t jw_swap64(jw_uint64_t val) {
    return ((val >> 56) & 0x00000000000000FFULL) |
           ((val >> 40) & 0x000000000000FF00ULL) |
           ((val >> 24) & 0x0000000000FF0000ULL) |
           ((val >>  8) & 0x00000000FF000000ULL) |
           ((val <<  8) & 0x000000FF00000000ULL) |
           ((val << 24) & 0x0000FF0000000000ULL) |
           ((val << 40) & 0x00FF000000000000ULL) |
           ((val << 56) & 0xFF00000000000000ULL);
}

/* 主机字节序转网络字节序 (大端) */
#if JW_LITTLE_ENDIAN
    #define jw_hton16(x)        jw_swap16(x)
    #define jw_hton32(x)        jw_swap32(x)
    #define jw_hton64(x)        jw_swap64(x)
    #define jw_ntoh16(x)        jw_swap16(x)
    #define jw_ntoh32(x)        jw_swap32(x)
    #define jw_ntoh64(x)        jw_swap64(x)
#else
    #define jw_hton16(x)        (x)
    #define jw_hton32(x)        (x)
    #define jw_hton64(x)        (x)
    #define jw_ntoh16(x)        (x)
    #define jw_ntoh32(x)        (x)
    #define jw_ntoh64(x)        (x)
#endif

/* 主机字节序转小端序 (文件存储统一用小端) */
#if JW_LITTLE_ENDIAN
    #define jw_htole16(x)       (x)
    #define jw_htole32(x)       (x)
    #define jw_htole64(x)       (x)
    #define jw_letoh16(x)       (x)
    #define jw_letoh32(x)       (x)
    #define jw_letoh64(x)       (x)
#else
    #define jw_htole16(x)       jw_swap16(x)
    #define jw_htole32(x)       jw_swap32(x)
    #define jw_htole64(x)       jw_swap64(x)
    #define jw_letoh16(x)       jw_swap16(x)
    #define jw_letoh32(x)       jw_swap32(x)
    #define jw_letoh64(x)       jw_swap64(x)
#endif

/* 浮点数字节序转换 */
JW_INLINE jw_float32_t jw_swapf32(jw_float32_t val) {
    jw_uint32_t tmp;
    memcpy(&tmp, &val, sizeof(tmp));
    tmp = jw_swap32(tmp);
    memcpy(&val, &tmp, sizeof(tmp));
    return val;
}

JW_INLINE jw_float64_t jw_swapf64(jw_float64_t val) {
    jw_uint64_t tmp;
    memcpy(&tmp, &val, sizeof(tmp));
    tmp = jw_swap64(tmp);
    memcpy(&val, &tmp, sizeof(tmp));
    return val;
}

/* 浮点数小端序转换 */
#if JW_LITTLE_ENDIAN
    #define jw_htolef32(x)      (x)
    #define jw_htolef64(x)      (x)
    #define jw_letohf32(x)      (x)
    #define jw_letohf64(x)      (x)
#else
    #define jw_htolef32(x)      jw_swapf32(x)
    #define jw_htolef64(x)      jw_swapf64(x)
    #define jw_letohf32(x)      jw_swapf32(x)
    #define jw_letohf64(x)      jw_swapf64(x)
#endif

/*
 * =============================================================================
 * 调试和日志宏
 * =============================================================================
 */

/* 日志级别 */
/* 日志回调类型在 jw_log.h 中定义 */

/* 调试打印宏 */
#ifdef JW_DEBUG
    #include <stdio.h>
    #define JW_DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[JW_DEBUG] %s:%d %s(): " fmt "\n", \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
    #define JW_DEBUG_PRINT(fmt, ...) ((void)0)
#endif

/* 断言宏 */
#ifdef JW_DEBUG
    #include <assert.h>
    #define JW_ASSERT(cond)     assert(cond)
    #define JW_ASSERT_MSG(cond, msg) \
        do { if (!(cond)) { fprintf(stderr, "Assertion failed: %s\n", msg); assert(cond); } } while(0)
#else
    #define JW_ASSERT(cond)     ((void)0)
    #define JW_ASSERT_MSG(cond, msg) ((void)0)
#endif

/* 错误检查宏 */
#define JW_CHECK(expr) \
    do { \
        jw_status_t __status = (expr); \
        if (JW_UNLIKELY(__status != JW_SUCCESS)) { \
            return __status; \
        } \
    } while(0)

#define JW_CHECK_NULL(ptr) \
    do { \
        if (JW_UNLIKELY((ptr) == NULL)) { \
            return JW_INVALID_PARAM; \
        } \
    } while(0)

/*
 * =============================================================================
 * 辅助函数声明
 * =============================================================================
 */

/* 获取版本信息 */
JW_API const char* jw_version_string(void);
JW_API jw_int32_t jw_version_number(void);

/* 获取错误描述 */
JW_API const char* jw_strerror(jw_status_t status);

/* 获取当前时间（秒） */
JW_API jw_uint64_t jw_time_now(void);

/* 获取平台信息 */
JW_API const char* jw_platform_name(void);
JW_API void jw_set_simd_enabled(jw_bool_t enable);
JW_API jw_bool_t jw_is_simd_available(void);

/*
 * =============================================================================
 * 内联辅助函数
 * =============================================================================
 */

/* 检查是否为2的幂 */
JW_INLINE jw_bool_t jw_is_power_of_two(jw_size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

/* 向上对齐到2的幂 */
JW_INLINE jw_size_t jw_align_up(jw_size_t x, jw_size_t alignment) {
    if (!jw_is_power_of_two(alignment)) {
        return x; /* 非法对齐值 */
    }
    return (x + alignment - 1) & ~(alignment - 1);
}

/* 比较两个浮点数是否近似相等 */
JW_INLINE jw_bool_t jw_float_equal(jw_float32_t a, jw_float32_t b, jw_float32_t epsilon) {
    return (a > b ? a - b : b - a) < epsilon;
}

/* 限制值在范围内 */
JW_INLINE jw_float32_t jw_clamp(jw_float32_t value, jw_float32_t min_val, jw_float32_t max_val) {
    return JW_MIN(JW_MAX(value, min_val), max_val);
}

JW_END_DECL

/* 基础C库头文件 - 放在最后，避免循环包含 */
#include "jw_stdarg.h"   /* 定义 va_list 等 */
#include "jw_stdlib.h"   /* 定义内存分配函数 */

#endif /* JW_TYPES_H */

/*
 * =============================================================================
 * 使用示例
 * =============================================================================
 * 
 * // 基本类型定义
 * jw_float32_t vector[128];
 * jw_dim_t dim = 128;
 * jw_vec_id_t id = 1;
 * 
 * // 字符串使用
 * jw_str_t name = JW_STR_INIT("my_collection", 13);
 * 
 * // 状态码检查
 * jw_status_t status = jw_vecdb_init(NULL, &db);
 * if (status != JW_SUCCESS) {
 *     printf("Error: %s\n", jw_strerror(status));
 *     return -1;
 * }
 * 
 * // 配置集合
 * jw_collection_config_t config = {
 *     .name = JW_STR_INIT("documents", 9),
 *     .dimension = 768,
 *     .metric = JW_METRIC_COSINE,
 *     .index_type = JW_INDEX_HNSW,
 *     .capacity = 100000,
 *     .auto_resize = JW_TRUE
 * };
 * 
 * // 平台判断
 * #ifdef JW_ANDROID
 *     config.storage_mode = JW_STORAGE_MEMORY; // 移动端优先内存模式
 * #endif
 * 
 * // 版本检查
 * printf("JinWo VecDB version: %s\n", jw_version_string());
 * 
 * // 索引类型选择
 * jw_index_type_t idx = JW_INDEX_HNSW;
 * jw_metric_t metric = JW_METRIC_COSINE;
 */
