/*
 * test_types.c - JinWo VecDB 类型测试
 */

#include "jw_types.h"
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

static void* memset_pattern(void *ptr, jw_uint8_t val, jw_size_t size)
{
    jw_uint8_t *p = (jw_uint8_t *)ptr;
    for (jw_size_t i = 0; i < size; i++) {
        p[i] = val;
    }
    return ptr;
}

static jw_size_t str_len(const char *s)
{
    jw_size_t len = 0;
    while (s[len]) len++;
    return len;
}

static jw_bool_t str_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return JW_FALSE;
        a++;
        b++;
    }
    return (*a == *b);
}

TEST(version)
{
    ASSERT_EQ(JW_VERSION_MAJOR, 0);
    ASSERT_EQ(JW_VERSION_MINOR, 1);
    ASSERT_EQ(JW_VERSION_PATCH, 31);
    ASSERT_TRUE(str_equal(JW_VERSION_STRING, "0.1.34"));
}

TEST(integer_sizes)
{
    ASSERT_EQ(sizeof(jw_int8_t), 1);
    ASSERT_EQ(sizeof(jw_uint8_t), 1);
    ASSERT_EQ(sizeof(jw_int16_t), 2);
    ASSERT_EQ(sizeof(jw_uint16_t), 2);
    ASSERT_EQ(sizeof(jw_int32_t), 4);
    ASSERT_EQ(sizeof(jw_uint32_t), 4);
    ASSERT_EQ(sizeof(jw_int64_t), 8);
    ASSERT_EQ(sizeof(jw_uint64_t), 8);
    ASSERT_EQ(sizeof(jw_size_t), sizeof(void*));
    ASSERT_EQ(sizeof(jw_ssize_t), sizeof(void*));
}

TEST(float_sizes)
{
    ASSERT_EQ(sizeof(jw_float32_t), 4);
    ASSERT_EQ(sizeof(jw_float64_t), 8);
}

TEST(memory_allocation)
{
    void *ptr = jw_alloc(1024);
    ASSERT_NOT_NULL(ptr);

    memset_pattern(ptr, 0xAB, 1024);

    ptr = jw_realloc(ptr, 2048);
    ASSERT_NOT_NULL(ptr);

    jw_free(ptr);
}

TEST(arena_allocation)
{
    jw_arena_t *arena = NULL;
    jw_status_t status = jw_arena_create(65536, &arena);
    ASSERT_EQ(status, JW_SUCCESS);
    ASSERT_NOT_NULL(arena);

    void *p1 = jw_arena_alloc(arena, 100);
    ASSERT_NOT_NULL(p1);

    void *p2 = jw_arena_alloc(arena, 200);
    ASSERT_NOT_NULL(p2);
    ASSERT_NE(p1, p2);

    ASSERT_TRUE(jw_arena_get_used_size(arena) >= 300);

    jw_arena_destroy(arena);
}

TEST(random_numbers)
{
    jw_srand(12345);

    jw_uint64_t r1 = jw_rand();
    jw_uint64_t r2 = jw_rand();

    ASSERT_NE(r1, r2);

    jw_float32_t f = jw_rand_float();
    ASSERT_TRUE(f >= 0.0f && f < 1.0f);
}

TEST(time_functions)
{
    jw_uint64_t t1 = jw_time_now();
    jw_sleep(10);
    jw_uint64_t t2 = jw_time_now();

    ASSERT_TRUE(t2 >= t1 + 10000);
}

int main(void)
{
    RUN_TEST(version);
    RUN_TEST(integer_sizes);
    RUN_TEST(float_sizes);
    RUN_TEST(memory_allocation);
    RUN_TEST(arena_allocation);
    RUN_TEST(random_numbers);
    RUN_TEST(time_functions);

    return (int)test_failed;
}
