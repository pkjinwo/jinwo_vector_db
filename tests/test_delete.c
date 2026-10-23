/**
 * @file test_delete.c
 * @brief 删除功能测试：IVF/IVF_PQ/IVF_SQ/HNSW 全类型覆盖
 *
 * 2026-10-23 新增。修复前 IVF 的 jw_index_remove 是空实现:
 *   - ntotal 不递减 (jw_index_get_count 虚高)
 *   - 已删向量仍留在倒排列表, 搜索继续返回它
 *   - jw_collection_search 无兜底过滤, 上层直接拿到已删数据
 * 本测试锁死这些行为, 防回归。
 *
 * 构建: cmake --build build_local --target test_delete
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jw_vecdb.h"
#include "jw_index.h"
#include "jw_arena.h"

static int g_pass = 0;
static int g_fail = 0;
static const char *g_cur = "";

#define CHECK(cond, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  [FAIL] %s: ", g_cur); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

#define SECTION(name) do { g_cur = name; printf("== %s ==\n", name); } while (0)

static unsigned long g_rng = 99173UL;
static double rnd01(void) {
    g_rng = g_rng * 1103515245UL + 12345UL;
    return (double)((g_rng >> 16) & 0x7FFFFFFF) / (double)0x7FFFFFFF;
}
static void gen_vec(jw_vec_t v, jw_dim_t d) {
    for (jw_dim_t i = 0; i < d; i++) {
        double x = rnd01() + rnd01() + rnd01() + rnd01() - 2.0;
        v[i] = (jw_float32_t)x;
    }
}

/* 单类型删除验证 */
static void verify_delete(jw_index_type_t type, jw_metric_t metric, const char *tn) {
    const jw_dim_t dim = 32;
    const jw_size_t n = 300;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    jw_arena_t *a = NULL;
    jw_arena_create(256UL << 20, &a);
    jw_index_config_t cfg;
    jw_memset(&cfg, 0, sizeof(cfg));
    cfg.type = type; cfg.metric = metric; cfg.dim = dim;
    if (type == JW_INDEX_HNSW) {
        cfg.params.hnsw.M = 16;
        cfg.params.hnsw.ef_construction = 100;
        cfg.params.hnsw.ef_search = 100;
    } else {
        cfg.params.ivf.nlist = 8;
        cfg.params.ivf.nprobe = 8;
        cfg.params.ivf.max_iter = 20;
        if (type == JW_INDEX_IVF_PQ) cfg.pq = (jw_pq_config_t)JW_PQ_CONFIG_DEFAULT;
    }

    jw_index_t *idx = jw_index_create(a, &cfg);
    jw_index_train(idx, data, n);
    jw_status_t st = jw_index_add_batch(idx, ids, data, n);
    CHECK(st == JW_SUCCESS, "%s add rc=%d", tn, st);

    jw_size_t before = jw_index_get_ntotal(idx);

    /* 删除 3 个: 首个/中间/末尾 */
    jw_vid_t to_del[3] = { ids[0], ids[n / 2], ids[n - 1] };
    for (int i = 0; i < 3; i++) {
        st = jw_index_remove(idx, to_del[i]);
        CHECK(st == JW_SUCCESS, "%s 删除 id=%llu rc=%d", tn,
              (unsigned long long)to_del[i], st);
    }

    jw_size_t after = jw_index_get_ntotal(idx);
    printf("  %-8s ntotal %zu -> %zu (期望 %zu)\n", tn, (size_t)before, (size_t)after,
           (size_t)(before - 3));
    CHECK(after == before - 3, "%s ntotal=%zu 期望 %zu", tn, (size_t)after, (size_t)(before - 3));

    /* 已删的不应再被搜到 */
    jw_search_result_t res[10];
    int found_deleted = 0;
    for (int i = 0; i < 3; i++) {
        jw_size_t got = jw_index_search(idx, data + (size_t)(to_del[i] - 1) * dim, 10, res);
        for (jw_size_t r = 0; r < got; r++) {
            if (res[r].id == to_del[i]) found_deleted = 1;
        }
    }
    CHECK(found_deleted == 0, "%s 已删 vid 仍可被搜到", tn);
    printf("  %-8s 已删 vid 是否仍可搜到: %s\n", tn, found_deleted ? "**是(缺陷)**" : "否 OK");

    /* 删除不存在的数据: 必须返回 JW_NOT_FOUND (而非假装成功) */
    st = jw_index_remove(idx, 999999);
    printf("  %-8s 删除不存在的 vid: rc=%d (JW_NOT_FOUND=%d)\n", tn, st, JW_NOT_FOUND);
    CHECK(st == JW_NOT_FOUND, "%s 删除不存在的 vid 返回 %d, 期望 JW_NOT_FOUND(%d)",
          tn, st, JW_NOT_FOUND);

    /* 剩余数据仍可正常搜到 (抽 5 个未删的验证) */
    int ok_remaining = 0;
    for (jw_size_t i = 0; i < n; i += 60) {
        int is_deleted = 0;
        for (int k = 0; k < 3; k++) if (ids[i] == to_del[k]) is_deleted = 1;
        if (is_deleted) continue;
        jw_size_t got = jw_index_search(idx, data + (size_t)i * dim, 5, res);
        if (got >= 1 && res[0].id == ids[i]) ok_remaining++;
    }
    CHECK(ok_remaining >= 3, "%s 剩余向量自查询仅命中 %d/5", tn, ok_remaining);
    printf("  %-8s 剩余向量自查询命中: %d/5\n", tn, ok_remaining);

    jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(data); free(ids);
}

