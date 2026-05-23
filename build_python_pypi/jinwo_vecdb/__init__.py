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

from .jinwo import JinWoDB, Collection

__version__ = "0.1.9"

__all__ = ["JinWoDB", "Collection", "open", "__version__"]


def open(path: str = "", flags: int = 0x06) -> JinWoDB:
    """
    打开或创建数据库

    Args:
        path: 数据库文件路径，默认为空字符串(内存数据库)
        flags: 打开标志，默认为 CREATE | READWRITE

    Returns:
        JinWoDB 实例

    示例:
        # 内存数据库
        db = jinwo_vecdb.open()

        # 文件数据库
        db = jinwo_vecdb.open("my_vecs.jwv")
    """
    return JinWoDB(path, flags)
