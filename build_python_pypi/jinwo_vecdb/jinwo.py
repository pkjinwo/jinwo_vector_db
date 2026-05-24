"""
JinWo VecDB - Python 高级 API

基于 ctypes 的 Python 绑定层
"""

import ctypes
import os
import sys
from pathlib import Path
from typing import List, Optional, Tuple, Union

#==============================================================================
# C 类型映射
#==============================================================================

class jw_str_t(ctypes.Structure):
    """JinWo 字符串类型: {char *ptr; size_t slen}"""
    _fields_ = [
        ("ptr", ctypes.c_char_p),
        ("slen", ctypes.c_size_t),
    ]

class jw_search_result_t(ctypes.Structure):
    """搜索结果: {id; score; vector; dim; metadata; metadata_size}"""
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("score", ctypes.c_float),
        ("vector", ctypes.POINTER(ctypes.c_float)),
        ("dimension", ctypes.c_size_t),
        ("metadata", ctypes.c_void_p),
        ("metadata_size", ctypes.c_size_t),
    ]

# 辅助: Python bytes → jw_str_t (指针指向 bytes 的数据)
def _bytes_to_jwstr(b: bytes):
    return jw_str_t(ctypes.c_char_p(b), len(b))

# 辅助: jw_str_t → Python str
def _jwstr_to_str(js):
    return js.ptr[:js.slen].decode('utf-8')

#==============================================================================
# 动态库加载
#==============================================================================

# 标志定义
JW_VECDB_READONLY = 0x01
JW_VECDB_READWRITE = 0x02
JW_VECDB_CREATE = 0x04
JW_VECDB_TRUNCATE = 0x08
JW_VECDB_MEMORY = 0x10

JW_SUCCESS = 0  # C 状态码: 成功

def _load_library():
    """加载 JinWo C 动态库"""
    package_dir = Path(__file__).parent

    # 定义所有可能的扩展模块文件名模式
    # 注意: Windows 上 scikit-build-core 可能生成 .dll 而非 .pyd，
    #       所以需要同时搜索两种扩展名
    patterns = [
        "_jinwo*.so",
        "_jinwo*.pyd",
        "_jinwo*.dll",
        "_jinwo*.dylib",
        "jinwo*.so",
        "jinwo*.pyd",
        "jinwo*.dll",
        "jinwo*.dylib",
        "libjinwo*.so",
        "libjinwo*.pyd",
        "libjinwo*.dll",
        "libjinwo*.dylib",
    ]

    # 搜索包目录
    for pattern in patterns:
        for f in package_dir.glob(pattern):
            try:
                print(f"  [DEBUG] Trying to load: {f}", flush=True)
                lib = ctypes.CDLL(str(f))
                print(f"  [DEBUG] Loaded successfully: {f}", flush=True)
                return lib
            except OSError as e:
                print(f"  [DEBUG] Failed to load {f}: {e}", flush=True)
                continue
    
    # 搜索 Release 目录 (Windows MSVC 编译输出目录)
    release_dir = package_dir / "Release"
    if release_dir.exists() and release_dir.is_dir():
        for pattern in patterns:
            for f in release_dir.glob(pattern):
                try:
                    print(f"  [DEBUG] Trying to load (Release): {f}", flush=True)
                    lib = ctypes.CDLL(str(f))
                    print(f"  [DEBUG] Loaded successfully (Release): {f}", flush=True)
                    return lib
                except OSError as e:
                    print(f"  [DEBUG] Failed to load (Release) {f}: {e}", flush=True)
                    continue
    
    # 搜索 py_jinwo 目录
    py_jinwo_dir = package_dir / "py_jinwo"
    if py_jinwo_dir.exists() and py_jinwo_dir.is_dir():
        for pattern in patterns:
            for f in py_jinwo_dir.glob(pattern):
                try:
                    print(f"  [DEBUG] Trying to load (py_jinwo): {f}", flush=True)
                    lib = ctypes.CDLL(str(f))
                    print(f"  [DEBUG] Loaded successfully (py_jinwo): {f}", flush=True)
                    return lib
                except OSError as e:
                    print(f"  [DEBUG] Failed to load (py_jinwo) {f}: {e}", flush=True)
                    continue
        # 搜索 py_jinwo/build 目录
        build_dir = py_jinwo_dir / "build"
        if build_dir.exists() and build_dir.is_dir():
            for pattern in patterns:
                for f in build_dir.glob(pattern):
                    try:
                        print(f"  [DEBUG] Trying to load (build): {f}", flush=True)
                        lib = ctypes.CDLL(str(f))
                        print(f"  [DEBUG] Loaded successfully (build): {f}", flush=True)
                        return lib
                    except OSError as e:
                        print(f"  [DEBUG] Failed to load (build) {f}: {e}", flush=True)
                        continue

    if sys.platform == "win32":
        lib_names = ["_jinwo.dll", "jinwo.dll", "libjinwo.dll"]
        ext = ".dll"
    elif sys.platform == "darwin":
        lib_names = ["libjinwo.dylib", "jinwo.dylib"]
        ext = ".dylib"
    else:
        lib_names = ["libjinwo.so", "jinwo.so"]
        ext = ".so"

    possible_paths = []
    for lib_name in lib_names:
        possible_paths.append(package_dir / lib_name)
        possible_paths.append(package_dir / "py_jinwo" / lib_name)
        possible_paths.append(package_dir.parent / lib_name)

    for lib_name in lib_names:
        possible_paths.append(Path("/usr/local/lib") / lib_name)
        possible_paths.append(Path("/usr/lib") / lib_name)

    for env_var in ["LD_LIBRARY_PATH", "PATH"]:
        if env_var in os.environ:
            for p in os.environ[env_var].split(os.pathsep):
                for lib_name in lib_names:
                    possible_paths.append(Path(p) / lib_name)

    for path in possible_paths:
        if path.exists():
            try:
                lib = ctypes.CDLL(str(path))
                return lib
            except OSError:
                continue

    raise ImportError(
        f"Failed to load JinWo dynamic library. "
        f"Please ensure jinwo_vecdb is correctly installed.\n"
        f"Attempted paths: {[str(p) for p in possible_paths]}"
    )


