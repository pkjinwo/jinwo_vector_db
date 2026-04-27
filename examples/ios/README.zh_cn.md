# JinWo VecDB iOS 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 iOS 示例，演示如何使用 Swift 调用 C API 来使用向量数据库。

## 前置条件

- Xcode 15.0+
- iOS 13.0+
- 已构建的 JinWo VecDB 静态库（`libjw_vecdb.a`）

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 打开 Xcode 项目

```bash
# 打开 Xcode 项目
cd ../examples/ios
open JinWoVecDBiOSDemo.xcodeproj
```

### 3. 构建和运行

- 在 Xcode 中，选择目标设备（模拟器或真机）
- 点击 Run 或按 Cmd+R
- 或使用命令行构建：
  ```bash
  xcodebuild -project JinWoVecDBiOSDemo.xcodeproj -scheme JinWoVecDBiOSDemo -destination 'platform=iOS Simulator,name=iPhone 15' build
  ```

## 功能演示

iOS 示例演示了以下功能：

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

- `JinWoVecDBiOSDemo/Sources/iOSDemo.swift` - Swift 演示程序
- `JinWoVecDBiOSDemo/Sources/ViewController.swift` - 视图控制器
- `JinWoVecDBiOSDemo/Sources/AppDelegate.swift` - 应用委托
- `JinWoVecDBiOSDemo.xcodeproj` - Xcode 项目文件

## 技术实现

- 使用 Swift 的 C 桥接功能调用原生库
- 封装 C API，提供 Swift 可调用的接口
- 直接调用 JinWo VecDB C API

## 故障排除

### 静态库找不到
- **问题**: 找不到 `libjw_vecdb.a`
- **解决**: 确保库已正确构建并集成到 Xcode 项目中

### 签名问题
- **问题**: 代码签名失败
- **解决**: 确保在 Xcode 中正确配置了开发者证书

### iOS 版本问题
- **问题**: iOS 版本不兼容
- **解决**: 确保目标设备运行 iOS 13.0 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [iOS 集成指南](../../docs/zh_cn/ios_integration.zh_cn.md)
