# JinWo VecDB Kotlin 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Kotlin 示例，演示如何使用 Kotlin 和 JNA 调用 C API 来使用向量数据库。

## 前置条件

- JDK 11+
- Gradle 7.0+
- 已构建的 JinWo VecDB 库

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 运行 Kotlin 示例

```bash
# 运行 Kotlin 示例
cd ../examples/kotlin
./gradlew run
```

## 功能演示

Kotlin 示例演示了以下功能：

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

- `build.gradle.kts` - Gradle 构建配置
- `src/main/kotlin/Main.kt` - Kotlin 演示程序

## 技术实现

- 使用 Kotlin 的 JNA 库调用 C API
- 封装 C API，提供 Kotlin 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### 库找不到
- **问题**: 找不到 JinWo VecDB 库
- **解决**: 确保库已正确构建并位于正确的路径（`../../build/`）

### 编译错误
- **问题**: 编译失败
- **解决**: 确保 JDK 11+ 和 Gradle 7.0+ 已安装

### JNA 相关问题
- **问题**: JNA 加载失败
- **解决**: 确保 build.gradle.kts 中已正确配置 JNA 依赖

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [JNA 文档](https://github.com/java-native-access/jna)
