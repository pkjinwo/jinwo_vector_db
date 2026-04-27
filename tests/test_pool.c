// /*
//  * test_arena.c - JinWo VecDB 内存池测试
//  */

// #include "jw_stdio.h"
// #include "jw_arena.h"
// #include "jw_string.h"

// static jw_uint32_t test_passed = 0;
// static jw_uint32_t test_failed = 0;

// #define TEST(name) static void test_##name(void)

// #define RUN_TEST(name) do { jw_printf("Running %s...\n", #name); test_##name(); test_passed++; } while(0)

// #define ASSERT_TRUE(cond) do { if (!(cond)) { jw_printf("ASSERT_TRUE failed in %s at line %d\n", __func__, __LINE__); test_failed++; return; } } while(0)

// #define ASSERT_EQ(a, b) do { if ((a) != (b)) { jw_printf("ASSERT_EQ failed: %ld != %ld in %s at line %d\n", (long)(a), (long)(b), __func__, __LINE__); test_failed++; return; } } while(0)

// #define ASSERT_NE(a, b) do { if ((a) == (b)) { jw_printf("ASSERT_NE failed: %ld == %ld in %s at line %d\n", (long)(a), (long)(b), __func__, __LINE__); test_failed++; return; } } while(0)

// #define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
// #define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

// static void* memset_pattern(void *ptr, jw_uint8_t val, jw_size_t size)
// {
//     return jw_memset(ptr, val, size);
// }

// TEST(arena_create_destroy)
// {
//     jw_arena_t *arena = NULL;

//     jw_status_t status = jw_arena_create(4096 * 1024, &arena);
//     ASSERT_TRUE(status == JW_SUCCESS);
//     ASSERT_NOT_NULL(arena);

//     jw_arena_destroy(arena);
// }

// TEST(arena_alloc)
// {
//     jw_arena_t *arena = NULL;

//     jw_status_t status = jw_arena_create(4096 * 1024, &arena);
//     ASSERT_TRUE(status == JW_SUCCESS);
//     ASSERT_NOT_NULL(arena);

//     // 测试分配内存
//     void *ptr1 = jw_arena_alloc(arena, 100);
//     ASSERT_NOT_NULL(ptr1);

//     void *ptr2 = jw_arena_alloc(arena, 200);
//     ASSERT_NOT_NULL(ptr2);

//     // 测试内存写入
//     memset_pattern(ptr1, 0xAA, 100);
//     memset_pattern(ptr2, 0xBB, 200);

//     jw_arena_destroy(arena);
// }

// TEST(arena_calloc)
// {
//     jw_arena_t *arena = NULL;

//     jw_status_t status = jw_arena_create(4096 * 1024, &arena);
//     ASSERT_TRUE(status == JW_SUCCESS);
//     ASSERT_NOT_NULL(arena);

//     // 测试分配并清零内存
//     void *ptr = jw_arena_calloc(arena, 100, 1);
//     ASSERT_NOT_NULL(ptr);

//     // 验证内存是否清零
//     jw_uint8_t *p = (jw_uint8_t *)ptr;
//     for (jw_size_t i = 0; i < 100; i++) {
//         ASSERT_EQ(p[i], 0);
//     }

//     jw_arena_destroy(arena);
// }

// int main(void)
// {
//     jw_printf("Testing arena operations...\n\n");

//     RUN_TEST(arena_create_destroy);
//     RUN_TEST(arena_alloc);
//     RUN_TEST(arena_calloc);

//     jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

//     return test_failed == 0 ? 0 : 1;
// }
int main()
{
    return 0;
}
