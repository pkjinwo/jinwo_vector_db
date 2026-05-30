/*
 * jw_lock.h - JinWo VecDB 锁机制
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
 * 锁机制说明:
 * 
 * 本文件提供跨平台的同步原语:
 *   1. 互斥锁 (Mutex) - 独占访问
 *   2. 读写锁 (RWLock) - 读多写少场景
 *   3. 自旋锁 (Spinlock) - 短时间持有
 *   4. 原子操作 (Atomic) - 无锁编程
 *   5. 条件变量 (Condition) - 线程同步
 * 
 * 设计原则:
 *   - 统一抽象接口，底层平台适配
 *   - 支持超时等待
 *   - 支持递归锁
 *   - 调试模式支持死锁检测
 * 
 * 版本: 0.1.20
 * 作者: 灵活就业码农
 */

#ifndef JW_LOCK_H
#define JW_LOCK_H

#include "jw_types.h"
#include "jw_arena.h"

/* 前向声明 */


#ifdef JW_WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

JW_BEGIN_DECL

/* 互斥锁类型定义 */
#ifdef JW_WIN32
typedef CRITICAL_SECTION jw_mutex_t;
#else
typedef pthread_mutex_t jw_mutex_t;
#endif

/* 读写锁类型定义 */
#ifdef JW_WIN32
typedef struct {
    SRWLOCK rwlock;
} jw_rwlock_t;
#else
typedef pthread_rwlock_t jw_rwlock_t;
#endif

/* 条件变量类型定义 */
#ifdef JW_WIN32
typedef CONDITION_VARIABLE jw_cond_t;
#else
typedef pthread_cond_t jw_cond_t;
#endif

/* 信号量类型定义 */
#ifdef JW_WIN32
typedef HANDLE jw_sem_t;
#else
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    jw_uint32_t count;
    jw_uint32_t max;
} jw_sem_t;
#endif

/*
 * =============================================================================
 * 锁类型定义
 * =============================================================================
 */

/* 锁类型枚举 */
typedef enum jw_lock_type {
    JW_LOCK_MUTEX = 0,          /* 普通互斥锁 */
    JW_LOCK_MUTEX_RECURSIVE,    /* 递归互斥锁 */
    JW_LOCK_RWLOCK,             /* 读写锁 */
    JW_LOCK_SPINLOCK            /* 自旋锁 */
} jw_lock_type_t;

/* 锁属性 */
typedef struct jw_lock_attr {
    jw_lock_type_t  type;           /* 锁类型 */
    jw_bool_t       process_shared; /* 进程间共享 */
    jw_str_t        name;           /* 锁名称 (调试用) */
} jw_lock_attr_t;

/*
 * =============================================================================
 * 互斥锁 (Mutex)
 * =============================================================================
 */

/**
 * 创建互斥锁
 * 
 * @param arena 内存池 (NULL则使用malloc)
 * @param attr 锁属性 (NULL使用默认)
 * @param mutex 输出互斥锁指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_mutex_create(jw_arena_t *arena,
                                    const jw_lock_attr_t *attr,
                                    jw_mutex_t **mutex);

/**
 * 销毁互斥锁
 */
JW_API void jw_mutex_destroy(jw_mutex_t *mutex);

/**
 * 加锁 (阻塞)
 * 
 * @param mutex 互斥锁
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_mutex_lock(jw_mutex_t *mutex);

/**
 * 尝试加锁 (非阻塞)
 * 
 * @param mutex 互斥锁
 * @return JW_SUCCESS 成功获取锁
 *         JW_BUSY 锁已被占用
 */
JW_API jw_status_t jw_mutex_trylock(jw_mutex_t *mutex);

/**
 * 带超时的加锁
 * 
 * @param mutex   互斥锁
 * @param timeout 超时时间 (毫秒)
 * @return JW_SUCCESS 成功
 *         JW_TIMEOUT 超时
 */
JW_API jw_status_t jw_mutex_timedlock(jw_mutex_t *mutex, 
                                       jw_uint32_t timeout_ms);

/**
 * 解锁
 */
JW_API jw_status_t jw_mutex_unlock(jw_mutex_t *mutex);

/**
 * 获取互斥锁类型
 */
JW_API jw_lock_type_t jw_mutex_get_type(const jw_mutex_t *mutex);

