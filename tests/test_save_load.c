/**
 * @file test_save_load.c
 * @brief 存盘/加载与 vid 健壮性回归测试
 *
 * 2026-10-23 新增。锁死以下四个缺陷, 防回归:
 *
 * 1) 【数据丢失】序列化按 `ntotal` 遍历 nodes 数组槽位, 但 nodes 是**按 vid
 *    索引**的。vid 稀疏时 (如 100..1000) 遍历 i<ntotal 一个节点都碰不到,
 *    文件里 0 条节点记录, 重载后索引是空的 —— 且不报错, 静默丢光。
 *
 * 2) 【越界写/段错误】加载时直接把文件里的 capacity 赋给 hnsw->capacity,
 *    但 nodes 数组是 jw_index_create 按初始容量分配的。vid 落在
 *    (实际容量, 文件声明容量) 区间时, `vid >= capacity` 判断失效,
 *    `nodes[vid] = node` 越界写 -> SIGSEGV。可被构造文件触发。
 *
 * 3) 【坏索引外泄】jw_index_load 的 cleanup 无条件 `return index`,
 *    文件截断/损坏时返回**部分初始化的坏索引** (非 NULL)。调用方拿到后
 *    一搜索就解引用垃圾 -> SIGSEGV。
 *
 * 4) 【重复 vid】add_batch 从不检查 vid 是否已存在, 覆盖旧节点 (泄漏)
 *    且 ntotal 虚增, 与真实节点数不符。
 *
 * 构建: cmake --build build_local --target test_save_load
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
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

static const char *PATH = "/tmp/jw_saveload_test.idx";
static jw_str_t g_sp;
static void init_path(void) { g_sp.ptr = (char *)PATH; g_sp.slen = (jw_size_t)strlen(PATH); }

static void gen_vec(jw_vec_t v, jw_dim_t d, jw_size_t seed) {
    for (jw_dim_t i = 0; i < d; i++) {
        v[i] = (jw_float32_t)sin((double)(seed * 31 + i) * 0.37) * 0.5f
             + (jw_float32_t)cos((double)(seed * 17 + i) * 0.11) * 0.5f;
    }
}

/* ============================================================
 * 1. 稀疏 vid: 存盘后重载必须保住全部向量
 * ============================================================ */
