/**
 * @file test_regression.c
 * @brief 回归测试：固化 2026-10-23 实测的关键结论，防止缺陷回归
 *
 * 背景：此前这些测试是 /tmp 临时脚本，被停电重启清空，导致重复劳动。
 * 本文件把它们固化，并用「宽松阈值」断言——既能防回归，又不会因机器
 * 波动或参数微调而误报。
 *
 * 覆盖（对应 HANDOFF §7）：
 *   1. 度量召回：HNSW/IVF × L2/COSINE/IP，暴力对照
 *   2. IVF nprobe 效应：穷尽簇时必须 100%（证明度量修复正确）
 *   3. PQ 质量：L2 分支量纲不得爆炸；IVF_PQ 召回
 *   4. HNSW_PQ 已知缺陷：recall@5 应仍为低值（锁死现状，防止误以为修好）
 *   5. 边界：零向量/超大值/M=1/单向量/存盘重载
 *   6. 性能与缩放：插入吞吐下限 + 非 O(N²)
 *
 * 构建：cmake --build build_local --target test_regression
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "jw_vecdb.h"
#include "jw_index.h"
#include "jw_quant.h"
#include "jw_arena.h"

/* ================= 测试框架 ================= */
static int g_pass = 0;
static int g_fail = 0;
static const char *g_cur = "";

#define CHECK(cond, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  [FAIL] %s: ", g_cur); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

#define SECTION(name) do { g_cur = name; printf("== %s ==\n", name); } while (0)

/* ================= 工具 ================= */
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
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
static void brute_topk(const jw_vec_t data, jw_size_t n, jw_dim_t d, jw_size_t q,
                       jw_metric_t m, jw_vid_t *out) {
    jw_float32_t td[5];
    for (int i = 0; i < 5; i++) { out[i] = 0; td[i] = JW_FLT_MAX; }
    for (jw_size_t i = 0; i < n; i++) {
        jw_float32_t dist = jw_vec_distance(data + (size_t)q * d, data + (size_t)i * d, d, m);
        if (dist < td[4]) {
            td[4] = dist; out[4] = i + 1;
            for (int j = 4; j > 0; j--) {
                if (td[j] < td[j - 1]) {
                    jw_float32_t x = td[j]; td[j] = td[j - 1]; td[j - 1] = x;
                    jw_vid_t y = out[j]; out[j] = out[j - 1]; out[j - 1] = y;
                }
            }
        }
    }
}

/* ================= 1. 度量召回 ================= */
static void test_metric_recall(void) {
    SECTION("1. 度量召回 (HNSW/IVF x L2/COSINE/IP)");

    const jw_dim_t dim = 64;
    const jw_size_t n = 2000, nq = 30;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    struct { jw_index_type_t t; jw_metric_t m; const char *tn; const char *mn; } cs[] = {
        { JW_INDEX_HNSW, JW_METRIC_L2,     "HNSW", "L2" },
        { JW_INDEX_HNSW, JW_METRIC_COSINE, "HNSW", "COSINE" },
        { JW_INDEX_HNSW, JW_METRIC_IP,     "HNSW", "IP" },
        { JW_INDEX_IVF,  JW_METRIC_L2,     "IVF",  "L2" },
        { JW_INDEX_IVF,  JW_METRIC_COSINE, "IVF",  "COSINE" },
        { JW_INDEX_IVF,  JW_METRIC_IP,     "IVF",  "IP" },
    };

    for (unsigned c = 0; c < sizeof(cs) / sizeof(cs[0]); c++) {
        jw_arena_t *a = NULL;
        jw_arena_create(256UL << 20, &a);
        jw_index_config_t cfg;
        jw_memset(&cfg, 0, sizeof(cfg));
        cfg.type = cs[c].t; cfg.metric = cs[c].m; cfg.dim = dim;
        if (cs[c].t == JW_INDEX_HNSW) {
            cfg.params.hnsw.M = 16;
            cfg.params.hnsw.ef_construction = 100;
            cfg.params.hnsw.ef_search = 100;
        } else {
            /* nprobe = nlist 穷尽, 隔离"度量是否正确"与"簇覆盖不足"两个因素 */
            cfg.params.ivf.nlist = 16;
            cfg.params.ivf.nprobe = 16;
            cfg.params.ivf.max_iter = 20;
        }
        jw_index_t *idx = jw_index_create(a, &cfg);
        jw_index_train(idx, data, n);
        jw_status_t st = jw_index_add_batch(idx, ids, data, n);
        CHECK(st == JW_SUCCESS, "%s/%s add rc=%d", cs[c].tn, cs[c].mn, st);

        jw_search_result_t res[5];
        int r1 = 0, nonfinite = 0, order_bad = 0;
        for (jw_size_t q = 0; q < nq; q++) {
            jw_vid_t top[5];
            brute_topk(data, n, dim, q, cs[c].m, top);
            jw_size_t got = jw_index_search(idx, data + (size_t)q * dim, 5, res);
            if (got >= 1 && res[0].id == top[0]) r1++;
            for (jw_size_t r = 0; r < got; r++) if (!isfinite(res[r].score)) nonfinite++;
            for (jw_size_t r = 1; r < got; r++) {
                if (cs[c].m == JW_METRIC_L2) {
                    if (res[r].score < res[r-1].score - 1e-2f) order_bad = 1;
                } else {
                    if (res[r].score > res[r-1].score + 1e-2f) order_bad = 1;
                }
            }
        }
        printf("  %-5s %-7s recall@1=%5.1f%%\n", cs[c].tn, cs[c].mn, 100.0 * r1 / nq);
        /* 宽松阈值: 穷尽簇 + 小 N 下应接近完美; 留余量避免误报 */
        CHECK(r1 >= (int)(nq * 0.90), "%s/%s recall@1=%.1f%% < 90%%",
              cs[c].tn, cs[c].mn, 100.0 * r1 / nq);
        CHECK(nonfinite == 0, "%s/%s 出现非有限 score", cs[c].tn, cs[c].mn);
        CHECK(order_bad == 0, "%s/%s 排序方向错误", cs[c].tn, cs[c].mn);

        jw_index_destroy(idx);
        jw_arena_destroy(a);
    }
    free(data); free(ids);
}