/*
 * =============================================================================
 * 读写锁 (RWLock)
 * =============================================================================
 * 
 * 特点:
 *   - 多个读者可以同时持有锁
 *   - 写者独占访问
 *   - 适合读多写少的场景
 */

/**
 * 创建读写锁
 */
JW_API jw_status_t jw_rwlock_create(jw_arena_t *arena,
                                     const jw_lock_attr_t *attr,
                                     jw_rwlock_t **rwlock);

/**
 * 销毁读写锁
 */
JW_API void jw_rwlock_destroy(jw_rwlock_t *rwlock);

/**
 * 获取读锁 (共享锁)
 * 
 * 多个线程可以同时持有读锁
 */
JW_API jw_status_t jw_rwlock_rdlock(jw_rwlock_t *rwlock);

/**
 * 尝试获取读锁 (非阻塞)
 */
JW_API jw_status_t jw_rwlock_tryrdlock(jw_rwlock_t *rwlock);

/**
 * 带超时获取读锁
 */
JW_API jw_status_t jw_rwlock_timedrdlock(jw_rwlock_t *rwlock,
                                          jw_uint32_t timeout_ms);

/**
 * 获取写锁 (排他锁)
 * 
 * 只有一个线程可以持有写锁
 */
JW_API jw_status_t jw_rwlock_wrlock(jw_rwlock_t *rwlock);

/**
 * 尝试获取写锁 (非阻塞)
 */
JW_API jw_status_t jw_rwlock_trywrlock(jw_rwlock_t *rwlock);

/**
 * 带超时获取写锁
 */
JW_API jw_status_t jw_rwlock_timedwrlock(jw_rwlock_t *rwlock,
                                          jw_uint32_t timeout_ms);

/**
 * 释放读锁
 */
JW_API jw_status_t jw_rwlock_rdunlock(jw_rwlock_t *rwlock);

/**
 * 释放写锁
 */
JW_API jw_status_t jw_rwlock_wrunlock(jw_rwlock_t *rwlock);

/**
 * 通用解锁 (根据当前锁类型自动选择)
 */
JW_API jw_status_t jw_rwlock_unlock(jw_rwlock_t *rwlock);

/*
 * =============================================================================
 * 自旋锁 (Spinlock)
 * =============================================================================
 * 
 * 特点:
 *   - 忙等待，不释放CPU
 *   - 适合短时间持有 (微秒级)
 *   - 在SMP系统上效率更高
 */

/**
 * 创建自旋锁
 */
JW_API jw_status_t jw_spinlock_create(jw_arena_t *arena,
                                       const jw_lock_attr_t *attr,
                                       jw_lock_t **lock);

/**
 * 销毁自旋锁
 */
JW_API void jw_spinlock_destroy(jw_lock_t *lock);

/**
 * 加锁 (忙等待)
 */
JW_API jw_status_t jw_spinlock_lock(jw_lock_t *lock);

/**
 * 尝试加锁
 */
JW_API jw_status_t jw_spinlock_trylock(jw_lock_t *lock);

/**
 * 解锁
 */
JW_API jw_status_t jw_spinlock_unlock(jw_lock_t *lock);

/*
 * =============================================================================
 * 统一锁接口 (抽象层)
 * =============================================================================
 */

/**
 * 创建锁 (根据类型自动选择)
 * 
 * @param arena 内存池
 * @param attr 锁属性
 * @param lock 输出锁指针
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_lock_create(jw_arena_t *arena,
                                   const jw_lock_attr_t *attr,
                                   jw_lock_t **lock);

/**
 * 销毁锁
 */
JW_API void jw_lock_destroy(jw_lock_t *lock);

/**
 * 加锁
 */
JW_API jw_status_t jw_lock_acquire(jw_lock_t *lock);

/**
 * 尝试加锁
 */
JW_API jw_status_t jw_lock_try_acquire(jw_lock_t *lock);

/**
 * 带超时加锁
 */
JW_API jw_status_t jw_lock_acquire_timeout(jw_lock_t *lock, 
                                            jw_uint32_t timeout_ms);

/**
 * 解锁
 */
JW_API jw_status_t jw_lock_release(jw_lock_t *lock);

/**
 * 获取锁类型
 */
JW_API jw_lock_type_t jw_lock_get_type(const jw_lock_t *lock);

/*
 * =============================================================================
 * 条件变量 (Condition Variable)
 * =============================================================================
 */

