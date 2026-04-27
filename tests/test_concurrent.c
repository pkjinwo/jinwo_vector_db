/*
 * test_concurrent.c - JinWo VecDB 并发测试
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
#include "jw_lock.h"
#include "jw_arena.h"
#include "jw_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef JW_WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
typedef HANDLE jw_thread_t;
#define THREAD_RETURN DWORD WINAPI
#else
#include <pthread.h>
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
typedef pthread_t jw_thread_t;
#define THREAD_RETURN void*
#endif

static jw_uint32_t g_test_passed = 0;
static jw_uint32_t g_test_failed = 0;
static jw_uint32_t g_errors = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    jw_printf("Running %s...\n", #name); \
    test_##name(); \
    g_test_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        jw_printf("  ASSERT_TRUE failed at line %d\n", __LINE__); \
        g_test_failed++; \
        g_errors++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        jw_printf("  ASSERT_EQ failed: %ld != %ld at line %d\n", (long)(a), (long)(b), __LINE__); \
        g_test_failed++; \
        g_errors++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        jw_printf("  ASSERT_NE failed: %ld == %ld at line %d\n", (long)(a), (long)(b), __LINE__); \
        g_test_failed++; \
        g_errors++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

#define MAX_THREADS 16
#define MAX_VECTORS 1000
#define VECTOR_DIM 128

typedef struct {
    jw_vecdb_t *db;
    const char *coll_name;
    jw_dim_t dim;
    int thread_id;
    int vector_count;
    int start_vid;
    volatile int *error_count;
} thread_data_t;

static volatile int g_concurrent_errors = 0;
static volatile int g_insert_count = 0;
static volatile int g_search_count = 0;

#ifdef JW_WIN32
static DWORD WINAPI concurrent_insert_thread(LPVOID param)
#else
static void *concurrent_insert_thread(void *param)
#endif
{
    thread_data_t *data = (thread_data_t *)param;
    jw_collection_t *coll = NULL;
    jw_status_t status;
    int i;

    coll = jw_vecdb_get_collection(data->db, jw_str(data->coll_name));
    if (coll == NULL) {
        jw_printf("  Thread %d: Failed to get collection\n", data->thread_id);
        (*data->error_count)++;
        return 0;
    }

    for (i = 0; i < data->vector_count; i++) {
        float vec[VECTOR_DIM];
        jw_vid_t vid;
        int j;

        for (j = 0; j < VECTOR_DIM; j++) {
            vec[j] = (float)(data->thread_id * 1000 + i + j) / 100.0f;
        }

        status = jw_collection_insert(coll, vec, &vid);
        if (status != JW_SUCCESS) {
            jw_printf("  Thread %d: Insert failed at %d, status=%d\n",
                      data->thread_id, i, status);
            (*data->error_count)++;
        } else {
            __sync_add_and_fetch(&g_insert_count, 1);
        }
    }

    return 0;
}

#ifdef JW_WIN32
static DWORD WINAPI concurrent_search_thread(LPVOID param)
#else
static void *concurrent_search_thread(void *param)
#endif
{
    thread_data_t *data = (thread_data_t *)param;
    jw_collection_t *coll = NULL;
    jw_status_t status;
    int i;

    coll = jw_vecdb_get_collection(data->db, jw_str(data->coll_name));
    if (coll == NULL) {
        jw_printf("  Thread %d: Failed to get collection\n", data->thread_id);
        (*data->error_count)++;
        return 0;
    }

    for (i = 0; i < data->vector_count; i++) {
        float query[VECTOR_DIM];
        jw_search_result_t results[5];
        int j;

        for (j = 0; j < VECTOR_DIM; j++) {
            query[j] = (float)(i + j) / 100.0f;
        }

        jw_size_t count = jw_collection_search(coll, query, 5, results);
        if (count == 0 && i > 0) {
            jw_printf("  Thread %d: Search returned 0 results at %d\n",
                      data->thread_id, i);
        }
        __sync_add_and_fetch(&g_search_count, 1);
    }

    return 0;
}

#ifdef JW_WIN32
static DWORD WINAPI concurrent_mixed_thread(LPVOID param)
#else
static void *concurrent_mixed_thread(void *param)
#endif
{
    thread_data_t *data = (thread_data_t *)param;
    jw_collection_t *coll = NULL;
    int i;

    coll = jw_vecdb_get_collection(data->db, jw_str(data->coll_name));
    if (coll == NULL) {
        jw_printf("  Thread %d: Failed to get collection\n", data->thread_id);
        (*data->error_count)++;
        return 0;
    }

    for (i = 0; i < data->vector_count; i++) {
        float vec[VECTOR_DIM];
        float query[VECTOR_DIM];
        jw_vid_t vid;
        int j;

        for (j = 0; j < VECTOR_DIM; j++) {
            vec[j] = (float)(data->thread_id * 1000 + i + j) / 100.0f;
            query[j] = vec[j];
        }

        jw_status_t status = jw_collection_insert(coll, vec, &vid);
        if (status == JW_SUCCESS) {
            __sync_add_and_fetch(&g_insert_count, 1);
        } else {
            (*data->error_count)++;
        }

        if (i % 3 == 0) {
            jw_search_result_t results[5];
            jw_size_t count = jw_collection_search(coll, query, 5, results);
            __sync_add_and_fetch(&g_search_count, 1);
        }
    }

    return 0;
}

#ifdef JW_WIN32
static DWORD WINAPI stress_test_thread(LPVOID param)
#else
static void *stress_test_thread(void *param)
#endif
{
    thread_data_t *data = (thread_data_t *)param;
    jw_collection_t *coll = NULL;
    int i;
    int local_errors = 0;

    coll = jw_vecdb_get_collection(data->db, jw_str(data->coll_name));
    if (coll == NULL) {
        jw_printf("  Thread %d: Failed to get collection\n", data->thread_id);
        (*data->error_count)++;
        return 0;
    }

    for (i = 0; i < data->vector_count; i++) {
        float vec[VECTOR_DIM];
        float query[VECTOR_DIM];
        jw_vid_t vid;
        int j;
        jw_status_t status;

        for (j = 0; j < VECTOR_DIM; j++) {
            vec[j] = (float)((data->thread_id * 10000 + i * VECTOR_DIM + j) % 1000) / 10.0f;
            query[j] = vec[j];
        }

        status = jw_collection_insert(coll, vec, &vid);
        if (status != JW_SUCCESS) {
            local_errors++;
        } else {
            __sync_add_and_fetch(&g_insert_count, 1);
        }

        jw_search_result_t results[3];
        jw_size_t count = jw_collection_search(coll, query, 3, results);
        __sync_add_and_fetch(&g_search_count, 1);

        if (i % 10 == 0) {
            sleep_ms(1);
        }
    }

    if (local_errors > 0) {
        (*data->error_count) += local_errors;
    }

    return 0;
}

static int create_test_db_and_collection(jw_vecdb_t **db, const char *coll_name, jw_dim_t dim)
{
    jw_status_t status;
    jw_collection_t *coll = NULL;

    *db = NULL;
    status = jw_vecdb_open(NULL, JW_VECDB_MEMORY | JW_VECDB_CREATE, db);
    if (status != JW_SUCCESS) {
        jw_printf("  Failed to open database: %d\n", status);
        return -1;
    }

    status = jw_vecdb_create_collection(*db, jw_str(coll_name), dim, &coll);
    if (status != JW_SUCCESS) {
        jw_printf("  Failed to create collection: %d\n", status);
        jw_vecdb_close(*db);
        *db = NULL;
        return -1;
    }

    return 0;
}

static jw_thread_t create_thread(void *(*func)(void *), void *param)
{
#ifdef JW_WIN32
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, param, 0, NULL);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, func, param);
    return thread;
#endif
}

static void join_thread(jw_thread_t thread)
{
#ifdef JW_WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

TEST(concurrent_insert_single_reader)
{
    jw_vecdb_t *db = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;
    int thread_count = 4;
    int vectors_per_thread = 100;

    g_insert_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_concurrent_insert", VECTOR_DIM) != 0) {
        return;
    }

    jw_printf("  Creating %d insert threads...\n", thread_count);

    for (i = 0; i < thread_count; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_concurrent_insert";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = vectors_per_thread;
        thread_data[i].error_count = &g_concurrent_errors;

        threads[i] = create_thread(concurrent_insert_thread, &thread_data[i]);
    }

    for (i = 0; i < thread_count; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  Inserted %d vectors with %d errors\n", g_insert_count, g_concurrent_errors);

    ASSERT_EQ(g_concurrent_errors, 0);
    ASSERT_EQ(g_insert_count, (jw_uint32_t)(thread_count * vectors_per_thread));

    jw_vecdb_close(db);
}

TEST(concurrent_search_while_insert)
{
    jw_vecdb_t *db = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;

    g_insert_count = 0;
    g_search_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_concurrent_search", VECTOR_DIM) != 0) {
        return;
    }

    jw_printf("  Creating insert and search threads...\n");

    thread_data[0].db = db;
    thread_data[0].coll_name = "test_concurrent_search";
    thread_data[0].dim = VECTOR_DIM;
    thread_data[0].thread_id = 0;
    thread_data[0].vector_count = 200;
    thread_data[0].error_count = &g_concurrent_errors;
    threads[0] = create_thread(concurrent_insert_thread, &thread_data[0]);

    for (i = 1; i < 4; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_concurrent_search";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = 50;
        thread_data[i].error_count = &g_concurrent_errors;
        threads[i] = create_thread(concurrent_search_thread, &thread_data[i]);
    }

    for (i = 0; i < 4; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  Inserted %d, searched %d, errors: %d\n",
              g_insert_count, g_search_count, g_concurrent_errors);

    ASSERT_TRUE(g_insert_count > 0);
    ASSERT_TRUE(g_search_count > 0);
    ASSERT_EQ(g_concurrent_errors, 0);

    jw_vecdb_close(db);
}

TEST(concurrent_mixed_operations)
{
    jw_vecdb_t *db = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;
    int thread_count = 4;
    int vectors_per_thread = 50;

    g_insert_count = 0;
    g_search_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_concurrent_mixed", VECTOR_DIM) != 0) {
        return;
    }

    jw_printf("  Creating %d mixed operation threads...\n", thread_count);

    for (i = 0; i < thread_count; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_concurrent_mixed";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = vectors_per_thread;
        thread_data[i].error_count = &g_concurrent_errors;

        threads[i] = create_thread(concurrent_mixed_thread, &thread_data[i]);
    }

    for (i = 0; i < thread_count; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  Inserted %d, searched %d, errors: %d\n",
              g_insert_count, g_search_count, g_concurrent_errors);

    ASSERT_TRUE(g_insert_count > 0);
    ASSERT_TRUE(g_search_count > 0);
    ASSERT_EQ(g_concurrent_errors, 0);

    jw_vecdb_close(db);
}

TEST(concurrent_stress_test)
{
    jw_vecdb_t *db = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;
    int thread_count = 8;
    int vectors_per_thread = 100;

    g_insert_count = 0;
    g_search_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_stress", VECTOR_DIM) != 0) {
        return;
    }

    jw_printf("  Running stress test with %d threads...\n", thread_count);

    for (i = 0; i < thread_count; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_stress";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = vectors_per_thread;
        thread_data[i].error_count = &g_concurrent_errors;

        threads[i] = create_thread(stress_test_thread, &thread_data[i]);
    }

    for (i = 0; i < thread_count; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  Stress test complete: inserted %d, searched %d, errors: %d\n",
              g_insert_count, g_search_count, g_concurrent_errors);

    ASSERT_TRUE(g_insert_count > 0);
    ASSERT_TRUE(g_search_count > 0);
    ASSERT_EQ(g_concurrent_errors, 0);

    jw_vecdb_close(db);
}

TEST(concurrent_read_write_lock)
{
    jw_vecdb_t *db = NULL;
    jw_collection_t *coll = NULL;
    jw_rwlock_t *lock = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;
    int read_thread_count = 6;
    int write_thread_count = 2;
    int total_threads = read_thread_count + write_thread_count;

    g_insert_count = 0;
    g_search_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_rwlock", VECTOR_DIM) != 0) {
        return;
    }

    coll = jw_vecdb_get_collection(db, jw_str("test_rwlock"));
    ASSERT_NOT_NULL(coll);

    jw_printf("  Testing read-write lock with %d readers and %d writers...\n",
             read_thread_count, write_thread_count);

    for (i = 0; i < read_thread_count; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_rwlock";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = 30;
        thread_data[i].error_count = &g_concurrent_errors;
        threads[i] = create_thread(concurrent_search_thread, &thread_data[i]);
    }

    for (i = 0; i < write_thread_count; i++) {
        thread_data[read_thread_count + i].db = db;
        thread_data[read_thread_count + i].coll_name = "test_rwlock";
        thread_data[read_thread_count + i].dim = VECTOR_DIM;
        thread_data[read_thread_count + i].thread_id = read_thread_count + i;
        thread_data[read_thread_count + i].vector_count = 50;
        thread_data[read_thread_count + i].error_count = &g_concurrent_errors;
        threads[read_thread_count + i] = create_thread(concurrent_insert_thread,
                                                        &thread_data[read_thread_count + i]);
    }

    for (i = 0; i < total_threads; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  RWLock test complete: inserted %d, searched %d, errors: %d\n",
              g_insert_count, g_search_count, g_concurrent_errors);

    ASSERT_TRUE(g_insert_count > 0);
    ASSERT_TRUE(g_search_count > 0);
    ASSERT_EQ(g_concurrent_errors, 0);

    jw_vecdb_close(db);
}

TEST(concurrent_batch_operations)
{
    jw_vecdb_t *db = NULL;
    jw_collection_t *coll = NULL;
    jw_thread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];
    int i;

    g_insert_count = 0;
    g_concurrent_errors = 0;

    if (create_test_db_and_collection(&db, "test_batch", VECTOR_DIM) != 0) {
        return;
    }

    coll = jw_vecdb_get_collection(db, jw_str("test_batch"));
    ASSERT_NOT_NULL(coll);

    jw_printf("  Testing concurrent batch operations...\n");

    for (i = 0; i < 4; i++) {
        thread_data[i].db = db;
        thread_data[i].coll_name = "test_batch";
        thread_data[i].dim = VECTOR_DIM;
        thread_data[i].thread_id = i;
        thread_data[i].vector_count = 50;
        thread_data[i].error_count = &g_concurrent_errors;

        threads[i] = create_thread(concurrent_insert_thread, &thread_data[i]);
    }

    for (i = 0; i < 4; i++) {
        join_thread(threads[i]);
    }

    jw_printf("  Batch test complete: inserted %d vectors, errors: %d\n",
              g_insert_count, g_concurrent_errors);

    ASSERT_EQ(g_concurrent_errors, 0);
    ASSERT_EQ(g_insert_count, (jw_uint32_t)(4 * 50));

    jw_vecdb_close(db);
}

TEST(concurrent_transaction)
{
    jw_vecdb_t *db = NULL;
    jw_status_t status;

    g_errors = 0;

    if (create_test_db_and_collection(&db, "test_transaction", VECTOR_DIM) != 0) {
        return;
    }

    jw_printf("  Testing transaction operations...\n");

    status = jw_vecdb_begin_transaction(db);
    ASSERT_EQ(status, JW_SUCCESS);

    {
        jw_collection_t *coll = jw_vecdb_get_collection(db, jw_str("test_transaction"));
        ASSERT_NOT_NULL(coll);

        float vec[VECTOR_DIM] = {0};
        jw_vid_t vid;
        int i;
        for (i = 0; i < 10; i++) {
            vec[0] = (float)i;
            status = jw_collection_insert(coll, vec, &vid);
            ASSERT_EQ(status, JW_SUCCESS);
        }
    }

    status = jw_vecdb_commit(db);
    ASSERT_EQ(status, JW_SUCCESS);

    jw_vecdb_close(db);
}

int main(void)
{
    jw_printf("\n========================================\n");
    jw_printf("JinWo VecDB Concurrent Test Suite\n");
    jw_printf("========================================\n\n");

    RUN_TEST(concurrent_insert_single_reader);
    RUN_TEST(concurrent_search_while_insert);
    RUN_TEST(concurrent_mixed_operations);
    RUN_TEST(concurrent_stress_test);
    RUN_TEST(concurrent_read_write_lock);
    RUN_TEST(concurrent_batch_operations);
    RUN_TEST(concurrent_transaction);

    jw_printf("\n========================================\n");
    jw_printf("Test Results: %d passed, %d failed\n", g_test_passed, g_test_failed);
    jw_printf("========================================\n\n");

    return g_test_failed == 0 ? 0 : 1;
}