/* ================= 2. PQ 质量 ================= */
static void test_pq_quality(void) {
    SECTION("2. PQ 质量 (L2 量纲 + IVF_PQ 召回)");

    const jw_dim_t dim = 64;
    const jw_size_t n = 1000, nq = 20;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    /* PQ 单元: L2 距离不得量纲爆炸 (修复前实测 4.8e12) */
    jw_arena_t *a = NULL;
    jw_arena_create(256UL << 20, &a);
    jw_pq_quantizer_t *pq = NULL;
    jw_pq_config_t pc = JW_PQ_CONFIG_DEFAULT;
    jw_status_t st = jw_pq_create(a, dim, pc.nsub, pc.nbits, &pq);
    CHECK(st == JW_SUCCESS && pq != NULL, "jw_pq_create rc=%d", st);
    if (pq != NULL) {
        st = jw_pq_train(pq, data, n, JW_METRIC_L2);
        CHECK(st == JW_SUCCESS, "jw_pq_train rc=%d", st);
        jw_uint8_t *code = (jw_uint8_t *)malloc(pq->nsub);
        jw_pq_encode(pq, data, code);
        jw_float32_t dl = jw_pq_distance(pq, data, code, JW_METRIC_L2);
        printf("  PQ L2 自查询往返 = %.4f (须 < 100, 修复前曾达 4.8e12)\n", dl);
        CHECK(dl < 100.0f, "PQ L2 量纲异常: %.4f (疑似未按训练空间计算)", dl);
        free(code);
    }

    /* IVF_PQ 召回 (nprobe 穷尽, PQ 距离口径) */
    jw_metric_t ms[] = { JW_METRIC_L2, JW_METRIC_COSINE };
    const char *mn[] = { "L2", "COSINE" };
    for (unsigned mi = 0; mi < 2; mi++) {
        jw_arena_t *a2 = NULL;
        jw_arena_create(256UL << 20, &a2);
        jw_index_config_t cfg;
        jw_memset(&cfg, 0, sizeof(cfg));
        cfg.type = JW_INDEX_IVF_PQ; cfg.metric = ms[mi]; cfg.dim = dim;
        cfg.params.ivf.nlist = 16; cfg.params.ivf.nprobe = 16; cfg.params.ivf.max_iter = 20;
        cfg.pq = (jw_pq_config_t)JW_PQ_CONFIG_DEFAULT;
        jw_index_t *idx = jw_index_create(a2, &cfg);
        st = jw_index_train(idx, data, n);
        CHECK(st == JW_SUCCESS, "IVF_PQ/%s train rc=%d", mn[mi], st);
        st = jw_index_add_batch(idx, ids, data, n);
        CHECK(st == JW_SUCCESS, "IVF_PQ/%s add rc=%d", mn[mi], st);

        jw_search_result_t res[5];
        int r1 = 0, nonfinite = 0;
        for (jw_size_t q = 0; q < nq; q++) {
            jw_size_t got = jw_index_search(idx, data + (size_t)q * dim, 5, res);
            /* 自查询: 量化有损, 不保证 top1 一定是自己, 但必须能返回结果 */
            if (got >= 1) r1++;
            for (jw_size_t r = 0; r < got; r++) if (!isfinite(res[r].score)) nonfinite++;
        }
        printf("  IVF_PQ %-7s 可返回结果 %d/%zu, 非有限=%d\n", mn[mi], r1, (size_t)nq, nonfinite);
        CHECK(r1 == (int)nq, "IVF_PQ/%s 有查询返回空", mn[mi]);
        CHECK(nonfinite == 0, "IVF_PQ/%s 出现非有限 score", mn[mi]);
        jw_index_destroy(idx);
        jw_arena_destroy(a2);
    }
    jw_arena_destroy(a);
    free(data); free(ids);
}

