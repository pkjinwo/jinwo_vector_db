# JinWo VecDB TypeScript 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 TypeScript 示例，演示如何使用 TypeScript 和 ffi-napi 调用 C API 来使用向量数据库。

## 前置条件

- Node.js 14+
- npm 6+
- 已构建的 JinWo VecDB 库

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 安装依赖

```bash
# 安装依赖
cd ../examples/typescript
npm install
```

### 3. 运行 TypeScript 示例

```bash
# 运行 TypeScript 示例
npm start
```

## 功能演示

TypeScript 示例演示了以下功能：

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

- `package.json` - Node.js 项目配置
- `tsconfig.json` - TypeScript 配置
- `src/index.ts` - TypeScript 演示程序

## 技术实现

- 使用 TypeScript 的 ffi-napi 库调用 C API
- 封装 C API，提供 TypeScript 可调用的接口
- 直接调用 JinWo VecDB C API

## 依赖

- `ffi-napi` - Node.js 外部函数接口，用于调用 C 函数
- `ref-napi` - 引用处理，用于 C 类型
- `ref-struct-napi` - 结构体处理，用于 C 结构体

## 故障排除

### 库找不到
- **问题**: 找不到 JinWo VecDB 库
- **解决**: 确保库已正确构建并位于正确的路径（`../../build/`）

### FFI 安装问题
- **问题**: 安装 ffi-napi 或相关包时出现问题
- **解决**:
  - 对于 Linux: `sudo apt install build-essential`（Ubuntu/Debian）
  - 对于 Windows: 安装 Visual Studio Build Tools
  - 对于 macOS: 安装 Xcode Command Line Tools

### Node.js 版本问题
- **问题**: Node.js 版本不兼容
- **解决**: 使用 Node.js 14 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [ffi-napi 文档](https://github.com/node-ffi-napi/node-ffi-napi)
