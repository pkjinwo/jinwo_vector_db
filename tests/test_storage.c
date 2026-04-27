/*
 * test_storage.c - JinWo VecDB 存储操作测试
 */

#include "jw_stdio.h"
#include "jw_string.h"
#include "jw_storage.h"
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

TEST(storage_basic)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_storage_t *storage = jw_storage_open(arena, "test_storage", JW_STORAGE_CREATE);
    ASSERT_NOT_NULL(storage);

    jw_status_t result = jw_storage_close(storage);
    ASSERT_TRUE(result == JW_SUCCESS);

    jw_arena_destroy(arena);
}

TEST(storage_data)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_storage_t *storage = jw_storage_open(arena, "test_storage_data", JW_STORAGE_CREATE);
    ASSERT_NOT_NULL(storage);

    const char *test_data = "Hello, JinWo VecDB!";
    jw_size_t data_size = 20;
    jw_uint64_t offset = 0;
    jw_status_t status = jw_storage_write(storage, test_data, data_size, &offset);
    ASSERT_TRUE(status == JW_SUCCESS);

    jw_uint8_t buffer[1024] = {0};
    jw_ssize_t read_size = jw_storage_read(storage, 0, buffer, data_size);
    ASSERT_TRUE(read_size > 0);
    ASSERT_EQ((jw_size_t)read_size, data_size);

    status = jw_storage_close(storage);
    ASSERT_TRUE(status == JW_SUCCESS);

    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Testing storage operations...\n\n");

    RUN_TEST(storage_basic);
    RUN_TEST(storage_data);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