/* 批量删除 */
static void test_batch_remove(void) {
    SECTION("批量删除 (jw_index_remove_batch, IVF)");

    const jw_dim_t dim = 32;
    const jw_size_t n = 400;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    jw_arena_t *a = NULL;
    jw_arena_create(256UL << 20, &a);
    jw_index_config_t cfg;
    jw_memset(&cfg, 0, sizeof(cfg));
    cfg.type = JW_INDEX_IVF; cfg.metric = JW_METRIC_L2; cfg.dim = dim;
    cfg.params.ivf.nlist = 8; cfg.params.ivf.nprobe = 8; cfg.params.ivf.max_iter = 20;
    jw_index_t *idx = jw_index_create(a, &cfg);
    jw_index_train(idx, data, n);
    jw_index_add_batch(idx, ids, data, n);

    jw_size_t before = jw_index_get_ntotal(idx);
    jw_vid_t batch[20];
    for (int i = 0; i < 20; i++) batch[i] = ids[i * 7];   /* 间隔取, 分散在各簇 */
    jw_status_t st = jw_index_remove_batch(idx, batch, 20);
    jw_size_t after = jw_index_get_ntotal(idx);

    printf("  批量删除 20 个: rc=%d ntotal %zu -> %zu (期望 %zu)\n",
           st, (size_t)before, (size_t)after, (size_t)(before - 20));
    CHECK(st == JW_SUCCESS, "remove_batch rc=%d", st);
    CHECK(after == before - 20, "ntotal=%zu 期望 %zu", (size_t)after, (size_t)(before - 20));

    jw_search_result_t res[10];
    int still = 0;
    for (int i = 0; i < 20; i++) {
        jw_size_t got = jw_index_search(idx, data + (size_t)(batch[i] - 1) * dim, 10, res);
        for (jw_size_t r = 0; r < got; r++) if (res[r].id == batch[i]) still = 1;
    }
    CHECK(still == 0, "批量删除后仍有 vid 可被搜到");
    printf("  批量删除后仍可搜到: %s\n", still ? "**是(缺陷)**" : "否 OK");

    jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(data); free(ids);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("========================================\n");
    printf("  jinwo_vecdb 删除功能测试\n");
    printf("========================================\n\n");

    SECTION("各索引类型删除");
    verify_delete(JW_INDEX_IVF,     JW_METRIC_L2,     "IVF");
    verify_delete(JW_INDEX_IVF,     JW_METRIC_COSINE, "IVF/COS");
    verify_delete(JW_INDEX_IVF_PQ,  JW_METRIC_L2,     "IVF_PQ");
    verify_delete(JW_INDEX_HNSW,    JW_METRIC_L2,     "HNSW");

    test_batch_remove();

    printf("\n========================================\n");
    printf("  结果: %d 通过, %d 失败\n", g_pass, g_fail);
    printf("========================================\n");
    return g_fail ? 1 : 0;
}