/**
 * 创建条件变量
 */
JW_API jw_status_t jw_cond_create(jw_arena_t *arena, jw_cond_t **cond);

/**
 * 销毁条件变量
 */
JW_API void jw_cond_destroy(jw_cond_t *cond);

/**
 * 等待条件变量
 * 
 * @param cond 条件变量
 * @param mutex 关联的互斥锁
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_cond_wait(jw_cond_t *cond, jw_mutex_t *mutex);

/**
 * 带超时等待
 */
JW_API jw_status_t jw_cond_timedwait(jw_cond_t *cond, 
                                      jw_mutex_t *mutex,
                                      jw_uint32_t timeout_ms);

/**
 * 唤醒一个等待线程
 */
JW_API jw_status_t jw_cond_signal(jw_cond_t *cond);

/**
 * 唤醒所有等待线程
 */
JW_API jw_status_t jw_cond_broadcast(jw_cond_t *cond);

/*
 * =============================================================================
 * 信号量 (Semaphore)
 * =============================================================================
 */

/**
 * 创建信号量
 * 
 * @param arena     内存池
 * @param initial  初始值
 * @param max      最大值
 * @param sem      输出信号量
 */
JW_API jw_status_t jw_sem_create(jw_arena_t *arena,
                                  jw_uint32_t initial,
                                  jw_uint32_t max,
                                  jw_sem_t **sem);

/**
 * 销毁信号量
 */
JW_API void jw_sem_destroy(jw_sem_t *sem);

/**
 * 等待信号量 (P操作，减1)
 */
JW_API jw_status_t jw_sem_wait(jw_sem_t *sem);

/**
 * 带超时等待
 */
JW_API jw_status_t jw_sem_timedwait(jw_sem_t *sem, jw_uint32_t timeout_ms);

/**
 * 尝试等待
 */
JW_API jw_status_t jw_sem_trywait(jw_sem_t *sem);

/**
 * 释放信号量 (V操作，加1)
 */
JW_API jw_status_t jw_sem_post(jw_sem_t *sem);

/**
 * 获取信号量当前值
 */
JW_API jw_uint32_t jw_sem_get_value(jw_sem_t *sem);

/*
 * =============================================================================
 * 原子操作
 * =============================================================================
 */

/* 原子整数类型 */
typedef struct jw_atomic_int {
    volatile jw_int32_t value;
} jw_atomic_int_t;

/* 原子指针类型 */
typedef struct jw_atomic_ptr {
    volatile void *value;
} jw_atomic_ptr_t;

/* 原子整数64位 */
typedef struct jw_atomic_int64 {
    volatile jw_int64_t value;
} jw_atomic_int64_t;

/**
 * 原子读
 */
JW_API jw_int32_t jw_atomic_get(jw_atomic_int_t *atomic);

/**
 * 原子写
 */
JW_API void jw_atomic_set(jw_atomic_int_t *atomic, jw_int32_t value);

/**
 * 原子加
 * 
 * @return 加之前的值
 */
JW_API jw_int32_t jw_atomic_fetch_add(jw_atomic_int_t *atomic, jw_int32_t delta);

/**
 * 原子减
 */
JW_API jw_int32_t jw_atomic_fetch_sub(jw_atomic_int_t *atomic, jw_int32_t delta);

/**
 * 原子交换
 * 
 * @return 交换之前的值
 */
JW_API jw_int32_t jw_atomic_exchange(jw_atomic_int_t *atomic, jw_int32_t new_val);

/**
 * 比较并交换 (CAS)
 * 
 * if (atomic->value == expected) {
 *     atomic->value = new_val;
 *     return JW_TRUE;
 * }
 * return JW_FALSE;
 * 
 * @return JW_TRUE 成功交换
 */
JW_API jw_bool_t jw_atomic_compare_exchange(jw_atomic_int_t *atomic,
                                             jw_int32_t expected,
                                             jw_int32_t new_val);

/**
 * 原子指针读
 */
JW_API void* jw_atomic_ptr_get(jw_atomic_ptr_t *atomic);

/**
 * 原子指针写
 */
JW_API void jw_atomic_ptr_set(jw_atomic_ptr_t *atomic, void *value);

/**
 * 原子指针比较交换
 */
JW_API jw_bool_t jw_atomic_ptr_compare_exchange(jw_atomic_ptr_t *atomic,
                                                 void *expected,
                                                 void *new_val);

