# JinWo VecDB (金幄向量数据库)

**JinWo VecDB** 是一个开源、跨平台、嵌入式向量数据库，专为移动端和嵌入式设备设计。

## 特性

- 🔥 **纯C实现** - C99标准，无外部依赖，易于集成
- 🚀 **跨平台** - 支持 Android, iOS, macOS, Windows, Linux (五大主流平台)
- 🎯 **高性能索引** - 支持 IVF 和 HNSW 索引算法
- ⚡ **SIMD加速** - 自动检测并使用 SSE/AVX/NEON 指令集
- 📦 **零配置** - 开箱即用，无需复杂配置
- 📊 **向量量化** - 支持 PQ (Product Quantization) 和 SQ (Scalar Quantization)
- 📱 **移动优化** - 针对移动设备和嵌入式系统进行了内存和性能优化
- 🔧 **完整API** - 提供完整的向量数据库操作API
- 🧪 **完善测试** - 包含全面的单元测试、并发测试和性能测试
- 🌐 **多语言支持** - 支持 C、C#、Java、Swift、Python、Go、Node.js、PHP、Rust、Kotlin、TypeScript、R、Dart、Julia
- 🔄 **CI/CD集成** - 完善的持续集成和持续部署流程

## 快速开始

JinWo VecDB 提供简单的 API，几行代码即可开始向量检索。

```bash
# Python
pip install jinwo-vecdb

# Node.js
npm install jinwo-vecdb

# C / 其他语言：从源码构建（产物为 libjinwo）
git clone https://gitee.com/pkjinwo/jinwo_vector_db.git   # 国内
cd jinwo_vector_db && cmake -S . -B build && cmake --build build
```

```python
import jinwo_vecdb

db = jinwo_vecdb.open("my_vectors.jwv")           # 打开/创建数据库
db.create_collection("documents", 384)            # 创建 Collection（384 维）
db.insert("documents", [0.1] * 384)               # 插入向量
results = db.search("documents", query_vec, k=10) # Top-10 相似向量
for vid, distance in results[:5]:
    print(f"vid={vid}, distance={distance:.6f}")
db.close()
```

**完整多语言快速开始（C / C++ / Python / Go / Android / iOS / Node.js 等）请参阅
[`docs/zh_cn/usage_examples.zh_cn.md`](docs/zh_cn/usage_examples.zh_cn.md)。**

## 项目结构

```
jw_vecdb/
├── include/           # 头文件
│   ├── jw_types.h     # 基础类型定义
│   ├── jw_arena.h     # 内存池
│   ├── jw_vector.h    # 向量操作
│   ├── jw_lock.h      # 锁机制
│   ├── jw_index.h     # 索引结构
│   ├── jw_collection.h# 向量集合
│   ├── jw_storage.h   # 存储抽象层
│   ├── jw_quant.h     # 向量量化
│   ├── jw_config.h    # 配置管理
│   ├── jw_file.h      # 文件操作
│   ├── jw_hash.h      # 哈希表
│   ├── jw_log.h       # 日志系统
│   ├── jw_math.h      # 数学工具
│   ├── jw_sort.h      # 排序算法
│   ├── jw_string.h    # 字符串操作
│   └── jw_vecdb.h     # 主接口
├── src/               # 源代码实现
├── examples/          # 示例程序
│   ├── c/             # C语言示例
│   ├── csharp/        # C#示例
│   ├── java/          # Java示例
│   ├── swift/         # Swift示例
│   ├── python/        # Python示例
│   ├── go/            # Go示例
│   ├── nodejs/        # Node.js示例
│   ├── php/           # PHP示例
│   ├── rust/          # Rust示例
│   ├── kotlin/        # Kotlin示例
│   ├── typescript/    # TypeScript示例
│   ├── r/             # R语言示例
│   ├── dart/          # Dart示例
│   ├── julia/         # Julia示例
│   ├── android/       # Android示例
│   ├── ios/           # iOS示例
│   ├── windows/       # Windows示例
│   ├── linux/         # Linux示例
│   └── macos/         # macOS示例
├── tests/             # 单元测试
│   ├── test_types.c   # 基础类型测试
│   ├── test_string.c  # 字符串测试
│   ├── test_arena.c   # 内存池测试
│   ├── test_vector.c  # 向量操作测试
│   ├── test_math.c    # 数学函数测试
│   ├── test_sort.c    # 排序算法测试
│   ├── test_hash.c    # 哈希表测试
│   ├── test_lock.c    # 锁机制测试
│   ├── test_file.c    # 文件操作测试
│   ├── test_storage.c # 存储层测试
│   ├── test_index.c   # 索引算法测试
│   ├── test_quantization.c # 量化测试
│   ├── test_config.c  # 配置测试
│   ├── test_vecdb.c   # 主接口测试
│   ├── test_collection.c # 集合测试
│   ├── test_concurrent.c # 并发测试
│   └── performance/   # 性能测试
├── cmake/             # CMake配置
├── docs/              # 文档
│   ├── en_us/         # 英文文档
│   └── zh_cn/         # 中文文档
├── local_doc/         # 本地文档
├── .github/workflows/ # CI/CD配置
└── CMakeLists.txt     # 构建配置
```

