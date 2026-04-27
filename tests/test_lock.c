/*
 * test_lock.c - JinWo VecDB 锁机制测试
 */

#include "jw_lock.h"
#include "jw_arena.h"

static jw_uint32_t test_passed = 0;
static jw_uint32_t test_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    test_##name(); \
    test_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

static jw_arena_t* create_test_arena(void)
{
    jw_arena_t config = {0};
    config.block_size = 4096;
    config.max_size = 65536;

    jw_arena_t *arena = NULL;
    jw_status_t status = jw_arena_create(&config, &arena);
    if (status != JW_SUCCESS) {
        return NULL;
    }
    return arena;
}

TEST(mutex_create_destroy)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_mutex_t *mutex = NULL;
    jw_status_t status = jw_mutex_create(arena, NULL, &mutex);
    ASSERT_EQ(status, JW_SUCCESS);
    ASSERT_NOT_NULL(mutex);

    jw_mutex_destroy(mutex);
    jw_arena_destroy(arena);
}

TEST(mutex_lock_unlock)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_mutex_t *mutex = NULL;
    jw_status_t status = jw_mutex_create(arena, NULL, &mutex);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_mutex_lock(mutex);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_mutex_unlock(mutex);
    ASSERT_EQ(status, JW_SUCCESS);

    jw_mutex_destroy(mutex);
    jw_arena_destroy(arena);
}

TEST(mutex_trylock)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_mutex_t *mutex = NULL;
    jw_status_t status = jw_mutex_create(arena, NULL, &mutex);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_mutex_trylock(mutex);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_mutex_trylock(mutex);
    ASSERT_EQ(status, JW_BUSY);

    jw_mutex_unlock(mutex);
    jw_mutex_destroy(mutex);
    jw_arena_destroy(arena);
}

TEST(rwlock_create_destroy)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_rwlock_t *rwlock = NULL;
    jw_status_t status = jw_rwlock_create(arena, NULL, &rwlock);
    ASSERT_EQ(status, JW_SUCCESS);
    ASSERT_NOT_NULL(rwlock);

    jw_rwlock_destroy(rwlock);
    jw_arena_destroy(arena);
}

TEST(rwlock_rdlock)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_rwlock_t *rwlock = NULL;
    jw_status_t status = jw_rwlock_create(arena, NULL, &rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_rwlock_rdlock(rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_rwlock_rdunlock(rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    jw_rwlock_destroy(rwlock);
    jw_arena_destroy(arena);
}

TEST(rwlock_wrlock)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_rwlock_t *rwlock = NULL;
    jw_status_t status = jw_rwlock_create(arena, NULL, &rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_rwlock_wrlock(rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    status = jw_rwlock_wrunlock(rwlock);
    ASSERT_EQ(status, JW_SUCCESS);

    jw_rwlock_destroy(rwlock);
    jw_arena_destroy(arena);
}

TEST(atomic_int)
{
    jw_atomic_int_t atomic = JW_ATOMIC_INIT(0);

    ASSERT_EQ(jw_atomic_get(&atomic), 0);

    jw_atomic_set(&atomic, 42);
    ASSERT_EQ(jw_atomic_get(&atomic), 42);

    jw_int32_t old = jw_atomic_fetch_add(&atomic, 10);
    ASSERT_EQ(old, 42);
    ASSERT_EQ(jw_atomic_get(&atomic), 52);

    old = jw_atomic_fetch_sub(&atomic, 2);
    ASSERT_EQ(old, 52);
    ASSERT_EQ(jw_atomic_get(&atomic), 50);

    old = jw_atomic_exchange(&atomic, 100);
    ASSERT_EQ(old, 50);
    ASSERT_EQ(jw_atomic_get(&atomic), 100);
}

TEST(atomic_compare_exchange)
{
    jw_atomic_int_t atomic = JW_ATOMIC_INIT(42);

    jw_bool_t result = jw_atomic_compare_exchange(&atomic, 42, 100);
    ASSERT_TRUE(result);
    ASSERT_EQ(jw_atomic_get(&atomic), 100);

    result = jw_atomic_compare_exchange(&atomic, 42, 200);
    ASSERT_FALSE(result);
    ASSERT_EQ(jw_atomic_get(&atomic), 100);
}

TEST(atomic_ptr)
{
    jw_atomic_ptr_t atomic;
    jw_atomic_ptr_set(&atomic, NULL);

    int value = 42;
    jw_atomic_ptr_set(&atomic, &value);

    void *ptr = jw_atomic_ptr_get(&atomic);
    ASSERT_EQ(ptr, &value);

    int value2 = 100;
    jw_bool_t result = jw_atomic_ptr_compare_exchange(&atomic, &value, &value2);
    ASSERT_TRUE(result);
    ASSERT_EQ(jw_atomic_ptr_get(&atomic), &value2);
}

TEST(lock_attr_default)
{
    jw_lock_attr_t attr;
    jw_lock_attr_default(&attr);

    ASSERT_EQ(attr.type, JW_LOCK_MUTEX);
    ASSERT_FALSE(attr.process_shared);
}

TEST(atomic_inc_dec)
{
    JW_ATOMIC_STATIC(counter, 10);

    jw_int32_t result = JW_ATOMIC_INC(&counter);
    ASSERT_EQ(result, 11);
    ASSERT_EQ(jw_atomic_get(&counter), 11);

    result = JW_ATOMIC_DEC(&counter);
    ASSERT_EQ(result, 10);
    ASSERT_EQ(jw_atomic_get(&counter), 10);
}

int main(void)
{
    RUN_TEST(mutex_create_destroy);
    RUN_TEST(mutex_lock_unlock);
    RUN_TEST(mutex_trylock);
    RUN_TEST(rwlock_create_destroy);
    RUN_TEST(rwlock_rdlock);
    RUN_TEST(rwlock_wrlock);
    RUN_TEST(atomic_int);
    RUN_TEST(atomic_compare_exchange);
    RUN_TEST(atomic_ptr);
    RUN_TEST(lock_attr_default);
    RUN_TEST(atomic_inc_dec);

    return (int)test_failed;
}