_lib = None

def _get_lib():
    global _lib
    if _lib is None:
        print("  [DEBUG] Loading library...", flush=True)
        _lib = _load_library()
        print("  [DEBUG] Library loaded, setting function signatures...", flush=True)

        # jw_vecdb_open(jw_str_t* path, uint32 flags, jw_vecdb_t** db) -> status
        _lib.jw_vecdb_open.restype = ctypes.c_int
        _lib.jw_vecdb_open.argtypes = [ctypes.POINTER(jw_str_t), ctypes.c_uint32, ctypes.POINTER(ctypes.c_void_p)]

        # jw_vecdb_close(jw_vecdb_t* db) -> status
        _lib.jw_vecdb_close.restype = ctypes.c_int
        _lib.jw_vecdb_close.argtypes = [ctypes.c_void_p]

        # jw_vecdb_sync(jw_vecdb_t* db) -> status
        _lib.jw_vecdb_sync.restype = ctypes.c_int
        _lib.jw_vecdb_sync.argtypes = [ctypes.c_void_p]

        # jw_vecdb_create_collection(db, jw_str_t* name, dim, jw_collection_t** coll) -> status
        _lib.jw_vecdb_create_collection.restype = ctypes.c_int
        _lib.jw_vecdb_create_collection.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t), ctypes.c_uint32, ctypes.POINTER(ctypes.c_void_p)]

        # jw_vecdb_get_collection(db, jw_str_t* name) -> jw_collection_t*
        _lib.jw_vecdb_get_collection.restype = ctypes.c_void_p
        _lib.jw_vecdb_get_collection.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t)]

        # jw_vecdb_drop_collection(db, jw_str_t* name) -> status
        _lib.jw_vecdb_drop_collection.restype = ctypes.c_int
        _lib.jw_vecdb_drop_collection.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t)]

        # jw_vecdb_list_collections(db, jw_str_t* names, capacity) -> count
        _lib.jw_vecdb_list_collections.restype = ctypes.c_size_t
        _lib.jw_vecdb_list_collections.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t), ctypes.c_size_t]

        # jw_vecdb_insert(db, jw_str_t* coll_name, cvec vec, dim, jw_vid_t* vid) -> status
        _lib.jw_vecdb_insert.restype = ctypes.c_int
        _lib.jw_vecdb_insert.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t), ctypes.POINTER(ctypes.c_float), ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint64)]

        # jw_vecdb_insert_batch(db, jw_str_t* coll_name, cvec vectors, dim, count, jw_vid_t* vids) -> status
        _lib.jw_vecdb_insert_batch.restype = ctypes.c_int
        _lib.jw_vecdb_insert_batch.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t), ctypes.POINTER(ctypes.c_float), ctypes.c_uint32, ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint64)]

        # jw_vecdb_search(db, jw_str_t* coll_name, cvec query, dim, k, results) -> count
        _lib.jw_vecdb_search.restype = ctypes.c_size_t
        _lib.jw_vecdb_search.argtypes = [ctypes.c_void_p, ctypes.POINTER(jw_str_t), ctypes.POINTER(ctypes.c_float), ctypes.c_uint32, ctypes.c_size_t, ctypes.POINTER(jw_search_result_t)]

        # jw_vecdb_version(void) -> jw_str_t (returns struct by value)
        _lib.jw_vecdb_version.restype = jw_str_t
        _lib.jw_vecdb_version.argtypes = []

        print("  [DEBUG] Function signatures set successfully", flush=True)

    return _lib