## API 设计

### 核心模块

| 模块 | 说明 |
|------|------|
| jw_types | 基础类型定义，跨平台兼容 |
| jw_arena | 内存池管理，高效内存分配 |
| jw_vector | 向量操作，支持SIMD加速 |
| jw_lock | 锁机制，支持多线程 |
| jw_index | 索引算法，IVF/HNSW |
| jw_collection | 向量集合管理 |
| jw_storage | 存储抽象层 |
| jw_quant | 向量量化，PQ/SQ支持 |
| jw_config | 配置管理 |
| jw_file | 文件操作 |
| jw_hash | 哈希表实现 |
| jw_log | 日志系统 |
| jw_math | 数学工具函数 |
| jw_sort | 排序算法 |
| jw_string | 字符串操作 |
| jw_vecdb | 主接口API |

### 索引算法

#### IVF (Inverted File Index)
- 适合大规模数据集（千万级以上）
- 内存占用小
- 通过聚类中心快速定位

#### HNSW (Hierarchical Navigable Small World)
- 查询速度快，精度高
- 适合中小规模数据（百万级）
- 图结构存储，增量更新友好

## 性能

在标准桌面环境下的基准测试结果（128维向量）：

| 操作 | 性能 |
|------|------|
| 点积 | ~10M ops/sec |
| L2距离 | ~8M ops/sec |
| 余弦相似度 | ~6M ops/sec |
| 归一化 | ~5M ops/sec |

*实际性能取决于硬件配置和SIMD支持*

## 路线图

### v0.1.32 (已完成)
- [x] 基础类型定义
- [x] 内存池管理
- [x] 向量操作（含SIMD加速）
- [x] 锁机制
- [x] 索引结构设计

### v0.2.0 (已完成)
- [x] IVF索引完整实现
- [x] HNSW索引完整实现
- [x] 存储层实现
- [x] Collection完整实现

### v0.3.0 (已完成)
- [x] PQ/SQ量化支持
- [x] 批量操作优化
- [x] 并行查询

### v1.0.0 (已完成)
- [x] 完整功能集
- [x] 稳定API
- [x] 完善文档
- [x] 全平台支持
- [x] 多语言绑定
- [x] 并发测试
- [x] CI/CD集成

## 平台支持

| 平台 | 支持状态 | 集成文档 | 示例代码 |
|------|----------|----------|----------|
| Linux | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| macOS | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| iOS | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Android | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Windows | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |

## 语言支持

| 语言 | 支持状态 | 示例代码 | 文档 |
|------|----------|----------|------|
| C | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| C# | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Java | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Swift | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Python | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Go | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Node.js | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| PHP | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Rust | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Kotlin | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| TypeScript | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| R | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Dart | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |
| Julia | ✅ 已完成 | ✅ 已提供 | ✅ 已提供 |

