# JinWo VecDB Linux 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Linux 示例，演示如何使用 C 语言直接调用 C API 来使用向量数据库。

## 前置条件

- GCC 11+
- CMake 3.20+
- 已构建的 JinWo VecDB 库（`libjw_vecdb.a`）

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 构建和运行示例

#### 使用 CMake

```bash
# 使用 CMake 构建
cd ../examples/linux
mkdir -p build && cd build
cmake .. && make
./linux_demo
```

#### 使用 Makefile

```bash
# 使用 Makefile 构建
cd ../examples/linux
make
./linux_demo

# 或直接运行
make run
```

## 功能演示

Linux 示例演示了以下功能：

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

- `linux_demo.c` - C 语言演示程序
- `CMakeLists.txt` - CMake 构建配置
- `Makefile` - Make 构建配置

## 技术实现

- 直接调用 JinWo VecDB C API
- 使用 CMake 或 Make 构建系统
- 原生 C 语言实现

## 故障排除

### 链接错误
- **问题**: 链接时找不到库
- **解决**: 确保 JinWo VecDB 库已正确构建并位于 `../../build/libjw_vecdb.a`

### 编译错误
- **问题**: 编译失败
- **解决**: 确保 GCC 版本 11+ 已安装

### 权限问题
- **问题**: 权限不足
- **解决**: 确保有执行权限：`chmod +x linux_demo`

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
