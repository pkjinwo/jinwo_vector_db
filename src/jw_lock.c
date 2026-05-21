/*
 * jw_lock.c - JinWo VecDB 锁机制实现
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

#include "jw_lock.h"
#include "jw_arena.h"
#include "jw_string.h"
#include <stdlib.h>
#include <errno.h>

#ifdef JW_WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <time.h>
#endif

/* 自旋锁类型 */
typedef volatile jw_int32_t jw_spinlock_t;

/* 线程函数类型 */
typedef void (*jw_thread_func_t)(void *data);

/* 线程结构 */
typedef struct jw_thread_t {
    #ifdef JW_WIN32
    HANDLE handle;
    #else
    pthread_t thread;
    #endif
    jw_thread_func_t func;
    void *data;
} jw_thread_t;

/* 信号量实现辅助定义 */
#ifdef JW_WIN32
#define jw_sem_impl(sem) (sem)
#else
#define jw_sem_impl(sem) (sem)
#endif

/* 自动锁结构 */
typedef struct jw_autolock_t {
    jw_mutex_t *mutex;
} jw_autolock_t;

/*
 * =============================================================================
 * 互斥锁实现
 * =============================================================================
 */

JW_API jw_status_t jw_mutex_create(jw_arena_t *arena,
                                    const jw_lock_attr_t *attr,
                                    jw_mutex_t **mutex)
{
    if (mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size;
#ifdef JW_WIN32
    size = sizeof(CRITICAL_SECTION);
#else
    size = sizeof(pthread_mutex_t);
#endif
    
    void *mem;
    if (arena) {
        mem = jw_arena_alloc(arena, size);
    } else {
        mem = jw_malloc(size);
    }
    
    if (mem == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    *mutex = (jw_mutex_t *)mem;
    
#ifdef JW_WIN32
    InitializeCriticalSection(*mutex);
    return JW_SUCCESS;
#else
    int ret = pthread_mutex_init(*mutex, NULL);
    if (ret != 0) {
        if (!arena) {
            jw_free(mem);
        }
        return JW_UNKNOWN_ERROR;
    }
    return JW_SUCCESS;
#endif
}

JW_API void jw_mutex_destroy(jw_mutex_t *mutex)
{
    if (mutex == NULL) {
        return;
    }
    
#ifdef JW_WIN32
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(mutex);
#endif
}

JW_API jw_status_t jw_mutex_lock(jw_mutex_t *mutex)
{
    if (mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    EnterCriticalSection(mutex);
    return JW_SUCCESS;
#else
    int ret = pthread_mutex_lock(mutex);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_mutex_trylock(jw_mutex_t *mutex)
{
    if (mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    BOOL success = TryEnterCriticalSection(mutex);
    return success ? JW_SUCCESS : JW_BUSY;
#else
    int ret = pthread_mutex_trylock(mutex);
    if (ret == EBUSY) {
        return JW_BUSY;
    }
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_mutex_unlock(jw_mutex_t *mutex)
{
    if (mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    LeaveCriticalSection(mutex);
    return JW_SUCCESS;
#else
    int ret = pthread_mutex_unlock(mutex);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

/*
 * =============================================================================
 * 读写锁实现
 * =============================================================================
 */

JW_API jw_status_t jw_rwlock_create(jw_arena_t *arena,
                                     const jw_lock_attr_t *attr,
                                     jw_rwlock_t **rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size;
#ifdef JW_WIN32
    size = sizeof(SRWLOCK);
#else
    size = sizeof(pthread_rwlock_t);
#endif
    
    void *mem;
    if (arena) {
        mem = jw_arena_alloc(arena, size);
    } else {
        mem = jw_malloc(size);
    }
    
    if (mem == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    *rwlock = (jw_rwlock_t *)mem;
    
#ifdef JW_WIN32
    InitializeSRWLock(*rwlock);
    return JW_SUCCESS;
#else
    int ret = pthread_rwlock_init(*rwlock, NULL);
    if (ret != 0) {
        if (!arena) {
            jw_free(mem);
        }
        return JW_UNKNOWN_ERROR;
    }
    return JW_SUCCESS;
#endif
}

JW_API void jw_rwlock_destroy(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    
#ifdef JW_WIN32
    /* SRWLock不需要显式销毁 */
    (void)rwlock;
#else
    pthread_rwlock_destroy(rwlock);
#endif
}

JW_API jw_status_t jw_rwlock_rdlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    AcquireSRWLockShared(rwlock);
    return JW_SUCCESS;
#else
    int ret = pthread_rwlock_rdlock(rwlock);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_rwlock_tryrdlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    /* Windows SRWLock不支持trylock */
    return JW_NOT_SUPPORTED;
#else
    int ret = pthread_rwlock_tryrdlock(rwlock);
    if (ret == EBUSY) {
        return JW_BUSY;
    }
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_rwlock_wrlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    AcquireSRWLockExclusive(rwlock);
    return JW_SUCCESS;
#else
    int ret = pthread_rwlock_wrlock(rwlock);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_rwlock_trywrlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    return JW_NOT_SUPPORTED;
#else
    int ret = pthread_rwlock_trywrlock(rwlock);
    if (ret == EBUSY) {
        return JW_BUSY;
    }
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_rwlock_rdunlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    ReleaseSRWLockShared(rwlock);
    return JW_SUCCESS;
#else
    int ret = pthread_rwlock_unlock(rwlock);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_rwlock_wrunlock(jw_rwlock_t *rwlock)
{
    if (rwlock == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    ReleaseSRWLockExclusive(rwlock);
    return JW_SUCCESS;
#else
    int ret = pthread_rwlock_unlock(rwlock);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

/*
 * =============================================================================
 * 自旋锁实现
 * =============================================================================
 */

JW_API jw_status_t jw_spinlock_create(jw_arena_t *arena,
                                       const jw_lock_attr_t *attr,
                                       jw_lock_t **lock)
{
    if (lock == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size = sizeof(jw_spinlock_t);
    
    void *mem;
    if (arena) {
        mem = jw_arena_alloc(arena, size);
    } else {
        mem = jw_malloc(size);
    }
    
    if (mem == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    jw_spinlock_t *spinlock = (jw_spinlock_t *)mem;
    *spinlock = 0;
    *lock = (jw_lock_t *)spinlock;
    
    return JW_SUCCESS;
}

JW_API void jw_spinlock_destroy(jw_lock_t *lock)
{
    if (lock == NULL) {
        return;
    }
    
    jw_spinlock_t *spinlock = (jw_spinlock_t *)lock;
    *spinlock = 0;
}

JW_API jw_status_t jw_spinlock_lock(jw_lock_t *lock)
{
    if (lock == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_spinlock_t *spinlock = (jw_spinlock_t *)lock;
    
#ifdef JW_WIN32
    /* Windows使用InterlockedCompareExchange */
    while (InterlockedCompareExchange(spinlock, 1, 0) != 0) {
        YieldProcessor();
    }
    return JW_SUCCESS;
#else
    /* 使用GCC内置原子操作 */
    while (__sync_lock_test_and_set(spinlock, 1)) {
        while (*spinlock) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
            __asm__ __volatile__("pause" ::: "memory");
#else
            __sync_synchronize();
#endif
        }
    }
    return JW_SUCCESS;
#endif
}

JW_API jw_status_t jw_spinlock_trylock(jw_lock_t *lock)
{
    if (lock == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_spinlock_t *spinlock = (jw_spinlock_t *)lock;
    
#ifdef JW_WIN32
    LONG ret = InterlockedCompareExchange(spinlock, 1, 0);
    return (ret == 0) ? JW_SUCCESS : JW_BUSY;
#else
    return __sync_lock_test_and_set(spinlock, 1) ? JW_BUSY : JW_SUCCESS;
#endif
}

JW_API jw_status_t jw_spinlock_unlock(jw_lock_t *lock)
{
    if (lock == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_spinlock_t *spinlock = (jw_spinlock_t *)lock;
    
#ifdef JW_WIN32
    InterlockedExchange(spinlock, 0);
    return JW_SUCCESS;
#else
    __sync_lock_release(spinlock);
    return JW_SUCCESS;
#endif
}

/*
 * =============================================================================
 * 条件变量实现
 * =============================================================================
 */

JW_API jw_status_t jw_cond_create(jw_arena_t *arena,
                                   jw_cond_t **cond)
{
    if (cond == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size;
#ifdef JW_WIN32
    size = sizeof(CONDITION_VARIABLE);
#else
    size = sizeof(pthread_cond_t);
#endif
    
    void *mem;
    if (arena) {
        mem = jw_arena_alloc(arena, size);
    } else {
        mem = jw_malloc(size);
    }
    
    if (mem == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    *cond = (jw_cond_t *)mem;
    
#ifdef JW_WIN32
    InitializeConditionVariable(*cond);
    return JW_SUCCESS;
#else
    int ret = pthread_cond_init(*cond, NULL);
    if (ret != 0) {
        if (!arena) {
            jw_free(mem);
        }
        return JW_UNKNOWN_ERROR;
    }
    return JW_SUCCESS;
#endif
}

JW_API void jw_cond_destroy(jw_cond_t *cond)
{
    if (cond == NULL) {
        return;
    }
    
#ifdef JW_WIN32
    /* Windows条件变量不需要显式销毁 */
    (void)cond;
#else
    pthread_cond_destroy(cond);
#endif
}

JW_API jw_status_t jw_cond_wait(jw_cond_t *cond, jw_mutex_t *mutex)
{
    if (cond == NULL || mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    BOOL success = SleepConditionVariableCS(cond, mutex, INFINITE);
    return success ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    int ret = pthread_cond_wait(cond, mutex);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_cond_timedwait(jw_cond_t *cond,
                                      jw_mutex_t *mutex,
                                      jw_uint32_t ms)
{
    if (cond == NULL || mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    BOOL success = SleepConditionVariableCS(cond, mutex, ms);
    if (!success && GetLastError() == ERROR_TIMEOUT) {
        return JW_TIMEOUT;
    }
    return success ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    int ret = pthread_cond_timedwait(cond, mutex, &ts);
    if (ret == ETIMEDOUT) {
        return JW_TIMEOUT;
    }
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_cond_signal(jw_cond_t *cond)
{
    if (cond == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    WakeConditionVariable(cond);
    return JW_SUCCESS;
#else
    int ret = pthread_cond_signal(cond);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_cond_broadcast(jw_cond_t *cond)
{
    if (cond == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    WakeAllConditionVariable(cond);
    return JW_SUCCESS;
#else
    int ret = pthread_cond_broadcast(cond);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

/*
 * =============================================================================
 * 线程实现
 * =============================================================================
 */

#ifdef JW_WIN32
static DWORD WINAPI thread_wrapper(LPVOID arg)
#else
static void *thread_wrapper(void *arg)
#endif
{
    jw_thread_t *thread = (jw_thread_t *)arg;
    if (thread != NULL && thread->func != NULL) {
        thread->func(thread->data);
    }
    
#ifdef JW_WIN32
    return 0;
#else
    return NULL;
#endif
}

JW_API jw_status_t jw_thread_create(jw_thread_t *thread,
                                     jw_thread_func_t func,
                                     void *data)
{
    if (thread == NULL || func == NULL) {
        return JW_INVALID_PARAM;
    }
    
    thread->func = func;
    thread->data = data;
    
#ifdef JW_WIN32
    thread->handle = CreateThread(NULL, 0, thread_wrapper, thread, 0, NULL);
    return (thread->handle != NULL) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    int ret = pthread_create(&thread->thread, NULL, thread_wrapper, thread);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_thread_join(jw_thread_t *thread)
{
    if (thread == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    return JW_SUCCESS;
#else
    int ret = pthread_join(thread->thread, NULL);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_status_t jw_thread_detach(jw_thread_t *thread)
{
    if (thread == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    CloseHandle(thread->handle);
    return JW_SUCCESS;
#else
    int ret = pthread_detach(thread->thread);
    return (ret == 0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#endif
}

JW_API jw_uint64_t jw_thread_id(void)
{
#ifdef JW_WIN32
    return (jw_uint64_t)GetCurrentThreadId();
#else
    return (jw_uint64_t)pthread_self();
#endif
}

/*
 * =============================================================================
 * 自动锁 (RAII风格)
 * =============================================================================
 */

JW_API jw_status_t jw_autolock_init(jw_autolock_t *autolock, jw_mutex_t *mutex)
{
    if (autolock == NULL || mutex == NULL) {
        return JW_INVALID_PARAM;
    }
    
    autolock->mutex = mutex;
    jw_status_t status = jw_mutex_lock(mutex);
    
    if (status != JW_SUCCESS) {
        autolock->mutex = NULL;
    }
    
    return status;
}

JW_API void jw_autolock_cleanup(jw_autolock_t *autolock)
{
    if (autolock != NULL && autolock->mutex != NULL) {
        jw_mutex_unlock(autolock->mutex);
        autolock->mutex = NULL;
    }
}

/*
 * =============================================================================
 * 信号量实现
 * =============================================================================
 */

JW_API jw_status_t jw_sem_create(jw_arena_t *arena, jw_uint32_t initial, jw_uint32_t max, jw_sem_t **sem)
{
    if (sem == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    *sem = CreateSemaphore(NULL, initial, max, NULL);
    return (*sem != NULL) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    (void)arena;
    jw_sem_t *impl = (jw_sem_t*)malloc(sizeof(jw_sem_t));
    if (impl == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    int ret = pthread_mutex_init(&impl->mutex, NULL);
    if (ret != 0) {
        free(impl);
        return JW_UNKNOWN_ERROR;
    }
    
    ret = pthread_cond_init(&impl->cond, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&impl->mutex);
        free(impl);
        return JW_UNKNOWN_ERROR;
    }
    
    impl->count = initial;
    impl->max = max;
    *sem = impl;
    return JW_SUCCESS;
#endif
}

JW_API void jw_sem_destroy(jw_sem_t *sem)
{
    if (sem == NULL) {
        return;
    }
    
#ifdef JW_WIN32
    CloseHandle(sem);
#else
    pthread_cond_destroy(&sem->cond);
    pthread_mutex_destroy(&sem->mutex);
    free(sem);
#endif
}

JW_API jw_status_t jw_sem_wait(jw_sem_t *sem)
{
    if (sem == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    DWORD ret = WaitForSingleObject(sem, INFINITE);
    return (ret == WAIT_OBJECT_0) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    int ret = pthread_mutex_lock(&sem->mutex);
    if (ret != 0) {
        return JW_UNKNOWN_ERROR;
    }
    
    while (sem->count == 0) {
        ret = pthread_cond_wait(&sem->cond, &sem->mutex);
        if (ret != 0) {
            pthread_mutex_unlock(&sem->mutex);
            return JW_UNKNOWN_ERROR;
        }
    }
    
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);
    return JW_SUCCESS;
#endif
}

JW_API jw_status_t jw_sem_post(jw_sem_t *sem)
{
    if (sem == NULL) {
        return JW_INVALID_PARAM;
    }
    
#ifdef JW_WIN32
    return ReleaseSemaphore(sem, 1, NULL) ? JW_SUCCESS : JW_UNKNOWN_ERROR;
#else
    int ret = pthread_mutex_lock(&sem->mutex);
    if (ret != 0) {
        return JW_UNKNOWN_ERROR;
    }
    
    if (sem->count < sem->max) {
        sem->count++;
        pthread_cond_signal(&sem->cond);
    }
    
    pthread_mutex_unlock(&sem->mutex);
    return JW_SUCCESS;
#endif
}
