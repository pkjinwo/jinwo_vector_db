# JinWo VecDB Python 示例

**版本**: v1.0.0
**更新日期**: 2026-10-10

## 概述

本目录包含 JinWo VecDB 的 Python 示例，**两种用法并存**：

1. **高层 API（推荐）**：`python_demo.py` —— 使用官方 `jinwo_vecdb` 包
   （`pip install` 安装，底层为跨平台编译的本地扩展），几行代码即可上手。
2. **低层 FFI**：`python_demo_ctypes.py` —— 不安装 pip 包，直接从源码构建
   C 库后用 `ctypes` 调用 `libjinwo`，适合嵌入式集成或调试底层 ABI。

## 前置条件

- Python 3.6+
- 高层 API 用法：已安装官方包 `pip install jinwo_vecdb`
- 低层 FFI 用法：已构建 C 库（`cmake -S . -B build && cmake --build build`）

## 快速开始（推荐：高层 API）

### 1. 安装依赖

```bash
cd examples/python
pip install -r requirements.txt
```

### 2. 运行 Python 示例

```bash
python python_demo.py
```

## 备选：低层 FFI（ctypes 直调编译库）

```bash
# 先回到仓库根目录构建 C 库
cmake -S . -B build && cmake --build build

# 再运行 ctypes 示例
cd examples/python
python python_demo_ctypes.py
```

## 功能演示

Python 示例演示了以下功能：

- **数据库操作**：打开/关闭数据库（`jinwo_vecdb.open` / `db.close`）
- **集合操作**：创建集合（`db.create_collection`）
- **向量操作**：插入向量（`db.insert` / `db.insert_batch`）、相似度搜索
  （KNN，`db.search`）、删除向量（`db.delete`）
- **版本信息**：获取版本号（`jinwo_vecdb.__version__`）

## 代码结构

- `python_demo.py` - Python 演示程序（使用 `jinwo_vecdb` 高层 API，推荐）
- `python_demo_ctypes.py` - Python 演示程序（ctypes 直调编译库，低层 FFI）
- `requirements.txt` - Python 依赖项（高层 API 用）

## 相关文档

- [多语言使用示例](../../docs/zh_cn/usage_examples.zh_cn.md)
- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
