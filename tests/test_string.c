/*
 * test_string.c - JinWo VecDB 字符串操作测试
 */

#include "jw_stdio.h"
#include "jw_string.h"
#include "jw_arena.h"

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
        jw_printf("ASSERT_NE failed: %ld == %ld in %s at line %d\n", (long)(a), (long)(b), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

#define ASSERT_STR_EQ(a, b) ASSERT_TRUE(jw_strcmp(a, b) == 0)

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

TEST(string_basic)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t str1 = { .ptr = "hello", .slen = 5 };
    jw_str_t str2 = { .ptr = "world", .slen = 5 };
    jw_str_t str3 = { .ptr = "hello", .slen = 5 };

    ASSERT_EQ(jw_strlen(&str1), 5);
    ASSERT_EQ(jw_strlen(&str2), 5);
    ASSERT_STR_EQ(&str1, &str3);
    ASSERT_NE(jw_strcmp(&str1, &str2), 0);

    jw_arena_destroy(arena);
}

TEST(string_copy)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t src = { .ptr = "test string", .slen = 11 };
    jw_str_t *dest = jw_strdup(arena, &src);
    ASSERT_NOT_NULL(dest);

    ASSERT_STR_EQ(&src, dest);

    jw_arena_destroy(arena);
}

TEST(string_concat)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t str1 = { .ptr = "hello ", .slen = 6 };
    jw_str_t str2 = { .ptr = "world", .slen = 5 };
    jw_str_t *result = jw_strdup(arena, &str1);
    ASSERT_NOT_NULL(result);

    (void)jw_strcat(result, &str2);
    ASSERT_EQ(jw_strlen(result), 11);

    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Testing string operations...\n\n");

    RUN_TEST(string_basic);
    RUN_TEST(string_copy);
    RUN_TEST(string_concat);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