#==============================================================================
# Collection 类
#==============================================================================

class Collection:
    """向量 Collection"""

    def __init__(self, db: 'JinWoDB', name: str, dim: int, handle: int):
        self._db = db
        self._name = name
        self._dim = dim
        self._handle = handle

    @property
    def name(self) -> str:
        """Collection 名称"""
        return self._name

    @property
    def dim(self) -> int:
        """向量维度"""
        return self._dim

    def insert(self, vector: List[float]) -> int:
        """
        插入单个向量

        Args:
            vector: 向量数据列表

        Returns:
            向量 ID
        """
        if len(vector) != self._dim:
            raise ValueError(f"向量维度不匹配: 期望 {self._dim}, 实际 {len(vector)}")
        lib = _get_lib()
        name_str = _bytes_to_jwstr(self._name.encode('utf-8'))
        vec_arr = (ctypes.c_float * len(vector))(*vector)
        vid_out = ctypes.c_uint64()
        status = lib.jw_vecdb_insert(
            self._db._handle,
            ctypes.byref(name_str),
            vec_arr,
            self._dim,
            ctypes.byref(vid_out)
        )
        if status != JW_SUCCESS:
            raise RuntimeError(f"插入向量失败, 状态码: {status}")
        return vid_out.value

    def insert_batch(self, vectors: List[List[float]]) -> List[int]:
        """
        批量插入向量

        Args:
            vectors: 向量数据列表的列表

        Returns:
            向量 ID 列表
        """
        if not vectors:
            return []
        count = len(vectors)
        first_dim = len(vectors[0])
        if first_dim != self._dim:
            raise ValueError(f"向量维度不匹配: 期望 {self._dim}, 实际 {first_dim}")

        for i, vec in enumerate(vectors):
            if len(vec) != self._dim:
                raise ValueError(f"第 {i} 个向量维度不匹配: 期望 {self._dim}, 实际 {len(vec)}")

        lib = _get_lib()
        name_str = _bytes_to_jwstr(self._name.encode('utf-8'))

        # 将所有向量展平为一个连续 float 数组
        flat = [v for vec in vectors for v in vec]
        vec_arr = (ctypes.c_float * len(flat))(*flat)
        vids_buf = (ctypes.c_uint64 * count)()

        status = lib.jw_vecdb_insert_batch(
            self._db._handle,
            ctypes.byref(name_str),
            vec_arr,
            self._dim,
            count,
            vids_buf
        )
        if status != JW_SUCCESS:
            raise RuntimeError(f"批量插入失败, 状态码: {status}")
        return list(vids_buf)

    def search(self, query: List[float], k: int = 10) -> List[Tuple[int, float]]:
        """
        搜索相似向量

        Args:
            query: 查询向量
            k: 返回结果数量

        Returns:
            [(vid, distance), ...] 列表，按距离升序排列
        """
        if len(query) != self._dim:
            raise ValueError(f"查询向量维度不匹配: 期望 {self._dim}, 实际 {len(query)}")

        lib = _get_lib()
        name_str = _bytes_to_jwstr(self._name.encode('utf-8'))
        query_arr = (ctypes.c_float * len(query))(*query)
        results_buf = (jw_search_result_t * k)()

        count = lib.jw_vecdb_search(
            self._db._handle,
            ctypes.byref(name_str),
            query_arr,
            self._dim,
            k,
            results_buf
        )
        return [(int(results_buf[i].id), float(results_buf[i].score)) for i in range(count)]


#==============================================================================
# JinWoDB 类
#==============================================================================

