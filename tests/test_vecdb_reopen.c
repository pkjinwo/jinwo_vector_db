/**
 * @file test_vecdb_reopen.c
 * @brief vecdb 级持久化回归：close 后 reopen 必须能取回 collection 与数据
 *
 * 2026-11-14 新增 (HANDOFF §16)。CI-Windows 的 Python E2E 暴露出：
 * Windows 上 reopen 后 "Collections in DB: []"（历史版本一直存在，被旧脚本
 * 的 WARN 容忍掩盖）。根因是 Windows CRT open() 默认文本模式，缺 _O_BINARY，
 * .jwcol 二进制数据中的 0x0A 被翻译成 0x0D 0x0A，加载必然失败。
 * 本测试在所有平台锁死 "close -> reopen -> 数据完整" 这条链路
 * （此前 C 层对此零覆盖，test_save_load.c 只测索引级且硬编码 /tmp 路径）。
 *
 * 构建: cmake --build build_local --target test_vecdb_reopen
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "jw_vecdb.h"

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  [FAIL] "); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

int main(void)
{
    char db_path[128];
    snprintf(db_path, sizeof(db_path), "jw_reopen_%ld.jwv", (long)time(NULL));
    jw_str_t path = {db_path, strlen(db_path)};

    /* ---- 第 1 轮: 打开(新建) -> 建集合 -> 插入 -> 关闭 ---- */
    jw_vecdb_t *db = NULL;
    jw_status_t st = jw_vecdb_open(&path, JW_VECDB_CREATE, &db);
    CHECK(st == JW_SUCCESS && db != NULL, "open(create) failed st=%d", (int)st);

    jw_str_t name = {"docs", 4};
    jw_collection_t *coll = NULL;
    st = jw_vecdb_create_collection(db, &name, 16, &coll);
    CHECK(st == JW_SUCCESS && coll != NULL, "create_collection failed st=%d", (int)st);

    jw_float32_t vec[16];
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 16; j++) vec[j] = (jw_float32_t)(i * 4 + j);
        jw_vid_t vid = 0;
        st = jw_vecdb_insert(db, &name, vec, 16, &vid);
        CHECK(st == JW_SUCCESS, "insert #%d failed st=%d", i, (int)st);
    }

    st = jw_vecdb_close(db);
    CHECK(st == JW_SUCCESS, "close failed st=%d", (int)st);

    /* ---- 第 2 轮: 重新打开 -> 集合必须还在 -> 数据必须完整 ---- */
    db = NULL;
    st = jw_vecdb_open(&path, 0, &db);
    CHECK(st == JW_SUCCESS && db != NULL, "reopen failed st=%d", (int)st);

    jw_str_t names[8];
    jw_size_t n = jw_vecdb_list_collections(db, names, 8);
    CHECK(n == 1, "list_collections after reopen: got=%zu expected=1", (size_t)n);

    jw_collection_t *coll2 = jw_vecdb_get_collection(db, &name);
    CHECK(coll2 != NULL, "get_collection after reopen returned NULL");

    if (coll2 != NULL) {
        jw_collection_stats_t stats;
        st = jw_collection_get_stats(coll2, &stats);
        CHECK(st == JW_SUCCESS && stats.count == 10,
              "record count after reopen: got=%zu expected=10", (size_t)stats.count);

        /* 逐条取回验证内容 */
        for (int i = 0; i < 10; i++) {
            jw_float32_t got[16];
            st = jw_collection_get(coll2, (jw_vid_t)(i + 1), got);
            CHECK(st == JW_SUCCESS, "get vid=%d failed st=%d", i + 1, (int)st);
            if (st == JW_SUCCESS) {
                int ok = 1;
                for (int j = 0; j < 16; j++) {
                    if (got[j] != (jw_float32_t)(i * 4 + j)) { ok = 0; break; }
                }
                CHECK(ok, "vid=%d vector content mismatch", i + 1);
            }
        }
    }

    jw_vecdb_close(db);

    printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
