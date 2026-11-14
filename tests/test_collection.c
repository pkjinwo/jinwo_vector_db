/*
 * test_collection.c - JinWo VecDB Collection测试
 */

#include "jw_stdio.h"
#include "jw_string.h"
#include "jw_collection.h"
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

static jw_size_t collection_count(jw_collection_t *coll)
{
    jw_collection_stats_t stats;
    if (jw_collection_get_stats(coll, &stats) != JW_SUCCESS) {
        return (jw_size_t)-1;
    }
    return stats.count;
}

TEST(collection_placeholder)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);
    jw_arena_destroy(arena);
}

/* 2026-11-14 (HANDOFF §15): upsert 不存在的 vid 时必须落到调用者指定的 vid,
 * 修复前走自动分配, 指定 vid 被静默丢弃 */
TEST(upsert_specified_vid)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_collection_config_t config = JW_COLLECTION_CONFIG_DEFAULT;
    config.dimension = 16;
    char coll_name_buf[32];
    jw_snprintf(coll_name_buf, sizeof(coll_name_buf), "upsert_test");
    JW_STR_SET(config.name, coll_name_buf);

    jw_collection_t *coll = jw_collection_create(arena, &config);
    ASSERT_NOT_NULL(coll);

    /* 1. upsert 新 vid=42 → 必须落到 vid=42 */
    jw_float32_t vec1[16];
    for (int j = 0; j < 16; j++) vec1[j] = 1.5f;
    ASSERT_EQ(jw_collection_upsert(coll, 42, vec1), JW_SUCCESS);
    ASSERT_EQ(collection_count(coll), 1);

    jw_float32_t got[16];
    ASSERT_EQ(jw_collection_get(coll, 42, got), JW_SUCCESS);
    ASSERT_FLOAT_EQ(got[0], 1.5f, 1e-5);
    ASSERT_FLOAT_EQ(got[15], 1.5f, 1e-5);

    /* 2. upsert 同一 vid → 原地更新, count 不变 */
    jw_float32_t vec2[16];
    for (int j = 0; j < 16; j++) vec2[j] = 2.5f;
    ASSERT_EQ(jw_collection_upsert(coll, 42, vec2), JW_SUCCESS);
    ASSERT_EQ(collection_count(coll), 1);
    ASSERT_EQ(jw_collection_get(coll, 42, got), JW_SUCCESS);
    ASSERT_FLOAT_EQ(got[0], 2.5f, 1e-5);

    /* 3. 稀疏 vid=100000 → 落到 100000, 且后续自动分配 vid 顺延 (next_vid 下推) */
    jw_float32_t vec3[16];
    for (int j = 0; j < 16; j++) vec3[j] = 3.5f;
    ASSERT_EQ(jw_collection_upsert(coll, 100000, vec3), JW_SUCCESS);
    ASSERT_EQ(collection_count(coll), 2);
    ASSERT_EQ(jw_collection_get(coll, 100000, got), JW_SUCCESS);
    ASSERT_FLOAT_EQ(got[0], 3.5f, 1e-5);

    jw_vid_t vid = 0;
    ASSERT_EQ(jw_collection_insert(coll, vec1, &vid), JW_SUCCESS);
    ASSERT_EQ(vid, 100001);

    /* 4. vid=0 是无效 ID, 必须拒绝 */
    ASSERT_EQ(jw_collection_upsert(coll, 0, vec1), JW_INVALID_PARAM);

    jw_collection_destroy(coll);
    jw_arena_destroy(arena);
}

int main(void)
{
    jw_printf("Testing collection operations...\n\n");

    RUN_TEST(collection_placeholder);
    RUN_TEST(upsert_specified_vid);

    jw_printf("\nTest results: %d passed, %d failed\n", test_passed, test_failed);

    return test_failed == 0 ? 0 : 1;
}