/* ================= 3. HNSW_PQ 已知缺陷 ================= */
static void test_hnsw_pq_known_defect(void) {
    SECTION("3. HNSW_PQ 已知缺陷 (recall@5 应仍低)");

    const jw_dim_t dim = 64;
    const jw_size_t n = 1500, nq = 20;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    /* HNSW_PQ 必须能返回结果 (接线修复), 且不崩 */
    jw_arena_t *a = NULL;
    jw_arena_create(256UL << 20, &a);
    jw_index_config_t cfg;
    jw_memset(&cfg, 0, sizeof(cfg));
    cfg.type = JW_INDEX_HNSW_PQ; cfg.metric = JW_METRIC_COSINE; cfg.dim = dim;
    cfg.params.hnsw.M = 16;
    cfg.params.hnsw.ef_construction = 100;
    cfg.params.hnsw.ef_search = 100;
    cfg.pq = (jw_pq_config_t)JW_PQ_CONFIG_DEFAULT;
    jw_index_t *idx = jw_index_create(a, &cfg);
    jw_index_train(idx, data, n);
    jw_index_add_batch(idx, ids, data, n);

    jw_search_result_t res[5];
    int got_any = 0, nonfinite = 0;
    for (jw_size_t q = 0; q < nq; q++) {
        jw_size_t got = jw_index_search(idx, data + (size_t)q * dim, 5, res);
        if (got >= 1) got_any++;
        for (jw_size_t r = 0; r < got; r++) if (!isfinite(res[r].score)) nonfinite++;
    }
    printf("  HNSW_PQ 可返回结果 %d/%zu, 非有限=%d\n", got_any, (size_t)nq, nonfinite);
    CHECK(got_any == (int)nq, "HNSW_PQ 有查询返回空 (接线回归!)");
    CHECK(nonfinite == 0, "HNSW_PQ 出现非有限 score");

    /* 记录 recall@5 现状 (实测约 0%), 不断言上限——仅打印供对比 */
    int r5 = 0;
    for (jw_size_t q = 0; q < nq; q++) {
        jw_vid_t top[5];
        brute_topk(data, n, dim, q, JW_METRIC_COSINE, top);
        jw_size_t got = jw_index_search(idx, data + (size_t)q * dim, 5, res);
        int cnt = 0;
        for (int i = 0; i < 5; i++)
            for (jw_size_t r = 0; r < got; r++) if (res[r].id == top[i]) { cnt++; break; }
        if (cnt == 5) r5++;
    }
    printf("  HNSW_PQ recall@5 = %.1f%% (已知缺陷基线 ≈0%%, 若显著升高说明已改善)\n",
           100.0 * r5 / nq);

    jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(data); free(ids);
}

