"""
JinWo VecDB - Python 绑定

嵌入式向量数据库，无需独立服务进程，零配置、零依赖、跨平台。

安装:
    pip install jinwo_vecdb

快速开始:
    import jinwo_vecdb

    # 打开/创建数据库
    db = jinwo_vecdb.open("my_vecs.jwv")

    # 插入向量
    vid = db.insert("documents", [0.1] * 1536)

    # 批量插入
    vectors = [[0.1] * 1536, [0.2] * 1536]
    vids = db.insert_batch("documents", vectors)

    # 搜索
    results = db.search("documents", [0.1] * 1536, k=5)
    for vid, distance in results:
        print(f"vid={vid}, distance={distance}")

    # 关闭
    db.close()
"""

from .jinwo import JinWoDB, Collection, _get_version

# 版本号直接取自 C 核心 (jw_vecdb_version)，与 include/jw_types.h 的 JW_VERSION_* 恒一致，
# 避免这里硬编码造成漂移（曾因硬编码停留在 0.1.33 与实际发布版本脱节）。
try:
    import re as _re
    _m = _re.search(r"(\d+\.\d+\.\d+)", _get_version())
    __version__ = _m.group(1) if _m else _get_version()
except Exception:
    try:
        from importlib.metadata import version as _pkg_version
        __version__ = _pkg_version("jinwo_vecdb")
    except Exception:
        __version__ = "unknown"

__all__ = ["JinWoDB", "Collection", "open", "__version__"]


def open(path: str = "", flags: int = 0) -> JinWoDB:
    """
    打开或创建数据库

    Args:
        path: 数据库文件路径，默认为空字符串(内存数据库)
        flags: 打开标志，0 表示自动判断（文件存在→打开，不存在→创建）
               也可手动指定 JW_VECDB_READONLY / JW_VECDB_READWRITE 等

    Returns:
        JinWoDB 实例

    示例:
        # 内存数据库
        db = jinwo_vecdb.open()

        # 文件数据库（自动判断创建/打开）
        db = jinwo_vecdb.open("my_vecs.jwv")
    """
    return JinWoDB(path, flags)
