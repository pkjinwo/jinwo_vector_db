#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JinWo VecDB 低层 C FFI 演示（ctypes 直调 libjinwo）。

与文档 docs/zh_cn/usage_examples.zh_cn.md 的「C / C++」示例保持一致的 API：
    jw_vecdb_open / jw_vecdb_create_collection /
    jw_collection_insert / jw_vecdb_search / jw_vecdb_close

运行前请先构建 C 库：
    cmake -S . -B build && cmake --build build
（产物为 libjinwo.so / libjinwo.dylib / libjinwo.dll）
"""

import ctypes
import os
import sys

#==============================================================================
# C 类型映射（与 include/jw_types.h 保持一致）
#==============================================================================

class jw_str_t(ctypes.Structure):
    """JinWo 字符串类型: {char *ptr; size_t slen}"""
    _fields_ = [
        ("ptr", ctypes.c_char_p),
        ("slen", ctypes.c_size_t),
    ]


class jw_search_result_t(ctypes.Structure):
    """搜索结果: {id; score; vector; dimension; metadata; metadata_size}"""
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("score", ctypes.c_float),
        ("vector", ctypes.POINTER(ctypes.c_float)),
        ("dimension", ctypes.c_size_t),
        ("metadata", ctypes.c_void_p),
        ("metadata_size", ctypes.c_size_t),
    ]


# 标志定义（与 include/jw_types.h 中 JW_* 宏一致）
JW_VECDB_CREATE = 0x04
JW_SUCCESS = 0

#==============================================================================
# 动态库加载（跨平台：自动查找 libjinwo / jinwo / libjw_vecdb）
#==============================================================================

def _load_library():
    here = os.path.dirname(os.path.abspath(__file__))

    if sys.platform == "darwin":
        exts = [".dylib"]
    elif sys.platform == "win32":
        exts = [".dll"]
    else:
        exts = [".so"]

    names = ["libjinwo", "jinwo", "libjw_vecdb", "jw_vecdb"]
    dirs = [
        os.path.join(here, "..", "..", "build"),
        os.path.join(here, "..", "..", "build_python_pypi", "jinwo_vecdb"),
        here,
        ".",
    ]

    for d in dirs:
        for name in names:
            for ext in exts:
                path = os.path.join(d, name + ext)
                if os.path.exists(path):
                    try:
                        return ctypes.CDLL(path)
                    except Exception:
                        continue
    raise RuntimeError(
        "找不到 JinWo VecDB 动态库。请先构建：\n"
        "  cmake -S . -B build && cmake --build build"
    )


lib = _load_library()

#==============================================================================
# 函数原型声明（匹配真实 ABI）
#==============================================================================

lib.jw_version_string.restype = ctypes.c_char_p
lib.jw_version_string.argtypes = []

lib.jw_vecdb_open.restype = ctypes.c_int
lib.jw_vecdb_open.argtypes = [
    ctypes.POINTER(jw_str_t),   # const jw_str_t *path
    ctypes.c_uint32,            # jw_uint32_t flags
    ctypes.POINTER(ctypes.c_void_p),  # jw_vecdb_t **db
]

lib.jw_vecdb_create_collection.restype = ctypes.c_int
lib.jw_vecdb_create_collection.argtypes = [
    ctypes.c_void_p,            # jw_vecdb_t *db
    ctypes.POINTER(jw_str_t),   # const jw_str_t *name
    ctypes.c_uint32,            # jw_dim_t dim
    ctypes.POINTER(ctypes.c_void_p),  # jw_collection_t **coll
]

lib.jw_collection_insert.restype = ctypes.c_int
lib.jw_collection_insert.argtypes = [
    ctypes.c_void_p,            # jw_collection_t *coll
    ctypes.POINTER(ctypes.c_float),  # jw_cvec_t vec
    ctypes.POINTER(ctypes.c_uint64),  # jw_vid_t *vid
]

lib.jw_vecdb_search.restype = ctypes.c_size_t
lib.jw_vecdb_search.argtypes = [
    ctypes.c_void_p,            # jw_vecdb_t *db
    ctypes.POINTER(jw_str_t),   # const jw_str_t *coll_name
    ctypes.POINTER(ctypes.c_float),  # jw_cvec_t query
    ctypes.c_uint32,            # jw_dim_t dim
    ctypes.c_size_t,            # jw_size_t k
    ctypes.POINTER(jw_search_result_t),  # jw_search_result_t *results
]

lib.jw_vecdb_list_collections.restype = ctypes.c_size_t
lib.jw_vecdb_list_collections.argtypes = [
    ctypes.c_void_p,            # const jw_vecdb_t *db
    ctypes.POINTER(jw_str_t),   # jw_str_t *names
    ctypes.c_size_t,            # jw_size_t capacity
]

lib.jw_vecdb_close.restype = ctypes.c_int
lib.jw_vecdb_close.argtypes = [ctypes.c_void_p]  # jw_vecdb_t *db


#==============================================================================
# 简易封装
#==============================================================================

def _to_jwstr(s):
    b = s.encode("utf-8")
    return jw_str_t(ctypes.c_char_p(b), len(b))


def _from_jwstr(js):
    return js.ptr[:js.slen].decode("utf-8")


class Collection:
    def __init__(self, db_ptr, coll_ptr):
        self.db_ptr = db_ptr
        self.ptr = coll_ptr

    def insert(self, vec):
        arr = (ctypes.c_float * len(vec))(*vec)
        vid = ctypes.c_uint64(0)
        lib.jw_collection_insert(self.ptr, arr, ctypes.byref(vid))
        return vid.value


class VecDB:
    def __init__(self, path):
        self.ptr = ctypes.c_void_p(0)
        st = lib.jw_vecdb_open(
            ctypes.byref(_to_jwstr(path)), JW_VECDB_CREATE, ctypes.byref(self.ptr)
        )
        if st != JW_SUCCESS:
            raise RuntimeError("jw_vecdb_open failed, status=%d" % st)

    def create_collection(self, name, dim):
        coll = ctypes.c_void_p(0)
        st = lib.jw_vecdb_create_collection(
            self.ptr, ctypes.byref(_to_jwstr(name)), ctypes.c_uint32(dim),
            ctypes.byref(coll),
        )
        if st != JW_SUCCESS:
            raise RuntimeError("jw_vecdb_create_collection failed, status=%d" % st)
        return Collection(self.ptr, coll)

    def search(self, name, vec, k):
        arr = (ctypes.c_float * len(vec))(*vec)
        results = (jw_search_result_t * k)()
        n = lib.jw_vecdb_search(
            self.ptr, ctypes.byref(_to_jwstr(name)), arr,
            ctypes.c_uint32(len(vec)), ctypes.c_size_t(k), results,
        )
        return [(results[i].id, results[i].score) for i in range(n)]

    def list_collections(self):
        cap = 16
        names = (jw_str_t * cap)()
        n = lib.jw_vecdb_list_collections(self.ptr, names, ctypes.c_size_t(cap))
        return [_from_jwstr(names[i]) for i in range(n)]

    def close(self):
        lib.jw_vecdb_close(self.ptr)


#==============================================================================
# 演示
#==============================================================================

def main():
    db_path = "vecdb_demo.jwv"
    if os.path.exists(db_path):
        os.remove(db_path)

    print("version:", lib.jw_version_string().decode("utf-8"))

    db = VecDB(db_path)
    col = db.create_collection("docs", dim=4)
    vid = col.insert([0.1, 0.2, 0.3, 0.4])
    print("inserted vid:", vid)
    print("collections:", db.list_collections())

    results = db.search("docs", [0.1, 0.2, 0.3, 0.4], k=3)
    print("search results (vid, score):", results)

    db.close()
    print("OK")


if __name__ == "__main__":
    main()
