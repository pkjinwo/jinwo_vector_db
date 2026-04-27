/*
 * test_sort.c - JinWo VecDB 排序函数测试
 */

#include "jw_sort.h"
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

static jw_int32_t int_compare(const void *a, const void *b, void *user_data)
{
    (void)user_data;
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    if (ia < ib) return -1;
    if (ia > ib) return 1;
    return 0;
}

TEST(sort_basic)
{
    int arr[] = {5, 2, 9, 1, 5, 6};
    jw_size_t n = sizeof(arr) / sizeof(arr[0]);

    jw_qsort(arr, n, sizeof(int), int_compare, NULL);

    for (jw_size_t i = 1; i < n; i++) {
        ASSERT_TRUE(arr[i-1] <= arr[i]);
    }
}

TEST(sort_single)
{
    int arr[] = {42};
    jw_size_t n = 1;

    jw_qsort(arr, n, sizeof(int), int_compare, NULL);
    ASSERT_EQ(arr[0], 42);
}

int main(void)
{
    jw_printf("Testing sort operations...\n\n");

    RUN_TEST(sort_basic);
    RUN_TEST(sort_single);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
