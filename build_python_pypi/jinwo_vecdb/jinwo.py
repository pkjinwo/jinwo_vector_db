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
# 动态库加载
#==============================================================================

# 标志定义
JW_VECDB_READONLY = 0x01
JW_VECDB_READWRITE = 0x02
JW_VECDB_CREATE = 0x04
JW_VECDB_TRUNCATE = 0x08
JW_VECDB_MEMORY = 0x10

# 加载动态库
def _load_library():
    """加载 JinWo C 动态库"""
    package_dir = Path(__file__).parent

    # 方式1: 尝试作为 Python C 扩展模块导入 _jinwo (放在包目录的 .pyd)
    try:
        from . import _jinwo
        if hasattr(_jinwo, 'vecdb_open'):
            return _ModuleWrapper(_jinwo)
    except ImportError:
        pass
    try:
        import _jinwo
        if hasattr(_jinwo, 'vecdb_open'):
            return _ModuleWrapper(_jinwo)
    except ImportError:
        pass

    # 方式2: 搜索 _jinwo* 文件，如果找到 Python 扩展 (.pyd/.dll 在子目录)，
    #         则将其路径加入 sys.path 后再 import
    for pattern in ["_jinwo*.pyd", "_jinwo*.dll"]:
        for f in package_dir.rglob(pattern):
            parent = str(f.parent)
            if parent not in sys.path:
                sys.path.insert(0, parent)
                try:
                    # 模块名是 _jinwo（去掉平台后缀如 .cp311-win_amd64）
                    mod_name = f.stem.split('.')[0]
                    __import__(mod_name)
                    mod = sys.modules.get(mod_name)
                    if mod and hasattr(mod, 'vecdb_open'):
                        return _ModuleWrapper(mod)
                except ImportError:
                    pass
                finally:
                    sys.path.remove(parent)

    # 方式3: 搜索包内的 _jinwo.* 文件，作为 ctypes CDLL 加载（纯 C 动态库）
    for pattern in ["_jinwo*.pyd", "_jinwo*.dll", "_jinwo*.so", "_jinwo*.dylib"]:
        for f in list(package_dir.glob(pattern)) + list(package_dir.rglob(pattern)):
            try:
                lib = ctypes.CDLL(str(f))
                if hasattr(lib, 'vecdb_open'):
                    return lib
            except OSError:
                continue

    if sys.platform == "win32":
        lib_names = ["_jinwo.dll", "jinwo.dll", "libjinwo.dll"]
    elif sys.platform == "darwin":
        lib_names = ["libjinwo.dylib", "jinwo.dylib"]
    else:
        lib_names = ["libjinwo.so", "jinwo.so"]

    possible_paths = []

    for lib_name in lib_names:
        possible_paths.append(package_dir / lib_name)
        possible_paths.append(package_dir / "py_jinwo" / lib_name)
        possible_paths.append(package_dir.parent / lib_name)
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
        f"无法加载 JinWo 动态库。请确保已正确安装 jinwo_vecdb 包。\n"
        f"尝试的路径: {[str(p) for p in possible_paths]}"
    )


class _ModuleWrapper:
    """
    Python 模块包装器，将模块函数转成 ctypes CDLL 风格的属性访问。
    
    当 _jinwo 作为 Python C 扩展模块导入时使用。
    """

    def __init__(self, module):
        self._module = module
        self._wrapped = {}

    def _wrap(self, name):
        """获取模块函数并包装为可设置 restype/argtypes 的对象"""
        func = getattr(self._module, name, None)
        if func is None:
            raise AttributeError(f"function '{name}' not found in _jinwo module")
        wrapper = _FuncWrapper(func)
        self._wrapped[name] = wrapper
        return wrapper

    def __getattr__(self, name):
        if name.startswith('_'):
            raise AttributeError(name)
        if name in self._wrapped:
            return self._wrapped[name]
        return self._wrap(name)


class _FuncWrapper:
    """包装模块函数，支持设置 restype/argtypes 属性（忽略它们）"""

    def __init__(self, func):
        self._func = func
        self.restype = None
        self.argtypes = None

    def __call__(self, *args, **kwargs):
        return self._func(*args, **kwargs)


_lib = None

