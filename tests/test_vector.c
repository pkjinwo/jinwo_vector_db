/*
 * test_vector.c - JinWo VecDB 向量操作测试
 */

#include "jw_stdio.h"
#include "jw_vector.h"
#include "jw_arena.h"
#include "jw_math.h"

static jw_uint32_t test_passed = 0;
static jw_uint32_t test_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    jw_printf("Running test_%s... ", #name); \
    test_##name(); \
    jw_printf("PASS\n"); \
    test_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s\n", #cond); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s == %s\n", #a, #b); \
        jw_printf("  Expected: %d, Actual: %d\n", (int)(a), (int)(b)); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s != %s\n", #a, #b); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (jw_math_abs_f32((a) - (b)) > 1e-6f) { \
        jw_printf("FAIL\n"); \
        jw_printf("  Assertion failed: %s == %s\n", #a, #b); \
        jw_printf("  Expected: %f, Actual: %f\n", (double)(a), (double)(b)); \
        jw_printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
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

TEST(dot_product)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_dim_t dim = 3;
    jw_vec_t a = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    jw_vec_t b = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));

    a[0] = 1.0f;
    a[1] = 2.0f;
    a[2] = 3.0f;
    b[0] = 4.0f;
    b[1] = 5.0f;
    b[2] = 6.0f;

    jw_float32_t result = jw_vec_dot(a, b, dim);
    ASSERT_FLOAT_EQ(result, 32.0f);

    jw_arena_destroy(arena);
}

TEST(l2_squared_distance)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_dim_t dim = 3;
    jw_vec_t a = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    jw_vec_t b = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));

    a[0] = 1.0f;
    a[1] = 2.0f;
    a[2] = 3.0f;
    b[0] = 4.0f;
    b[1] = 5.0f;
    b[2] = 6.0f;

    jw_float32_t result = jw_vec_l2_squared(a, b, dim);
    ASSERT_FLOAT_EQ(result, 27.0f);

    jw_arena_destroy(arena);
}

TEST(cosine_similarity)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_dim_t dim = 3;
    jw_vec_t a = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
    jw_vec_t b = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));

    a[0] = 1.0f;
    a[1] = 2.0f;
    a[2] = 3.0f;
    b[0] = 4.0f;
    b[1] = 5.0f;
    b[2] = 6.0f;

    jw_float32_t result = jw_vec_cosine_similarity(a, b, dim);
    ASSERT_FLOAT_EQ(result, 0.97463185f);

    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Testing vector operations...\n\n");

    RUN_TEST(dot_product);
    RUN_TEST(l2_squared_distance);
    RUN_TEST(cosine_similarity);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