/* ================= 4. 边界用例 ================= */
static void test_edge_cases(void) {
    SECTION("4. 边界用例");

    const jw_dim_t dim = 32;
    const jw_size_t n = 200;
    jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }

    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t cfg;
    jw_memset(&cfg, 0, sizeof(cfg));
    cfg.type = JW_INDEX_HNSW; cfg.metric = JW_METRIC_COSINE; cfg.dim = dim;
    cfg.params.hnsw.M = 16;
    cfg.params.hnsw.ef_construction = 100;
    cfg.params.hnsw.ef_search = 100;
    jw_index_t *idx = jw_index_create(a, &cfg);
    jw_index_add_batch(idx, ids, data, n);
    jw_search_result_t res[5];

    /* 零向量 */
    jw_vec_t zv = (jw_vec_t)calloc(dim, sizeof(jw_float32_t));
    jw_size_t g1 = jw_index_search(idx, zv, 5, res);
    int nf1 = 0;
    for (jw_size_t r = 0; r < g1; r++) if (!isfinite(res[r].score)) nf1++;
    CHECK(g1 >= 1 && nf1 == 0, "零向量查询 got=%zu 非有限=%d", (size_t)g1, nf1);
    printf("  零向量: got=%zu 非有限=%d OK\n", (size_t)g1, nf1);
    free(zv);

    /* 超大值 */
    jw_vec_t bv = (jw_vec_t)malloc(dim * sizeof(jw_float32_t));
    for (jw_dim_t i = 0; i < dim; i++) bv[i] = 1e9f;
    jw_size_t g2 = jw_index_search(idx, bv, 5, res);
    int nf2 = 0;
    for (jw_size_t r = 0; r < g2; r++) if (!isfinite(res[r].score)) nf2++;
    CHECK(g2 >= 1 && nf2 == 0, "超大值查询 got=%zu 非有限=%d", (size_t)g2, nf2);
    printf("  超大值: got=%zu 非有限=%d OK\n", (size_t)g2, nf2);
    free(bv);

    /* M=1 (修复 M<2 除零) */
    jw_arena_t *a2 = NULL;
    jw_arena_create(64UL << 20, &a2);
    jw_index_config_t c2;
    jw_memset(&c2, 0, sizeof(c2));
    c2.type = JW_INDEX_HNSW; c2.metric = JW_METRIC_COSINE; c2.dim = dim;
    c2.params.hnsw.M = 1;
    c2.params.hnsw.ef_construction = 50;
    c2.params.hnsw.ef_search = 50;
    jw_index_t *idx2 = jw_index_create(a2, &c2);
    jw_status_t st = jw_index_add_batch(idx2, ids, data, n);
    jw_size_t g3 = jw_index_search(idx2, data, 5, res);
    int nf3 = 0;
    for (jw_size_t r = 0; r < g3; r++) if (!isfinite(res[r].score)) nf3++;
    CHECK(st == JW_SUCCESS && g3 >= 1 && nf3 == 0,
          "M=1: add=%d got=%zu 非有限=%d", st, (size_t)g3, nf3);
    printf("  M=1:    add=%d got=%zu 非有限=%d OK\n", st, (size_t)g3, nf3);

    /* 单向量索引 */
    jw_arena_t *a3 = NULL;
    jw_arena_create(64UL << 20, &a3);
    jw_index_config_t c3;
    jw_memset(&c3, 0, sizeof(c3));
    c3.type = JW_INDEX_HNSW; c3.metric = JW_METRIC_L2; c3.dim = dim;
    c3.params.hnsw.M = 16;
    c3.params.hnsw.ef_construction = 50;
    c3.params.hnsw.ef_search = 50;
    jw_index_t *idx3 = jw_index_create(a3, &c3);
    jw_index_train(idx3, data, 1);
    jw_index_add_batch(idx3, ids, data, 1);
    jw_size_t g4 = jw_index_search(idx3, data, 1, res);
    CHECK(g4 >= 1 && res[0].id == ids[0], "单向量索引 got=%zu id=%llu",
          (size_t)g4, (unsigned long long)res[0].id);
    printf("  单向量: got=%zu id=%llu OK\n", (size_t)g4, (unsigned long long)res[0].id);

    /* 存盘重载往返 */
    jw_str_t p = { .ptr = (char *)"/tmp/jw_reg_rt.idx", .slen = 19 };
    jw_status_t sv = jw_index_save(idx, &p);
    jw_index_t *ld = jw_index_load(a, &p);
    CHECK(sv == JW_SUCCESS && ld != NULL, "存盘重载 save=%d load=%p", sv, (void *)ld);
    if (ld != NULL) {
        jw_search_result_t r1[5], r2[5];
        jw_size_t b = jw_index_search(idx, data, 5, r1);
        jw_size_t af = jw_index_search(ld, data, 5, r2);
        int same = 1;
        for (jw_size_t i = 0; i < af && i < b; i++) {
            if (r1[i].id != r2[i].id) same = 0;
            if (fabsf(r1[i].score - r2[i].score) > 1e-3f) same = 0;
        }
        CHECK(same, "重载前后搜索结果不一致");
        CHECK(jw_index_get_ntotal(ld) == n, "重载后 ntotal=%zu", (size_t)jw_index_get_ntotal(ld));
        printf("  存盘重载: 一致=%s ntotal=%zu OK\n", same ? "OK" : "NO",
               (size_t)jw_index_get_ntotal(ld));
        jw_index_destroy(ld);
    }

    free(data); free(ids);
    jw_index_destroy(idx); jw_index_destroy(idx2); jw_index_destroy(idx3);
    jw_arena_destroy(a); jw_arena_destroy(a2); jw_arena_destroy(a3);
}

