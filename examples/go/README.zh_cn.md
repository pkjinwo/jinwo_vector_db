# JinWo VecDB Go 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Go 示例，演示如何使用 Go 和 cgo 调用 C API 来使用向量数据库。

## 前置条件

- Go 1.18+
- C 编译器（Linux 上的 gcc，Windows 上的 MinGW）
- 已构建的 JinWo VecDB 库

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 运行 Go 示例

```bash
# 运行 Go 示例
cd ../examples/go
go run go_demo.go
```

## 功能演示

Go 示例演示了以下功能：

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

- `go_demo.go` - Go 演示程序
- `go.mod` - Go 模块定义

## 技术实现

- 使用 Go 的 cgo 功能调用 C API
- 封装 C API，提供 Go 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### 库找不到
- **问题**: 链接时找不到库
- **解决**: 确保 JinWo VecDB 库已正确构建并位于 `../../build/libjw_vecdb.a`

### CGO 问题
- **问题**: CGO 相关错误
- **解决**: 确保安装了 C 编译器
  - 对于 Linux: `sudo apt install build-essential`（Ubuntu/Debian）
  - 对于 Windows: 安装 MinGW 或使用 WSL

### Go 版本问题
- **问题**: Go 版本不兼容
- **解决**: 使用 Go 1.18 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Go CGO 文档](https://golang.org/cmd/cgo/)
