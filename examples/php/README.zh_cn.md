# JinWo VecDB PHP 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 PHP 示例，演示如何使用 PHP 和 FFI 扩展调用 C API 来使用向量数据库。

## 前置条件

- PHP 7.4+（启用 FFI 扩展）
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

### 2. 启用 FFI 扩展

确保在 `php.ini` 文件中启用 FFI 扩展：

```ini
; php.ini
; 取消注释以下行以启用 FFI 扩展
extension=ffi
```

### 3. 运行 PHP 示例

```bash
# 运行 PHP 示例
cd ../examples/php
php php_demo.php
```

## 功能演示

PHP 示例演示了以下功能：

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

- `php_demo.php` - PHP 演示程序

## 技术实现

- 使用 PHP 的 FFI 扩展调用 C API
- 封装 C API，提供 PHP 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### FFI 扩展不可用
- **问题**: `Error: PHP FFI extension is not available`
- **解决**: 在 `php.ini` 文件中启用 FFI 扩展

### 库找不到
- **问题**: `Error: Could not find JinWo VecDB library`
- **解决**: 确保库已正确构建并位于正确的路径（`../../build/`）

### PHP 版本问题
- **问题**: PHP 版本不兼容
- **解决**: 使用 PHP 7.4 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [PHP FFI 文档](https://www.php.net/manual/en/book.ffi.php)
