#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JinWo VecDB Python 演示（使用 pip 安装的 jinwo_vecdb 包，高层 API）。

运行前请先安装官方包：
    pip install jinwo_vecdb

运行：
    python python_demo.py

API 以 jinwo_vecdb/__init__.py 为准：
    open / create_collection / insert / insert_batch / search / delete / close
"""

import os

import jinwo_vecdb


def main():
    db_path = "vecdb_demo.jwv"
    if os.path.exists(db_path):
        os.remove(db_path)

    print("version:", jinwo_vecdb.__version__)

    # 打开/创建数据库（path 为空字符串表示内存数据库）
    db = jinwo_vecdb.open(db_path)

    # 创建 Collection
    db.create_collection("docs", dim=4)

    # 单条插入
    vid = db.insert("docs", [0.1, 0.2, 0.3, 0.4])
    print("inserted vid:", vid)

    # 批量插入
    vids = db.insert_batch("docs", [
        [0.1, 0.2, 0.3, 0.4],
        [0.5, 0.6, 0.7, 0.8],
    ])
    print("batch inserted vids:", vids)

    # 相似度搜索，返回 [(vid, distance), ...]
    results = db.search("docs", [0.1, 0.2, 0.3, 0.4], k=3)
    print("search results (vid, distance):")
    for vid, distance in results:
        print(f"  vid={vid}, distance={distance}")

    # 删除向量
    if results:
        db.delete("docs", results[0][0])
        print("deleted vid:", results[0][0])

    db.close()
    print("OK")


if __name__ == "__main__":
    main()
