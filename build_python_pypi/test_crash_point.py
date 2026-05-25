#!/usr/bin/env python3
"""测试: 先插入不触发训练, 再 build_index, 再搜索"""
import sys, os, random

lib_dir = os.path.join(os.path.dirname(__file__), 'build', 'lib.linux-x86_64-cpython-39', 'jinwo_vecdb')
sys.path.insert(0, os.path.dirname(__file__))
os.environ['LD_LIBRARY_PATH'] = lib_dir

import jinwo_vecdb

# 用 threshold=100M 避免 auto-training 干扰
# 已经在 C 代码里设了 threshold=100000000
db = jinwo_vecdb.open("")
coll = db.create_collection('test', dim=128)

print("1. 插入 200 个向量 (不触发训练)...")
for i in range(200):
    coll.insert([random.random() for _ in range(128)])
print("   done")

print("2. build_index...")
sys.stdout.flush()
coll.build_index()
print("   done")

print("3. 搜索 (threshold=1, 会走 IVF)...")
sys.stdout.flush()
results = coll.search([random.random() for _ in range(128)], k=5)
print(f"   结果: {len(results)}")
print("   done!")
