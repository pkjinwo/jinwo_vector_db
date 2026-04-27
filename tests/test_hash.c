/*
 * test_hash.c - JinWo VecDB 哈希函数测试
 */

#include "jw_hash.h"
#include "jw_arena.h"
#include "jw_stdio.h"

static jw_uint32_t test_passed = 0;
static jw_uint32_t test_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    jw_printf("Running %s...\n", #name); \
    test_##name(); \
    test_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        jw_printf("ASSERT_TRUE failed in %s at line %d\n", __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        jw_printf("ASSERT_EQ failed: %ld != %ld in %s at line %d\n", (long)(a), (long)(b), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        jw_printf("ASSERT_NE failed in %s at line %d\n", __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

static jw_arena_t* create_test_arena(void)
{
    jw_arena_t *arena = NULL;
    jw_status_t status = jw_arena_create(4096 * 1024, &arena);
    if (status != JW_SUCCESS || arena == NULL) {
        jw_printf("Failed to create arena: %d\n", status);
        test_failed++;
        return NULL;
    }
    return arena;
}

TEST(hash_string)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t str1 = { .ptr = "hello", .slen = 5 };
    jw_str_t str2 = { .ptr = "world", .slen = 5 };
    jw_str_t str3 = { .ptr = "hello", .slen = 5 };

    jw_uint32_t hash1 = jw_hash_string(str1.ptr);
    jw_uint32_t hash2 = jw_hash_string(str2.ptr);
    jw_uint32_t hash3 = jw_hash_string(str3.ptr);

    ASSERT_EQ(hash1, hash3);
    ASSERT_NE(hash1, hash2);

    jw_arena_destroy(arena);
}

TEST(hash_int)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_int32_t val1 = 42;
    jw_int32_t val2 = -123;
    jw_int32_t val3 = 42;

    jw_uint32_t hash1 = jw_hash_int(val1);
    jw_uint32_t hash2 = jw_hash_int(val2);
    jw_uint32_t hash3 = jw_hash_int(val3);

    ASSERT_EQ(hash1, hash3);
    ASSERT_NE(hash1, hash2);

    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Testing hash operations...\n\n");

    RUN_TEST(hash_string);
    RUN_TEST(hash_int);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