/*
 * =============================================================================
 * 自动锁 (RAII风格)
 * =============================================================================
 */

/* 自动锁上下文 */
typedef struct jw_scoped_lock {
    jw_lock_t *lock;
    jw_status_t status;
} jw_scoped_lock_t;

/**
 * 进入作用域时自动加锁
 * 
 * 使用示例:
 *   JW_SCOPED_LOCK(lock) {
 *       // 临界区代码
 *   }
 */
#define JW_SCOPED_LOCK(lock) \
    for (jw_scoped_lock_t __scoped = {lock, jw_lock_acquire(lock)}; \
         __scoped.status == JW_SUCCESS; \
         jw_lock_release(lock), __scoped.status = JW_FALSE)

/**
 * 初始化锁属性为默认值
 */
JW_API void jw_lock_attr_default(jw_lock_attr_t *attr);

/*
 * =============================================================================
 * 读写锁自动锁定宏
 * =============================================================================
 */

/* 读锁作用域 */
#define JW_RDLOCK_SCOPE(rwlock) \
    for (int __rdlock_once = (jw_rwlock_rdlock(rwlock), 1); \
         __rdlock_once; \
         jw_rwlock_unlock(rwlock), __rdlock_once = 0)

/* 写锁作用域 */
#define JW_WRLOCK_SCOPE(rwlock) \
    for (int __wrlock_once = (jw_rwlock_wrlock(rwlock), 1); \
         __wrlock_once; \
         jw_rwlock_unlock(rwlock), __wrlock_once = 0)

/*
 * =============================================================================
 * 线程局部存储 (TLS)
 * =============================================================================
 */

/* TLS键类型 */
typedef jw_uint32_t jw_tls_key_t;

/**
 * 创建TLS键
 */
JW_API jw_status_t jw_tls_create(jw_tls_key_t *key);

/**
 * 销毁TLS键
 */
JW_API void jw_tls_destroy(jw_tls_key_t key);

/**
 * 设置TLS值
 */
JW_API jw_status_t jw_tls_set(jw_tls_key_t key, void *value);

/**
 * 获取TLS值
 */
JW_API void* jw_tls_get(jw_tls_key_t key);

/*
 * =============================================================================
 * 死锁检测 (调试模式)
 * =============================================================================
 */

#ifdef JW_DEBUG

/* 锁持有信息 */
typedef struct jw_lock_info {
    jw_lock_type_t  type;           /* 锁类型 */
    jw_str_t        name;           /* 锁名称 */
    jw_uint64_t     thread_id;      /* 持有线程ID */
    jw_uint64_t     lock_time;      /* 加锁时间 */
    jw_str_t        lock_file;      /* 加锁文件 */
    int             lock_line;      /* 加锁行号 */
} jw_lock_info_t;

/**
 * 启用死锁检测
 */
JW_API void jw_lock_debug_enable(jw_bool_t enable);

/**
 * 设置锁持有超时警告
 * 
 * @param timeout_ms 超时时间 (毫秒)，超过后打印警告
 */
JW_API void jw_lock_set_timeout_warning(jw_uint32_t timeout_ms);

/**
 * 打印所有锁的状态
 */
JW_API void jw_lock_dump_all(void);

/**
 * 调试版加锁 (记录文件和行号)
 */
JW_API jw_status_t jw_mutex_lock_debug(jw_mutex_t *mutex,
                                        const char *file,
                                        int line);

/* 调试版锁宏 */
#define jw_mutex_lock(m) jw_mutex_lock_debug(m, __FILE__, __LINE__)

#endif /* JW_DEBUG */

/*
 * =============================================================================
 * 平台相关实现声明 (内部使用)
 * =============================================================================
 */

/* 平台互斥锁初始化 */
JW_API jw_status_t jw_mutex_init_platform(jw_mutex_t *mutex, 
                                           const jw_lock_attr_t *attr);

/* 平台互斥锁销毁 */
JW_API void jw_mutex_deinit_platform(jw_mutex_t *mutex);

/* 平台读写锁初始化 */
JW_API jw_status_t jw_rwlock_init_platform(jw_rwlock_t *rwlock,
                                            const jw_lock_attr_t *attr);

/* 平台读写锁销毁 */
JW_API void jw_rwlock_deinit_platform(jw_rwlock_t *rwlock);