def _get_lib():
    global _lib
    if _lib is None:
        _lib = _load_library()
        # 设置返回类型
        _lib.vecdb_open.restype = ctypes.c_void_p
        _lib.vecdb_open.argtypes = [ctypes.c_char_p, ctypes.c_int]

        _lib.vecdb_close.restype = None
        _lib.vecdb_close.argtypes = [ctypes.c_void_p]

        _lib.vecdb_sync.restype = None
        _lib.vecdb_sync.argtypes = [ctypes.c_void_p]

        _lib.vecdb_create_collection.restype = ctypes.c_void_p
        _lib.vecdb_create_collection.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]

        _lib.vecdb_get_collection.restype = ctypes.c_void_p
        _lib.vecdb_get_collection.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        _lib.vecdb_drop_collection.restype = ctypes.c_bool
        _lib.vecdb_drop_collection.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        _lib.vecdb_list_collections.restype = ctypes.py_object
        _lib.vecdb_list_collections.argtypes = [ctypes.c_void_p]

        _lib.vecdb_insert.restype = ctypes.c_ulonglong
        _lib.vecdb_insert.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.py_object, ctypes.c_int]

        _lib.vecdb_insert_batch.restype = ctypes.py_object
        _lib.vecdb_insert_batch.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.py_object, ctypes.c_int]

        _lib.vecdb_search.restype = ctypes.py_object
        _lib.vecdb_search.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.py_object, ctypes.c_int, ctypes.c_int]

        _lib.vecdb_version.restype = ctypes.c_char_p

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
        vid = lib.vecdb_insert(
            self._db._handle,
            self._name,
            vector,
            self._dim
        )
        return vid

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

        first_dim = len(vectors[0])
        if first_dim != self._dim:
            raise ValueError(f"向量维度不匹配: 期望 {self._dim}, 实际 {first_dim}")

        for i, vec in enumerate(vectors):
            if len(vec) != self._dim:
                raise ValueError(f"第 {i} 个向量维度不匹配: 期望 {self._dim}, 实际 {len(vec)}")

        lib = _get_lib()
        vids = lib.vecdb_insert_batch(
            self._db._handle,
            self._name,
            vectors,
            self._dim
        )
        return list(vids)

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
        results = lib.vecdb_search(
            self._db._handle,
            self._name,
            query,
            self._dim,
            k
        )
        return [(int(vid), float(dist)) for vid, dist in results]


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
        lib = _get_lib()

        self._handle = lib.vecdb_open(path, flags)
        if not self._handle:
            raise RuntimeError("无法打开数据库")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

    def close(self):
        """关闭数据库"""
        if hasattr(self, '_handle') and self._handle:
            lib = _get_lib()
            lib.vecdb_close(self._handle)
            self._handle = None

    def sync(self):
        """同步数据到磁盘"""
        lib = _get_lib()
        lib.vecdb_sync(self._handle)

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
        handle = lib.vecdb_create_collection(
            self._handle,
            name,
            dim
        )
        if not handle:
            raise RuntimeError(f"无法创建 Collection: {name}")
        return Collection(self, name, dim, handle)

    def get_collection(self, name: str) -> Optional[Collection]:
        """
        获取 Collection

        Args:
            name: Collection 名称

        Returns:
            Collection 实例，不存在返回 None
        """
        lib = _get_lib()
        handle = lib.vecdb_get_collection(self._handle, name)
        if not handle:
            return None
        return Collection(self, name, 0, handle)

    def drop_collection(self, name: str) -> bool:
        """
        删除 Collection

        Args:
            name: Collection 名称

        Returns:
            是否成功
        """
        lib = _get_lib()
        return lib.vecdb_drop_collection(self._handle, name)

    def list_collections(self) -> List[str]:
        """
        列出所有 Collection

        Returns:
            Collection 名称列表
        """
        lib = _get_lib()
        names = lib.vecdb_list_collections(self._handle)
        result = []
        seen = set()
        for n in names:
            if isinstance(n, bytes):
                n = n.split(b'\x00')[0].decode('utf-8')
            else:
                n = str(n).split('\x00')[0]
            if n and n not in seen:
                seen.add(n)
                result.append(n)
        return result

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
    version = lib.vecdb_version()
    if isinstance(version, bytes):
        return version.decode('utf-8')
    return str(version)
