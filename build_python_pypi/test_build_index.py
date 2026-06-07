#!/usr/bin/env python3
"""测试 build_index() 在 Linux 上是否会崩溃"""
import sys
import os

# 直接把 libjinwo.so 所在目录加到搜索路径
lib_dir = os.path.join(os.path.dirname(__file__), 'build', 'lib.linux-x86_64-cpython-39', 'jinwo_vecdb')
os.environ['LD_LIBRARY_PATH'] = lib_dir + ':' + os.environ.get('LD_LIBRARY_PATH', '')
sys.path.insert(0, lib_dir)
sys.path.insert(0, os.path.dirname(__file__))

import jinwo_vecdb

print("1. 创建数据库 (内存模式)...")
db = jinwo_vecdb.open("")

print("2. 创建 collection (dim=128)...")
coll = db.create_collection('test_build_index', dim=128)

print("3. 插入 200 个向量...")
import random
for i in range(200):
    vec = [random.random() for _ in range(128)]
    coll.insert(vec)
print("   已插入 200 个向量")

print("4. 搜索 (无索引, 暴力扫描)...")
query = [random.random() for _ in range(128)]
results = coll.search(query, k=5)
assert len(results) == 5, f"暴力搜索应返回 5 条, 实际 {len(results)}"
print(f"   搜索结果: {len(results)} 条")

print("5. build_index() ... 关键步骤! 如果崩溃会在这里")
sys.stdout.flush()
coll.build_index()
print("   build_index() 成功!")

print("6. 搜索 (有索引)...")
results = coll.search(query, k=5)
assert len(results) == 5, f"索引搜索应返回 5 条, 实际 {len(results)}"
print(f"   搜索结果: {len(results)} 条")

print("7. 再插入 100 个向量...")
for i in range(100):
    vec = [random.random() for _ in range(128)]
    coll.insert(vec)

print("8. 再次搜索...")
results = coll.search(query, k=5)
assert len(results) == 5, f"再次搜索应返回 5 条, 实际 {len(results)}"
print(f"   搜索结果: {len(results)} 条")

print("\n✅ 全部测试通过!")