/*
 * =============================================================================
 * 便捷宏定义
 * =============================================================================
 */

/* 初始化原子整数 */
#define JW_ATOMIC_INIT(val)     {val}

/* 声明静态原子整数 */
#define JW_ATOMIC_STATIC(name, val) \
    static jw_atomic_int_t name = JW_ATOMIC_INIT(val)

/* 原子自增并获取 */
#define JW_ATOMIC_INC(atomic)   (jw_atomic_fetch_add(atomic, 1) + 1)

/* 原子自减并获取 */
#define JW_ATOMIC_DEC(atomic)   (jw_atomic_fetch_sub(atomic, 1) - 1)

/* 自旋等待直到条件满足 */
#define JW_SPIN_UNTIL(cond, max_spin) \
    do { \
        int __spin_count = 0; \
        while (!(cond) && __spin_count < (max_spin)) { \
            __spin_count++; \
        } \
    } while(0)

JW_END_DECL

#endif /* JW_LOCK_H */

/*
 * =============================================================================
 * 使用示例
 * =============================================================================
 * 
 * // 示例1: 基本互斥锁使用
 * jw_mutex_t *mutex;
 * jw_lock_attr_t attr;
 * jw_lock_attr_default(&attr);
 * attr.type = JW_LOCK_MUTEX;
 * 
 * jw_mutex_create(NULL, &attr, &mutex);
 * 
 * jw_mutex_lock(mutex);
 * // 临界区代码
 * counter++;
 * jw_mutex_unlock(mutex);
 * 
 * jw_mutex_destroy(mutex);
 * 
 * 
 * // 示例2: 使用作用域锁 (推荐)
 * jw_lock_t *lock;
 * jw_lock_create(NULL, NULL, &lock);
 * 
 * JW_SCOPED_LOCK(lock) {
 *     // 自动加锁，退出作用域自动解锁
 *     data->value++;
 * }
 * 
 * 
 * // 示例3: 读写锁 (读多写少场景)
 * jw_rwlock_t *rwlock;
 * jw_rwlock_create(NULL, NULL, &rwlock);
 * 
 * // 读者
 * JW_RDLOCK_SCOPE(rwlock) {
 *     int val = data->value;  // 多个读者可同时读
 * }
 * 
 * // 写者
 * JW_WRLOCK_SCOPE(rwlock) {
 *     data->value = new_val;  // 写者独占
 * }
 * 
 * 
 * // 示例4: 原子操作 (无锁计数器)
 * JW_ATOMIC_STATIC(counter, 0);
 * 
 * // 原子自增
 * jw_int32_t old = jw_atomic_fetch_add(&counter, 1);
 * 
 * // 比较交换
 * jw_int32_t expected = 10;
 * if (jw_atomic_compare_exchange(&counter, expected, 20)) {
 *     printf("CAS成功\n");
 * }
 * 
 * 
 * // 示例5: 条件变量
 * jw_mutex_t *mutex;
 * jw_cond_t *cond;
 * jw_mutex_create(NULL, NULL, &mutex);
 * jw_cond_create(NULL, &cond);
 * 
 * // 等待线程
 * jw_mutex_lock(mutex);
 * while (!condition) {
 *     jw_cond_wait(cond, mutex);
 * }
 * // 处理条件满足后的逻辑
 * jw_mutex_unlock(mutex);
 * 
 * // 通知线程
 * jw_mutex_lock(mutex);
 * condition = 1;
 * jw_cond_signal(cond);  // 或 jw_cond_broadcast(cond)
 * jw_mutex_unlock(mutex);
 * 
 * 
 * // 示例6: 超时等待
 * jw_status_t status = jw_mutex_timedlock(mutex, 1000); // 1秒超时
 * if (status == JW_TIMEOUT) {
 *     printf("获取锁超时\n");
 * } else if (status == JW_SUCCESS) {
 *     // 成功获取锁
 *     jw_mutex_unlock(mutex);
 * }
 * 
 * 
 * // 示例7: 线程局部存储
 * jw_tls_key_t tls_key;
 * jw_tls_create(&tls_key);
 * 
 * // 设置线程私有数据
 * jw_tls_set(tls_key, my_data);
 * 
 * // 获取线程私有数据
 * void *data = jw_tls_get(tls_key);
 * 
 * jw_tls_destroy(tls_key);
 */