## 已发布版本（包管理平台）

以下语言已发布到对应包管理平台，可直接 `install` / `get` 引入；其余语言目前仅提供源码与示例，尚未发布到包管理。

| 语言 | 发布平台 | 发布方式 | 当前版本 | 引入方式示例 | 状态 |
|------|----------|----------|----------|--------------|------|
| C / C++ | GitHub（源码 + 预编译 `.so`/`.a`） | CMake 编译 / 直接下载产物 | 0.1.34 | `cmake --build` 产物 `libjinwo` | ✅ 可用（非包管理） |
| Python | PyPI | `twine upload` 上传预编译 wheel | 0.1.33 | `pip install jinwo-vecdb` | ✅ 已发布 |
| Node.js | npm（WASM 预编译） | `npm publish` | 0.1.36 | `npm install jinwo-vecdb` | ✅ 已发布 |
| Go | pkg.go.dev（Go module） | GitHub tag + `go get` | 0.1.34 | `go get github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb@v0.1.34` | ✅ 已发布（默认 wasm 绑定，cgo 可选） |
| iOS | CocoaPods trunk | `pod trunk push` | 0.1.34 | `pod 'JinWoVecDB'` | ✅ 已发布 |
| Android | JitPack（AAR） | `jitpack.yml` 云端 NDK 编译 + 发布 | 0.1.34.spring | `implementation 'com.github.pkjinwo:jinwo_vector_db:0.1.34.spring'` | ✅ 已发布 |
| PHP | Composer（FFI + 预编译底座） | `composer require pkjinwo/vecdb`，安装时自动按平台下载 `libjinwo` | 0.1.34 | `composer require pkjinwo/vecdb` | ✅ 已发布 |
| C# | 未发布（仅 csproj demo） | 需 NuGet + RID | — | — | 🔶 仅示例 |
| Rust / Dart / Java / Kotlin / Swift / R / TypeScript / Julia | 未发布（仅 README/示例） | — | — | — | 🔶 仅文档/示例 |

> 说明：
> - **核心版本**（C 头文件 `JW_VERSION_*`）当前为 **0.1.34**。
> - Android 统一以全新 tag **`0.1.34.spring`** 发布（JitPack 按 tag 缓存构建，`0.1.34` 与 `0.1.34-beta` 已发布或缓存态不可复用，故改用 `0.1.34.spring`）。
> - Python 当前已发布最新为 0.1.33，待随核心版本升级到 0.1.34 时再发版。
> - **PHP 已通过 Composer 包 `pkjinwo/vecdb` 发布**（仓库 `pkjinwo/jinwo-vecdb-php`）：其底层用 PHP-FFI 调用与 C/C++ **完全相同**的 `libjinwo` C 接口；预编译底座（`libjinwo-<平台>.tar.gz`）由 GitHub Release `v0.1.34` 按平台提供，安装时自动下载。若你已在系统用 C 源码编译并安装 `libjinwo`，PHP 包也可直接复用（无需下载），两种来源完全兼容——因为无论底座来自 Composer 还是 C 源码构建，都是同一个 `libjinwo` C 库。
> - 详细安装步骤见 [`docs/zh_cn/usage_examples.zh_cn.md`](docs/zh_cn/usage_examples.zh_cn.md) 与各语言 `*_integration` 文档。

## 贡献指南

欢迎贡献代码、报告问题或提出建议！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 开源协议

本项目采用 [Apache 2.0](LICENSE) 协议开源。

## 关于

**JinWo VecDB** 由 [北京金幄科技有限公司](https://jinwo.site) 开发维护。

- 网站: https://jinwo.site
- 文档: https://docs.jinwo.site/jinwo_vector_db
- 问题反馈: https://github.com/pkjinwo/jinwo_vector_db/issues  #国外
- 问题反馈: https://gitee.com/pkjinwo/jinwo_vector_db/issues  #国内
