/*
 * test_math.c - JinWo VecDB 数学函数测试
 */

#include "jw_math.h"
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
        jw_printf("ASSERT_NE failed: %ld == %ld in %s at line %d\n", (long)(a), (long)(b), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (jw_math_abs_f32((a) - (b)) > 1e-6f) { \
        jw_printf("ASSERT_FLOAT_EQ failed: %f != %f in %s at line %d\n", (double)(a), (double)(b), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_DOUBLE_EQ(a, b) do { \
    if (jw_math_abs_f64((a) - (b)) > 1e-6) { \
        jw_printf("ASSERT_DOUBLE_EQ failed: %lf != %lf in %s at line %d\n", (a), (b), __func__, __LINE__); \
        test_failed++; \
        return; \
    } \
} while(0)

TEST(math_abs)
{
    ASSERT_FLOAT_EQ(jw_math_abs_f32(5.0f), 5.0f);
    ASSERT_FLOAT_EQ(jw_math_abs_f32(-5.0f), 5.0f);
    ASSERT_FLOAT_EQ(jw_math_abs_f32(0.0f), 0.0f);

    ASSERT_DOUBLE_EQ(jw_math_abs_f64(5.0), 5.0);
    ASSERT_DOUBLE_EQ(jw_math_abs_f64(-5.0), 5.0);
    ASSERT_DOUBLE_EQ(jw_math_abs_f64(0.0), 0.0);
}

TEST(math_sqrt)
{
    ASSERT_FLOAT_EQ(jw_math_sqrt_f32(4.0f), 2.0f);
    ASSERT_FLOAT_EQ(jw_math_sqrt_f32(9.0f), 3.0f);
    ASSERT_FLOAT_EQ(jw_math_sqrt_f32(0.0f), 0.0f);

    ASSERT_DOUBLE_EQ(jw_math_sqrt_f64(4.0), 2.0);
    ASSERT_DOUBLE_EQ(jw_math_sqrt_f64(9.0), 3.0);
    ASSERT_DOUBLE_EQ(jw_math_sqrt_f64(0.0), 0.0);
}

TEST(math_log)
{
    ASSERT_FLOAT_EQ(jw_math_log_f32(1.0f), 0.0f);

    ASSERT_DOUBLE_EQ(jw_math_log_f64(1.0), 0.0);
}

TEST(math_clamp)
{
    ASSERT_FLOAT_EQ(jw_math_clamp_f32(5.0f, 0.0f, 10.0f), 5.0f);
    ASSERT_FLOAT_EQ(jw_math_clamp_f32(-5.0f, 0.0f, 10.0f), 0.0f);
    ASSERT_FLOAT_EQ(jw_math_clamp_f32(15.0f, 0.0f, 10.0f), 10.0f);

    ASSERT_DOUBLE_EQ(jw_math_clamp_f64(5.0, 0.0, 10.0), 5.0);
    ASSERT_DOUBLE_EQ(jw_math_clamp_f64(-5.0, 0.0, 10.0), 0.0);
    ASSERT_DOUBLE_EQ(jw_math_clamp_f64(15.0, 0.0, 10.0), 10.0);
}

int main(void)
{
    jw_printf("Testing math operations...\n\n");

    RUN_TEST(math_abs);
    RUN_TEST(math_sqrt);
    RUN_TEST(math_log);
    RUN_TEST(math_clamp);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
