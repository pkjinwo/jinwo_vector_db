/*
 * test_vecdb.c - JinWo VecDB 主接口测试
 */

#include "jw_stdio.h"
#include "jw_string.h"
#include "jw_vecdb.h"
#include "jw_arena.h"
#include "jw_math.h"

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

#define ASSERT_FLOAT_EQ(a, b, tol) do { \
    if (jw_math_abs_f64((a) - (b)) > (tol)) { \
        jw_printf("ASSERT_FLOAT_EQ failed: %f != %f (tolerance %f) in %s at line %d\n", \
                  (double)(a), (double)(b), (double)(tol), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

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

TEST(vecdb_version)
{
    jw_str_t version = jw_vecdb_version();
    ASSERT_NOT_NULL(version.ptr);
    ASSERT_TRUE(version.slen > 0);
    jw_printf("Version: %.*s\n", (int)version.slen, version.ptr);
}

TEST(vecdb_simd)
{
    jw_bool_t simd_avail = jw_vecdb_is_simd_available();
    jw_printf("SIMD available: %s\n", simd_avail ? "yes" : "no");
}

int main(void)
{
    jw_printf("Testing VecDB main interface...\n\n");

    RUN_TEST(vecdb_version);
    RUN_TEST(vecdb_simd);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