class JinWoDB:
    """
    JinWo VecDB 数据库类

    用法:
        # 打开数据库
        db = JinWoDB("my_vecs.jwv")

        # 创建 Collection
        coll = db.create_collection("documents", dim=1536)

        # 插入向量
        vid = coll.insert([0.1] * 1536)

        # 搜索
        results = coll.search([0.1] * 1536, k=5)

        # 关闭
        db.close()
    """

    def __init__(self, path: str = "", flags: int = JW_VECDB_CREATE | JW_VECDB_READWRITE):
        """
        打开或创建数据库

        Args:
            path: 数据库文件路径，空字符串表示内存数据库
            flags: 打开标志

        Raises:
            RuntimeError: 数据库打开失败
        """
        self._path = path
        self._flags = flags
        self._collection_dims = {}  # 缓存 Collection 维度: name -> dim
        lib = _get_lib()

        if path:
            path_str = _bytes_to_jwstr(path.encode('utf-8'))
        else:
            path_str = jw_str_t(ctypes.c_char_p(None), 0)

        db_out = ctypes.c_void_p()
        status = lib.jw_vecdb_open(ctypes.byref(path_str), flags, ctypes.byref(db_out))
        if status != JW_SUCCESS or not db_out.value:
            raise RuntimeError(f"无法打开数据库, 状态码: {status}")
        self._handle = db_out.value

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

    def close(self):
        """关闭数据库"""
        if hasattr(self, '_handle') and self._handle:
            lib = _get_lib()
            lib.jw_vecdb_close(self._handle)
            self._handle = None

    def sync(self):
        """同步数据到磁盘"""
        lib = _get_lib()
        lib.jw_vecdb_sync(self._handle)

    def create_collection(self, name: str, dim: int) -> Collection:
        """
        创建 Collection

        Args:
            name: Collection 名称
            dim: 向量维度

        Returns:
            Collection 实例

        Raises:
            RuntimeError: 创建失败
        """
        lib = _get_lib()
        name_str = _bytes_to_jwstr(name.encode('utf-8'))
        coll_out = ctypes.c_void_p()
        status = lib.jw_vecdb_create_collection(
            self._handle,
            ctypes.byref(name_str),
            dim,
            ctypes.byref(coll_out)
        )
        if status != JW_SUCCESS or not coll_out.value:
            raise RuntimeError(f"无法创建 Collection: {name}, 状态码: {status}")
        self._collection_dims[name] = dim  # 缓存维度
        return Collection(self, name, dim, coll_out.value)

    def get_collection(self, name: str) -> Optional[Collection]:
        """
        获取 Collection

        Args:
            name: Collection 名称

        Returns:
            Collection 实例，不存在返回 None
        """
        lib = _get_lib()
        name_str = _bytes_to_jwstr(name.encode('utf-8'))
        handle = lib.jw_vecdb_get_collection(self._handle, ctypes.byref(name_str))
        if not handle:
            return None
        dim = self._collection_dims.get(name, 0)
        return Collection(self, name, dim, handle)

    def drop_collection(self, name: str) -> bool:
        """
        删除 Collection

        Args:
            name: Collection 名称

        Returns:
            是否成功
        """
        lib = _get_lib()
        name_str = _bytes_to_jwstr(name.encode('utf-8'))
        status = lib.jw_vecdb_drop_collection(self._handle, ctypes.byref(name_str))
        return status == JW_SUCCESS

    def list_collections(self) -> List[str]:
        """
        列出所有 Collection

        Returns:
            Collection 名称列表
        """
        lib = _get_lib()

        # 先用大容量获取实际数量
        MAX_COLLECTIONS = 256
        names_buf = (jw_str_t * MAX_COLLECTIONS)()
        count = lib.jw_vecdb_list_collections(self._handle, names_buf, MAX_COLLECTIONS)
        return [_jwstr_to_str(names_buf[i]) for i in range(count)]

    def insert(self, collection: str, vector: List[float]) -> int:
        """
        快速插入向量（自动创建/获取 Collection）

        注意: 需要先创建过该 Collection 以确定维度

        Args:
            collection: Collection 名称
            vector: 向量数据

        Returns:
            向量 ID
        """
        coll = self.get_collection(collection)
        if coll is None:
            raise ValueError(f"Collection 不存在: {collection}")
        if coll.dim == 0:
            raise ValueError(f"无法确定 Collection 维度，请先使用 create_collection")
        return coll.insert(vector)

    def insert_batch(self, collection: str, vectors: List[List[float]]) -> List[int]:
        """
        快速批量插入向量

        Args:
            collection: Collection 名称
            vectors: 向量列表

        Returns:
            向量 ID 列表
        """
        coll = self.get_collection(collection)
        if coll is None:
            raise ValueError(f"Collection 不存在: {collection}")
        return coll.insert_batch(vectors)

    def search(self, collection: str, query: List[float], k: int = 10) -> List[Tuple[int, float]]:
        """
        快速搜索

        Args:
            collection: Collection 名称
            query: 查询向量
            k: 返回结果数

        Returns:
            [(vid, distance), ...]
        """
        coll = self.get_collection(collection)
        if coll is None:
            raise ValueError(f"Collection 不存在: {collection}")
        return coll.search(query, k)


#==============================================================================
# 版本信息
#==============================================================================

def _get_version() -> str:
    """获取 JinWo C 库版本"""
    lib = _get_lib()
    ver = lib.jw_vecdb_version()
    return _jwstr_to_str(ver)