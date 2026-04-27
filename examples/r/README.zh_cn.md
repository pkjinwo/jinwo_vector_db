# JinWo VecDB R 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 R 示例，演示如何使用 R 和 Rcpp 调用 C API 来使用向量数据库。

## 前置条件

- R 4.0+
- Rcpp 包
- 已构建的 JinWo VecDB 库

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 运行 R 示例

```bash
# 运行 R 示例
cd ../examples/r
Rscript r_demo.R
```

## 功能演示

R 示例演示了以下功能：

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

- `r_demo.R` - R 演示程序

## 技术实现

- 使用 R 的 Rcpp 包调用 C API
- 封装 C API，提供 R 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### 库找不到
- **问题**: 找不到 JinWo VecDB 库
- **解决**: 确保库已正确构建并位于正确的路径（`../../build/`）

### Rcpp 相关问题
- **问题**: Rcpp 编译失败
- **解决**: 确保 Rcpp 包已安装：`install.packages("Rcpp")`

### R 版本问题
- **问题**: R 版本不兼容
- **解决**: 使用 R 4.0 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Rcpp 文档](https://cran.r-project.org/web/packages/Rcpp/index.html)
