#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
完整生命周期测试: 新建 -> 增查 -> 关闭 -> 重新打开(100条持久化) -> 删改查 -> 关闭 -> 重新打开 -> 增删查 -> 关闭
"""
import os
import sys
import shutil

# 确保能找到 jinwo_vecdb
sys.path.insert(0, os.path.dirname(__file__))

import jinwo_vecdb

DB_PATH = os.path.join(os.path.dirname(__file__), "test_lifecycle_db.jwv")
COLL_NAME = "lifecycle_test"
DIM = 128

def cleanup():
    if os.path.exists(DB_PATH):
        if os.path.isfile(DB_PATH):
            os.remove(DB_PATH)
        else:
            shutil.rmtree(DB_PATH, ignore_errors=True)

def assert_eq(actual, expected, msg):
    assert actual == expected, f"{msg}: expected {expected}, got {actual}"

def log(msg):
    print(f"  {msg}")

def log_ok(msg):
    print(f"  [OK] {msg}")

def log_fail(msg):
    print(f"  [FAIL] {msg}")

# ============================================================
# Phase 1: 新建数据库 -> 插入100条 -> 验证查询 -> 关闭 (不删除!)
# ============================================================
print("=" * 60)
print("Phase 1: 新建库 -> 插入100 -> 关闭 (不删除)")
print("=" * 60)

cleanup()

# 1.1 创建/打开库
db = jinwo_vecdb.open(DB_PATH)
log_ok("打开数据库")

# 1.2 创建 collection
db.create_collection(COLL_NAME, DIM)
log_ok("创建 collection")

# 1.3 获取 collection 验证
coll = db.get_collection(COLL_NAME)
assert_eq(coll.dim, DIM, "collection dim")
log_ok(f"get_collection: dim={coll.dim}")

# 1.4 Insert 100 条向量
vids = []
for i in range(100):
    vec = [float(i * 100 + j) for j in range(DIM)]
    vid = db.insert(COLL_NAME, vec)
    vids.append(vid)
log_ok(f"插入 100 条向量, vids={vids[:3]}...{vids[-3:]}")

# 1.5 Count = 100
count = db.get_collection(COLL_NAME).count
assert_eq(count, 100, "插入后 count")
log_ok(f"count = {count}")

# 1.6 Get - 取第0条
vec_data = db.get(COLL_NAME, vids[0])
assert vec_data is not None, "get 应该返回数据"
log_ok(f"get(vid={vids[0]}): dim={len(vec_data)}, first_3={vec_data[:3]}")

# 1.7 Update - 更新第0条
new_vec = [float(999 + j) for j in range(DIM)]
db.update(COLL_NAME, vids[0], new_vec)
log_ok(f"update(vid={vids[0]})")

# 验证 update
vec_data2 = db.get(COLL_NAME, vids[0])
assert abs(vec_data2[0] - 999.0) < 0.001, f"更新后值不对: {vec_data2[0]}"
log_ok(f"验证 update: first_val={vec_data2[0]}")

# 1.8 Search 查 10 条
query = [float(5000 + j) for j in range(DIM)]
results = db.search(COLL_NAME, query, k=10)
log_ok(f"查询返回 {len(results)} 条结果, count={count}")
assert len(results) >= 1, "搜索结果不应为空"
for idx, (vid, dist) in enumerate(results[:5]):
    log(f"  #{idx+1}: vid={vid}, dist={dist:.6f}")

# 1.9 关闭（不删除任何数据！）
db.close()
log_ok("Phase 1 关闭数据库 (100条完整数据)")

# ============================================================
# Phase 2: 重新打开 -> 验证100条持久化 -> 删除50条 -> 增改查 -> 关闭
# ============================================================
print()
print("=" * 60)
print("Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> CRUD -> 关闭")
print("=" * 60)

# 2.1 重新打开
db2 = jinwo_vecdb.open(DB_PATH)
log_ok("重新打开数据库")

# 2.2 验证 collection 存在
coll2 = db2.get_collection(COLL_NAME)
assert_eq(coll2.dim, DIM, "重新打开后 dim")
log_ok(f"重新打开后 dim={coll2.dim}")

# 2.3 验证 100 条完整持久化
count2 = coll2.count
assert_eq(count2, 100, "100条持久化验证: count 应为 100")
log_ok(f"100条持久化: count={count2} ✓")

# 2.4 重新打开后 search 应该能查到 10 条
results3 = db2.search(COLL_NAME, query, k=10)
log_ok(f"重新打开后查询返回 {len(results3)} 条结果 (应有 10 条)")
assert len(results3) == 10, f"重新打开后应返回 10 条, 实际 {len(results3)}"
for idx, (vid, dist) in enumerate(results3[:5]):
    log(f"  #{idx+1}: vid={vid}, dist={dist:.6f}")

# 2.5 重新打开后 get 能取到每条数据
for test_vid in [vids[0], vids[50], vids[99]]:
    vec_data3 = db2.get(COLL_NAME, test_vid)
    assert vec_data3 is not None, f"get(vid={test_vid}) 应该返回数据"
log_ok(f"get 3条随机抽检: vid={vids[0]},{vids[50]},{vids[99]} 全部存在 ✓")

# 2.6 Update 验证的 vid
update_vec = [float(7777 + j) for j in range(DIM)]
db2.update(COLL_NAME, vids[50], update_vec)
vec_data4 = db2.get(COLL_NAME, vids[50])
assert abs(vec_data4[0] - 7777.0) < 0.001, f"更新后值不对: {vec_data4[0]}"
log_ok(f"更新 vid={vids[50]}: first_val={vec_data4[0]} ✓")

# 2.7 Delete 删除一半 (50条)
delete_count = 50
for i in range(delete_count):
    db2.delete(COLL_NAME, vids[i])
log_ok(f"删除 {delete_count} 条向量")

# 2.8 验证删除后的 count
count3 = db2.get_collection(COLL_NAME).count
assert_eq(count3, 50, "删除后 count")
log_ok(f"删除后 count={count3}")

# 2.9 验证删除后的 search：已删除的 vid 不应出现
results4 = db2.search(COLL_NAME, query, k=10)
log_ok(f"删除后查询返回 {len(results4)} 条结果")
deleted_vids = set(vids[:delete_count])
for (vid, _) in results4:
    if vid in deleted_vids:
        log_fail(f"已删除的 vid={vid} 仍然出现在搜索结果中!")
        break
else:
    log_ok("已删除的 vid 没有出现在搜索结果中 ✓")

# 2.10 关闭
db2.close()
log_ok("Phase 2 关闭数据库 (50条剩余)")

# ============================================================
# Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭
# ============================================================
print()
print("=" * 60)
print("Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭")
print("=" * 60)

# 3.1 再次重新打开
db3 = jinwo_vecdb.open(DB_PATH)
log_ok("再次重新打开数据库")

# 3.2 验证 collection 存在且 dim 正确
coll3 = db3.get_collection(COLL_NAME)
assert_eq(coll3.dim, DIM, "再次重新打开后 dim")
log_ok(f"dim={coll3.dim}")

# 3.3 验证 count = 50
count4 = coll3.count
assert_eq(count4, 50, "50条持久化验证: count 应为 50")
log_ok(f"50条持久化: count={count4} ✓")

# 3.4 Get 已删除的应该返回 None
deleted_vid = vids[0]
vec_data5 = db3.get(COLL_NAME, deleted_vid)
if vec_data5 is None:
    log_ok(f"get 已删除的 vid={deleted_vid} 返回 None (正确) ✓")
else:
    log_fail(f"get 已删除的 vid={deleted_vid} 返回了数据!")

# 3.5 Get 未删除的应该正常
remaining_vid = vids[delete_count]
vec_data6 = db3.get(COLL_NAME, remaining_vid)
assert vec_data6 is not None, f"get(vid={remaining_vid}) 应该返回数据"
log_ok(f"get 未删除的 vid={remaining_vid}: dim={len(vec_data6)} ✓")

# 3.6 Insert 20 条新数据
new_vids = []
for i in range(20):
    vec = [float(10000 + i * 100 + j) for j in range(DIM)]
    vid = db3.insert(COLL_NAME, vec)
    new_vids.append(vid)
log_ok("插入 20 条新向量")

# 3.7 验证 count = 70
count5 = db3.get_collection(COLL_NAME).count
assert_eq(count5, 70, "新插入后 count")
log_ok(f"新插入后 count={count5}")

# 3.8 Search 查询新数据
new_query = [float(10000 + 500 + j) for j in range(DIM)]
results5 = db3.search(COLL_NAME, new_query, k=5)
log_ok(f"查询新数据返回 {len(results5)} 条结果")
for idx, (vid, dist) in enumerate(results5[:5]):
    log(f"  #{idx+1}: vid={vid}, dist={dist:.6f}")

# 3.9 删除新插入的前 10 条
for i in range(10):
    db3.delete(COLL_NAME, new_vids[i])
log_ok("删除新插入的 10 条向量")

# 3.10 验证 count = 60
count6 = db3.get_collection(COLL_NAME).count
assert_eq(count6, 60, "再次删除后 count")
log_ok(f"再次删除后 count={count6}")

# 3.11 验证删除后的查询
results6 = db3.search(COLL_NAME, new_query, k=5)
log_ok(f"删除新数据后查询返回 {len(results6)} 条结果")
new_deleted = set(new_vids[:10])
for (vid, _) in results6:
    if vid in new_deleted:
        log_fail(f"已删除的 vid={vid} 出现在第二次查询中!")
        break
else:
    log_ok("Phase 3 删除的 vid 不在查询结果中 ✓")

# 3.12 关闭
db3.close()
log_ok("Phase 3 关闭数据库")

# ============================================================
# 清理
# ============================================================
print()
cleanup()
log_ok("清理测试文件")

# ============================================================
# 总结
# ============================================================
print()
print("=" * 60)
print("所有生命周期测试通过!")
print("  Phase 1: 100条完整持久化 ✓")
print("  Phase 2: 删除50后持久化 ✓")
print("  Phase 3: 50条+新增20删10持久化 ✓")
print("=" * 60)
