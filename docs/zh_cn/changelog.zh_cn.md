# JinWo VecDB v1.0.0 发布说明

**版本**: v1.0.0
**发布日期**: 2026-04-26
**文档类型**: 对外发布

---

## 一、版本简介

JinWo VecDB v1.0.0 是项目的首个正式发布版本（GA）。本版本提供了一套完整的嵌入式向量数据库解决方案，支持向量存储、索引、检索等核心功能。

### 1.1 主要特性

- **高性能向量操作**: 支持 SIMD 加速的向量运算
- **多种索引算法**: 支持暴力搜索、IVF、PQ 等索引
- **跨平台支持**: Linux、macOS、iOS、Android、Windows
- **轻量级设计**: 最小依赖，适合嵌入式和移动端
- **简单易用**: 提供简洁的 C API

### 1.2 版本号说明

| 位置 | 值 | 说明 |
|------|-----|------|
| 主版本号 | 1 | 重大版本，表示不兼容的API变更 |
| 次版本号 | 0 | 次要版本，表示功能新增但兼容 |
| 修订号 | 0 | 修订版本，表示bug修复 |

---

## 二、功能清单

### 2.1 核心功能

| 功能 | 描述 | 状态 |
|------|------|------|
| 数据库管理 | 创建、打开、关闭数据库 | ✅ 已实现 |
| 集合管理 | 创建、删除、列举集合 | ✅ 已实现 |
| 向量插入 | 单次和批量插入向量 | ✅ 已实现 |
| 向量搜索 | Top-K 相似向量搜索 | ✅ 已实现 |
| 向量删除 | 删除指定ID的向量 | ✅ 已实现 |
| 数据持久化 | 自动持久化到磁盘 | ✅ 已实现 |

### 2.2 索引功能

| 功能 | 描述 | 状态 |
|------|------|------|
| 暴力搜索 | 线性扫描搜索 | ✅ 已实现 |
| IVF索引 | 倒排文件索引 | ✅ 已实现 |
| PQ量化 | 产品量化压缩 | ✅ 已实现 |
| 距离度量 | L2、余弦、点积 | ✅ 已实现 |

### 2.3 高级功能

| 功能 | 描述 | 状态 |
|------|------|------|
| 内存池 | Arena 内存池管理 | ✅ 已实现 |
| 线程安全 | 多线程并发访问 | ✅ 已实现 |
| 错误处理 | 统一的错误码体系 | ✅ 已实现 |
| 日志系统 | 可配置的日志输出 | ✅ 已实现 |

---

## 三、平台支持

### 3.1 支持的平台

| 平台 | 架构 | 最低版本 | 编译方式 |
|------|------|----------|----------|
| Linux | x86_64, ARM | GCC 9.0+ | CMake |
| macOS | x86_64, arm64 | Xcode 13.0+ | CMake/Xcode |
| iOS | arm64, x86_64 | iOS 12.0+ | Xcode |
| Android | armeabi-v7a, arm64-v8a, x86, x86_64 | API 21+ | CMake/NDK |
| Windows | x86, x64 | VS 2019+ | CMake/VS |

### 3.2 第三方依赖

| 依赖 | 版本 | 用途 | 平台 |
|------|------|------|------|
| CMake | 3.10+ | 构建系统 | ALL |
| pthread | - | 线程支持 | Unix |
| libc | - | C标准库 | ALL |

---

## 四、安装指南

### 4.1 Linux/macOS

```bash
# 下载源码
git clone https://github.com/pkjinwo/jinwo_vector_db.git
cd jinwo_vector_db

# 创建构建目录
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 安装
sudo make install
```

### 4.2 iOS

```bash
# 配置 iOS 构建
mkdir build && cd build
cmake .. \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path) \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### 4.3 Android

```bash
# 配置 Android NDK 构建
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### 4.4 Windows

```powershell
# 使用 Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 或使用 MinGW
cmake .. -G "MinGW Makefiles"
mingw32-make
```

---

## 五、快速开始

### 5.1 基础使用

```c
#include "jw_vecdb.h"
#include <stdio.h>

int main() {
    jw_vecdb_t* db = NULL;
    jw_vecdb_open(&db, "/tmp/vecdb", true);

    // 创建集合
    jw_collection_t* col = NULL;
    jw_collection_create(&col, db, "vectors", 128);

    // 插入向量
    float vector[128];
    for (int i = 0; i < 128; i++) {
        vector[i] = (float)rand() / RAND_MAX;
    }
    uint64_t id;
    jw_collection_insert(col, vector, &id);

    // 搜索向量
    float query[128];
    for (int i = 0; i < 128; i++) {
        query[i] = (float)rand() / RAND_MAX;
    }
    jw_search_result_t results[10];
    size_t count;
    jw_collection_search(col, query, 10, results, &count);

    printf("Found %zu results\n", count);

    // 清理
    jw_collection_close(col);
    jw_vecdb_close(db);

    return 0;
}
```

---

## 六、变更日志

### 6.1 v1.0.0 (2026-04-26)

**新功能**:
- ✅ 数据库打开和关闭
- ✅ 集合创建、删除、列举
- ✅ 向量插入、搜索、删除
- ✅ IVF 索引支持
- ✅ PQ 量化支持
- ✅ 多种距离度量（L2、余弦、点积）
- ✅ Arena 内存池
- ✅ 线程安全支持
- ✅ 数据持久化

**平台支持**:
- ✅ Linux 支持
- ✅ macOS 支持
- ✅ iOS 支持
- ✅ Android 支持
- ✅ Windows 支持

**文档**:
- ✅ API 参考文档
- ✅ 集成指南（iOS、Android、Windows）
- ✅ 架构设计文档
- ✅ 安全审计指南
- ✅ 性能测试指南
- ✅ 命名规范文档
- ✅ 验证指南
- ✅ 跨平台测试指南
- ✅ 回归测试指南

---

## 七、已知问题

| 问题 | 描述 | 严重程度 | 状态 |
|------|------|----------|------|
| 暂无 | - | - | - |

---

## 八、路线图

### 8.1 计划中的功能

| 功能 | 目标版本 | 描述 |
|------|-----------|------|
| HNSW 索引 | v1.1.0 | 图索引支持 |
| 分布式支持 | v2.0.0 | 集群部署 |
| REST API | v1.2.0 | HTTP 接口 |
| Python 绑定 | v1.1.0 | Python 语言绑定 |
| Go 绑定 | v1.2.0 | Go 语言绑定 |

### 8.2 性能优化

| 优化项 | 目标版本 | 描述 |
|------|-----------|------|
| SIMD 优化 | v1.1.0 | AVX2/NEON 加速 |
| 索引优化 | v1.1.0 | 搜索性能提升 |
| 内存优化 | v1.2.0 | 内存占用降低 |

---

## 九、反馈和支持

### 9.1 反馈渠道

| 渠道 | 描述 |
|------|------|
| GitHub Issues | 功能请求和Bug报告 |
| GitHub Discussions | 讨论和问答 |
| 邮件 | 官方支持邮箱 |

### 9.2 资源链接

| 资源 | 链接 |
|------|------|
| 项目主页 | https://github.com/pkjinwo/jinwo_vector_db |
| 文档 | https://jinwovecdb.github.io/docs |
| API 参考 | docs/api_reference.md |

---

## 十、许可证

JinWo VecDB 使用 Apache License 2.0 许可证。详见 LICENSE 文件。

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始发布版本 |
