# JinWo VecDB Android 示例

**版本**: v1.0.0
**更新日期**: 2026-04-26

## 概述

本目录包含 JinWo VecDB 的 Android 示例，演示如何使用 Java 和 JNI 调用 C API 来使用向量数据库。

## 前置条件

- Android Studio Hedgehog (2023.1.1) 或更高版本
- Android NDK r25+
- Android SDK API 34
- 已构建的 JinWo VecDB 库

## 快速开始

### 1. 构建 JinWo VecDB 库

```bash
# 首先构建 JinWo VecDB 库
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. 打开 Android 项目

```bash
# 打开 Android 项目
cd ../examples/android
# 在 Android Studio 中打开项目
```

### 3. 构建和运行

- 在 Android Studio 中，选择目标设备
- 点击 Run 或按 Shift+F10
- 或使用命令行构建：
  ```bash
  ./gradlew assembleDebug
  adb install app/build/outputs/apk/debug/app-debug.apk
  ```

## 功能演示

Android 示例演示了以下功能：

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

- `app/src/main/java/com/jinwo/vecdb/demo/MainActivity.java` - 主活动类
- `app/src/main/jni/android_demo_jni.cpp` - JNI 包装器
- `app/build.gradle` - 应用级构建配置
- `build.gradle` - 项目级构建配置

## 技术实现

- **Java 层**：通过 JNI 调用原生库
- **JNI 层**：封装 C API，提供 Java 可调用的接口
- **原生层**：直接调用 JinWo VecDB C API

## 故障排除

### NDK 构建失败
- **问题**: NDK 构建失败
- **解决**: 确保 NDK 已正确安装并在 Android Studio 中配置

### 库文件找不到
- **问题**: 找不到 JinWo VecDB 库
- **解决**: 确保库已正确构建并在 CMakeLists.txt 中正确引用

### API 级别问题
- **问题**: API 级别不兼容
- **解决**: 确保使用 API 23 或更高版本

## 相关文档

- [API 参考文档](../../docs/zh_cn/api_reference.zh_cn.md)
- [Android 集成指南](../../docs/zh_cn/android_integration.zh_cn.md)
