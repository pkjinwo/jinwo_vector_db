# JinWo VecDB Windows 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Windows 示例，演示如何使用 C# 和 P/Invoke 调用 C API 来使用向量数据库。

## 前置条件

- Visual Studio 2022
- .NET 8.0 SDK
- 已构建的 JinWo VecDB 库（`jw_vecdb.dll`）

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && cmake --build . --config Release
```

### 2. 打开 Visual Studio 项目

```bash
# 打开 Visual Studio 项目
cd ../examples/windows
# 在 Visual Studio 中打开 JinWoVecDBDemo.sln
```

### 3. 构建和运行

- 在 Visual Studio 中，选择 Release 配置
- 点击 Build > Build Solution 或按 F7
- 点击 Debug > Start Without Debugging 或按 Ctrl+F5
- 或使用命令行构建：
  ```bash
  dotnet build JinWoVecDBDemo.csproj -c Release
  cd bin/Release/net8.0
  ./JinWoVecDBDemo.exe
  ```

## 功能演示

Windows 示例演示了以下功能：

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

- `windows_demo.cs` - C# 演示程序
- `JinWoVecDBDemo.sln` - Visual Studio 解决方案
- `JinWoVecDBDemo.csproj` - Visual Studio 项目文件

## 技术实现

- 使用 C# 的 P/Invoke 功能调用原生 DLL
- 封装 C API，提供 C# 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### DLL 找不到
- **问题**: 找不到 `jw_vecdb.dll`
- **解决**: 确保 DLL 文件在可执行文件目录或系统 PATH 中

### .NET 版本问题
- **问题**: .NET 版本不兼容
- **解决**: 确保安装了 .NET 8.0 SDK

### 平台目标问题
- **问题**: 平台目标不匹配
- **解决**: 确保项目设置为 x64 平台

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Windows 集成指南](../../docs/zh_cn/windows_integration.zh_cn.md)