static void test_sparse_vid(void) {
    SECTION("稀疏 vid 存盘重载不丢数据");
    jw_dim_t dim = 16;
    jw_size_t n = 40;
    jw_vec_t data = (jw_vec_t)calloc((size_t)n * dim, sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    /* 稀疏 vid: 100, 200, 300 ... 4000 (远大于 ntotal=40) */
    for (jw_size_t i = 0; i < n; i++) {
        gen_vec(data + (size_t)i * dim, dim, i + 1);
        ids[i] = (jw_vid_t)((i + 1) * 100);
    }

    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t c;
    jw_memset(&c, 0, sizeof(c));
    c.type = JW_INDEX_HNSW; c.metric = JW_METRIC_L2; c.dim = dim;
    c.params.hnsw.M = 16; c.params.hnsw.ef_construction = 64; c.params.hnsw.ef_search = 32;

    jw_index_t *idx = jw_index_create(a, &c);
    CHECK(idx != NULL, "索引创建失败");
    if (idx == NULL) { jw_arena_destroy(a); free(data); free(ids); return; }

    CHECK(jw_index_add_batch(idx, ids, data, n) == JW_SUCCESS, "批量插入失败");
    CHECK(jw_index_get_ntotal(idx) == n, "插入后 ntotal=%zu 应为 %zu",
          (size_t)jw_index_get_ntotal(idx), (size_t)n);

    remove(PATH);
    CHECK(jw_index_save(idx, &g_sp) == JW_SUCCESS, "存盘失败");

    jw_arena_t *a2 = NULL;
    jw_arena_create(64UL << 20, &a2);
    jw_index_t *ld = jw_index_load(a2, &g_sp);

    /* 缺陷 1/3 的检验点: 修复前这里要么崩溃, 要么拿到 ntotal=40 但 0 节点的空索引 */
    CHECK(ld != NULL, "稀疏 vid 重载失败 (返回 NULL)");
    if (ld != NULL) {
        jw_size_t cnt = jw_index_get_ntotal(ld);
        CHECK(cnt == n, "重载后 ntotal=%zu 应为 %zu (修复前为 0 节点)", (size_t)cnt, (size_t)n);

        /* 逐个确认每个稀疏 vid 都还在, 且能搜到
           注意: jw_index_search 返回的是"实际结果条数" (jw_size_t),
           不是状态码, 所以判据是 got>0 而非 == JW_SUCCESS。 */
        jw_search_result_t res[10];
        jw_size_t found = 0;
        for (jw_size_t i = 0; i < n; i++) {
            jw_memset(res, 0, sizeof(res));
            jw_size_t got = jw_index_search(ld, data + (size_t)i * dim, 1, res);
            if (got > 0 && res[0].id == ids[i]) {
                found++;
            }
        }
        /* 允许 HNSW 近似搜索的极小误差, 但至少 90% 要能对上 */
        CHECK(found >= n * 9 / 10, "重载后仅 %zu/%zu 个稀疏向量能被正确检索到",
              (size_t)found, (size_t)n);
        jw_index_destroy(ld);
    }
    jw_arena_destroy(a2);
    jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(data);
    free(ids);
}

/* ============================================================
 * 2. 稀疏 vid 的 IVF 分支同样不能丢
 * ============================================================ */
static void test_sparse_vid_ivf(void) {
    SECTION("稀疏 vid (IVF) 存盘重载不丢数据");
    jw_dim_t dim = 16;
    jw_size_t n = 40;
    jw_vec_t data = (jw_vec_t)calloc((size_t)n * dim, sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) {
        gen_vec(data + (size_t)i * dim, dim, i + 1);
        ids[i] = (jw_vid_t)((i + 1) * 100);
    }

    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t c;
    jw_memset(&c, 0, sizeof(c));
    c.type = JW_INDEX_IVF; c.metric = JW_METRIC_L2; c.dim = dim;
    c.params.ivf.nlist = 4; c.params.ivf.nprobe = 4; c.params.ivf.max_iter = 10;
    jw_index_t *idx = jw_index_create(a, &c);
    jw_index_train(idx, data, n);
    jw_index_add_batch(idx, ids, data, n);

    remove(PATH);
    jw_index_save(idx, &g_sp);
    jw_arena_t *a2 = NULL;
    jw_arena_create(64UL << 20, &a2);
    jw_index_t *ld = jw_index_load(a2, &g_sp);
    CHECK(ld != NULL, "IVF 稀疏 vid 重载失败");
    if (ld != NULL) {
        CHECK(jw_index_get_ntotal(ld) == n, "IVF 重载后 ntotal=%zu 应为 %zu",
              (size_t)jw_index_get_ntotal(ld), (size_t)n);
        jw_index_destroy(ld);
    }
    jw_arena_destroy(a2);
    jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(data);
    free(ids);
}

/* ============================================================
 * 3. 损坏/截断文件必须返回 NULL, 绝不能返回可搜索的坏索引
 * ============================================================ */
static void test_corrupt_file(void) {
    SECTION("损坏文件加载必须返回 NULL");

    /* 先造一个正常的索引文件 */
    jw_dim_t dim = 16;
    jw_size_t n = 30;
    jw_vec_t data = (jw_vec_t)calloc((size_t)n * dim, sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) {
        gen_vec(data + (size_t)i * dim, dim, i + 1);
        ids[i] = i + 1;
    }
    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t c;
    jw_memset(&c, 0, sizeof(c));
    c.type = JW_INDEX_HNSW; c.metric = JW_METRIC_L2; c.dim = dim;
    c.params.hnsw.M = 16; c.params.hnsw.ef_construction = 64; c.params.hnsw.ef_search = 32;
    jw_index_t *idx = jw_index_create(a, &c);
    jw_index_add_batch(idx, ids, data, n);
    remove(PATH);
    jw_index_save(idx, &g_sp);
    jw_index_destroy(idx);
    jw_arena_destroy(a);

    /* 截断到只留头部 (几十字节), 模拟损坏 */
    FILE *f = fopen(PATH, "rb");
    CHECK(f != NULL, "打开索引文件失败");
    if (f == NULL) { free(data); free(ids); return; }
    fseek(f, 0, SEEK_END);
    long full = ftell(f);
    fclose(f);
    CHECK(full > 100, "索引文件过小 (%ld 字节), 前序步骤异常", full);

    int fd = open(PATH, O_WRONLY);
    if (fd >= 0) { ftruncate(fd, 40); close(fd); }

    jw_arena_t *a2 = NULL;
    jw_arena_create(64UL << 20, &a2);
    jw_index_t *ld = jw_index_load(a2, &g_sp);
    /* 缺陷 3 的检验点: 修复前这里返回非 NULL 的坏索引, 后续搜索会段错误 */
    CHECK(ld == NULL, "截断文件加载应返回 NULL, 实际返回 %p", (void *)ld);
    if (ld != NULL) { jw_index_destroy(ld); }
    jw_arena_destroy(a2);

    /* 空文件 / 不存在的文件也必须返回 NULL */
    jw_str_t bad;
    bad.ptr = (char *)"/tmp/jw_definitely_not_exist.idx";
    bad.slen = (jw_size_t)strlen(bad.ptr);
    jw_arena_t *a3 = NULL;
    jw_arena_create(16UL << 20, &a3);
    CHECK(jw_index_load(a3, &bad) == NULL, "不存在的索引文件应返回 NULL");
    jw_arena_destroy(a3);

    free(data);
    free(ids);
}

/* ============================================================
 * 4. 重复 vid 必须被拒绝, 且不影响 ntotal
 * ============================================================ */
static void test_duplicate_vid(void) {
    SECTION("重复 vid 必须被拒绝");
    jw_dim_t dim = 8;
    jw_size_t n = 20;
    jw_vec_t data = (jw_vec_t)calloc((size_t)n * dim, sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) {
        gen_vec(data + (size_t)i * dim, dim, i + 1);
        ids[i] = i + 1;
    }
    /* 再插一批, 第 2 个就与已有的 vid=1 重复 */
    jw_vid_t dup[3] = { 1, 2, 3 };

    /* --- HNSW --- */
    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t c;
    jw_memset(&c, 0, sizeof(c));
    c.type = JW_INDEX_HNSW; c.metric = JW_METRIC_L2; c.dim = dim;
    c.params.hnsw.M = 8; c.params.hnsw.ef_construction = 32; c.params.hnsw.ef_search = 32;
    jw_index_t *idx = jw_index_create(a, &c);
    CHECK(jw_index_add_batch(idx, ids, data, n) == JW_SUCCESS, "HNSW 首次插入失败");
    CHECK(jw_index_get_ntotal(idx) == n, "HNSW 插入后 ntotal 应为 %zu", (size_t)n);

    jw_status_t st = jw_index_add_batch(idx, dup, data, 3);
    CHECK(st == JW_ALREADY_EXISTS, "HNSW 重复 vid 应返回 JW_ALREADY_EXISTS(-5), 实际 %d", st);
    CHECK(jw_index_get_ntotal(idx) == n, "HNSW 重复插入后 ntotal 仍应为 %zu (不能虚增), 实际 %zu",
          (size_t)n, (size_t)jw_index_get_ntotal(idx));
    jw_index_destroy(idx);
    jw_arena_destroy(a);

    /* --- IVF --- */
    jw_arena_t *a4 = NULL;
    jw_arena_create(64UL << 20, &a4);
    jw_index_config_t c4;
    jw_memset(&c4, 0, sizeof(c4));
    c4.type = JW_INDEX_IVF; c4.metric = JW_METRIC_L2; c4.dim = dim;
    c4.params.ivf.nlist = 4; c4.params.ivf.nprobe = 4; c4.params.ivf.max_iter = 10;
    jw_index_t *i4 = jw_index_create(a4, &c4);
    jw_index_train(i4, data, n);
    jw_status_t s1 = jw_index_add_batch(i4, ids, data, n);
    CHECK(s1 == JW_SUCCESS, "IVF 首次插入失败 (%d)", s1);
    jw_status_t s2 = jw_index_add_batch(i4, dup, data, 3);
    CHECK(s2 == JW_ALREADY_EXISTS, "IVF 重复 vid 应返回 JW_ALREADY_EXISTS(-5), 实际 %d", s2);
    CHECK(jw_index_get_ntotal(i4) == n, "IVF 重复插入后 ntotal 仍应为 %zu, 实际 %zu",
          (size_t)n, (size_t)jw_index_get_ntotal(i4));
    jw_index_destroy(i4);
    jw_arena_destroy(a4);

    free(data);
    free(ids);
}

/* ============================================================
 * 5. 超大 vid 必须被拒绝 (防 OOM: vid=40亿 会分配 ~72GB)
 * ============================================================ */
static void test_huge_vid(void) {
    SECTION("超大 vid 必须被拒绝 (防 OOM)");
    jw_dim_t dim = 4;
    jw_vec_t v = (jw_vec_t)calloc(dim, sizeof(jw_float32_t));
    v[0] = 0.1f; v[1] = 0.2f; v[2] = 0.3f; v[3] = 0.4f;

    jw_arena_t *a = NULL;
    jw_arena_create(64UL << 20, &a);
    jw_index_config_t c;
    jw_memset(&c, 0, sizeof(c));
    c.type = JW_INDEX_HNSW; c.metric = JW_METRIC_L2; c.dim = dim;
    c.params.hnsw.M = 8; c.params.hnsw.ef_construction = 32; c.params.hnsw.ef_search = 32;
    jw_index_t *idx = jw_index_create(a, &c);
    if (idx != NULL) {
        /* vid = 40 亿: 修复前会让 capacity=(vid+1)*2 触发 ~72GB 分配被 OOM kill */
        jw_vid_t big = 4000000000UL;
        jw_status_t st = jw_index_add_batch(idx, &big, v, 1);
        CHECK(st == JW_INVALID_PARAM, "超大 vid 应返回 JW_INVALID_PARAM, 实际 %d", st);
        CHECK(jw_index_get_ntotal(idx) == 0, "超大 vid 插入后 ntotal 应为 0, 实际 %zu",
              (size_t)jw_index_get_ntotal(idx));
    }
    if (idx) jw_index_destroy(idx);
    jw_arena_destroy(a);
    free(v);
}

/* ============================================================
 * 6. 密集 vid 往返完整性 (对照, 保证没改坏正常路径)
 * ============================================================ */
static void test_dense_roundtrip(void) {
    SECTION("密集 vid 存盘加载往返 (对照)");
    jw_index_type_t types[] = { JW_INDEX_IVF, JW_INDEX_IVF_PQ, JW_INDEX_IVF_SQ, JW_INDEX_HNSW };
    const char *names[] = { "IVF", "IVF_PQ", "IVF_SQ", "HNSW" };
    jw_dim_t dim = 16;
    jw_size_t n = 60;

    jw_vec_t data = (jw_vec_t)calloc((size_t)n * dim, sizeof(jw_float32_t));
    jw_vid_t *ids = (jw_vid_t *)malloc(n * sizeof(jw_vid_t));
    for (jw_size_t i = 0; i < n; i++) {
        gen_vec(data + (size_t)i * dim, dim, i + 1);
        ids[i] = i + 1;
    }

    for (int t = 0; t < 4; t++) {
        jw_arena_t *a = NULL;
        jw_arena_create(256UL << 20, &a);
        jw_index_config_t c;
        jw_memset(&c, 0, sizeof(c));
        c.type = types[t]; c.metric = JW_METRIC_L2; c.dim = dim;
        c.params.ivf.nlist = 4; c.params.ivf.nprobe = 4; c.params.ivf.max_iter = 10;
        c.params.hnsw.M = 16; c.params.hnsw.ef_construction = 64; c.params.hnsw.ef_search = 32;
        if (types[t] == JW_INDEX_IVF_PQ) {
            c.pq = (jw_pq_config_t)JW_PQ_CONFIG_DEFAULT;
        }
        jw_index_t *idx = jw_index_create(a, &c);
        if (idx == NULL) { jw_arena_destroy(a); continue; }
        jw_index_train(idx, data, n);
        jw_index_add_batch(idx, ids, data, n);

        remove(PATH);
        jw_status_t sv = jw_index_save(idx, &g_sp);
        CHECK(sv == JW_SUCCESS, "%s 存盘失败 (%d)", names[t], sv);

        jw_arena_t *a2 = NULL;
        jw_arena_create(256UL << 20, &a2);
        jw_index_t *ld = jw_index_load(a2, &g_sp);
        CHECK(ld != NULL, "%s 重载失败 (返回 NULL)", names[t]);
        if (ld != NULL) {
            CHECK(jw_index_get_ntotal(ld) == n, "%s 重载后 ntotal=%zu 应为 %zu",
                  names[t], (size_t)jw_index_get_ntotal(ld), (size_t)n);
            jw_search_result_t res[10];
            jw_memset(res, 0, sizeof(res));
            jw_size_t got = jw_index_search(ld, data, 5, res);
            CHECK(got > 0, "%s 重载后搜索返回 0 条结果", names[t]);
            jw_index_destroy(ld);
        }
        jw_arena_destroy(a2);
        jw_index_destroy(idx);
        jw_arena_destroy(a);
    }
    free(data);
    free(ids);
}

int main(void) {
    printf("========================================\n");
    printf(" 存盘/加载与 vid 健壮性回归测试\n");
    printf("========================================\n\n");

    init_path();
    test_dense_roundtrip();
    test_sparse_vid();
    test_sparse_vid_ivf();
    test_corrupt_file();
    test_duplicate_vid();
    test_huge_vid();

    printf("\n========================================\n");
    printf(" 通过: %d   失败: %d\n", g_pass, g_fail);
    printf("========================================\n");
    return (g_fail == 0) ? 0 : 1;
}
