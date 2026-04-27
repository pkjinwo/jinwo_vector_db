# JinWo VecDB Python 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Python 示例，演示如何使用 Python 和 ctypes 调用 C API 来使用向量数据库。

## 前置条件

- Python 3.6+
- 已构建的 JinWo VecDB 库
- 对于 Linux: `libjw_vecdb.so` 在 `../../build/`
- 对于 Windows: `jw_vecdb.dll` 在 `../../build/`

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 运行 Python 示例

```bash
# 运行 Python 示例
cd ../examples/python
python python_demo.py
```

## 功能演示

Python 示例演示了以下功能：

- **数据库操作**
  - 打开/关闭数据库
  - 创建新数据库或打开现有数据库

- **集合操作**
  - 创建集合
  - 列出所有集合

- **向量操作**
  - 插入向量
  - 搜索相似向量（KNN 搜索）

- **版本信息**
  - 获取 JinWo VecDB 版本号

## 代码结构

- `python_demo.py` - Python 演示程序
- `requirements.txt` - Python 依赖项

## 技术实现

- 使用 Python 的 ctypes 库调用 C API
- 封装 C API，提供 Python 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### 库找不到
- **问题**: 找不到 JinWo VecDB 库
- **解决**: 确保库已正确构建并位于正确的路径（`../../build/`）

### 权限问题
- **问题**: 权限不足
- **解决**: 确保库文件有执行权限：`chmod +x ../../build/libjw_vecdb.so`

### Python 版本问题
- **问题**: Python 版本不兼容
- **解决**: 使用 Python 3.6 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Python ctypes 文档](https://docs.python.org/3/library/ctypes.html)