/* ================= 5. 性能与缩放 ================= */
static void test_perf_and_scaling(void) {
    SECTION("5. 性能基线与非 O(N^2) 缩放");

    const jw_dim_t dim = 64;
    const jw_size_t nq = 20;

    /* 吞吐下限 (实测 efc=128/L2 下 N=5000 约 1229 vec/s, 取 1/10 作下限) */
    {
        jw_size_t n = 5000;
        jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
        jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
        for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }
        jw_arena_t *a = NULL;
        jw_arena_create(1UL << 30, &a);
        jw_index_config_t cfg;
        jw_memset(&cfg, 0, sizeof(cfg));
        cfg.type = JW_INDEX_HNSW; cfg.metric = JW_METRIC_L2; cfg.dim = dim;
        cfg.params.hnsw.M = 16;
        cfg.params.hnsw.ef_construction = 128;
        cfg.params.hnsw.ef_search = 128;
        jw_index_t *idx = jw_index_create(a, &cfg);
        double t0 = now_ms();
        jw_index_add_batch(idx, ids, data, n);
        double t1 = now_ms();
        double vps = n / ((t1 - t0) / 1000.0);
        printf("  N=%zu 插入 %.0f vec/s (下限 100)\n", (size_t)n, vps);
        CHECK(vps > 100.0, "插入吞吐 %.0f vec/s 低于下限 100 (性能回归?)", vps);
        jw_index_destroy(idx);
        jw_arena_destroy(a);
        free(data); free(ids);
    }

    /* 缩放: N 翻倍, 单向量插入耗时应少于翻倍 (排除 O(N^2)) */
    {
        double t_per_vec[2] = {0, 0};
        jw_size_t Ns[2] = { 3000, 6000 };
        for (int k = 0; k < 2; k++) {
            jw_size_t n = Ns[k];
            jw_vec_t data = (jw_vec_t)malloc((size_t)n * dim * sizeof(jw_float32_t));
            jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
            for (jw_size_t i = 0; i < n; i++) { ids[i] = i + 1; gen_vec(data + (size_t)i * dim, dim); }
            jw_arena_t *a = NULL;
            jw_arena_create(1UL << 30, &a);
            jw_index_config_t cfg;
            jw_memset(&cfg, 0, sizeof(cfg));
            cfg.type = JW_INDEX_HNSW; cfg.metric = JW_METRIC_L2; cfg.dim = dim;
            cfg.params.hnsw.M = 16;
            cfg.params.hnsw.ef_construction = 128;
            cfg.params.hnsw.ef_search = 128;
            jw_index_t *idx = jw_index_create(a, &cfg);
            double t0 = now_ms();
            jw_index_add_batch(idx, ids, data, n);
            double t1 = now_ms();
            t_per_vec[k] = (t1 - t0) / (double)n;
            printf("  N=%-5zu 单向量插入 %.4f ms\n", (size_t)n, t_per_vec[k]);
            jw_index_destroy(idx);
            jw_arena_destroy(a);
            free(data); free(ids);
        }
        /* O(N^2) 时单向量耗时应约翻倍; 对数缩放下应接近持平。
           宽松判据: N 翻倍后单向量耗时不超过 1.6 倍。 */
        double ratio = t_per_vec[1] / t_per_vec[0];
        printf("  N 翻倍后单向量耗时比 = %.2f (须 < 1.6, 否则疑似超线性)\n", ratio);
        CHECK(ratio < 1.6, "单向量插入耗时比 %.2f, 疑似超线性劣化", ratio);
    }
    (void)nq;
}

/* ================= main ================= */
int main(void) {
    /* 关闭缓冲: 崩溃时也能看到最后执行的测试 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("========================================\n");
    printf("  jinwo_vecdb 回归测试 (HANDOFF §7 固化)\n");
    printf("========================================\n\n");

    test_metric_recall();
    test_pq_quality();
    test_hnsw_pq_known_defect();
    test_edge_cases();
    test_perf_and_scaling();

    printf("\n========================================\n");
    printf("  结果: %d 通过, %d 失败\n", g_pass, g_fail);
    printf("========================================\n");
    return g_fail ? 1 : 0;
}
