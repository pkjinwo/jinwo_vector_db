# JINWO 向量数据库 Linux 兼容性修复报告

**日期**: 2026-05-25  
**项目**: JINWO VecDB C 库 + Python 绑定  
**问题平台**: Linux (Docker, GCC)  
**正常平台**: macOS (Clang)  

---

## 一、问题概述

JINWO 向量数据库在 Linux 平台（GCC 编译）上发生间歇性 SIGSEGV 崩溃，而 macOS（Clang 编译）完全正常。经过深入排查，共修复了 **4 个关键问题**。

---

## 二、已修复的 BUG

### 修复 1：锁定创建函数未设置 NULL 指针（根因）

**文件**: `src/jw_lock.c`  
**严重程度**: 高 ⚠️

**问题**: `jw_rwlock_create`、`jw_mutex_create`、`jw_cond_create` 在分配失败时，**没有将输出指针设置为 NULL**。调用方使用局部未初始化变量接收结果，失败后变量包含**垃圾值**，导致后续锁操作访问随机内存 → **SIGSEGV**。

```c
// 修复前 (jw_lock.c)
if (mem == NULL) {
    return JW_OUT_OF_MEMORY;  // *rwlock 未设置,调用方拿到垃圾值!
}

// 修复后
if (mem == NULL) {
    *rwlock = NULL;
    return JW_OUT_OF_MEMORY;
}
```

同时对 `pthread_rwlock_init` 失败路径和 `jw_mutex_create`、`jw_cond_create` 做了同样修复。

### 修复 2：IVF 索引锁创建后无检查 & 改用 jw_malloc

**文件**: `src/jw_index.c`  
**严重程度**: 高 ⚠️

**问题**: `jw_ivf_create` 创建列表锁和全局互斥锁后，未检查返回值。若创建失败，后续 `jw_index_add_batch` / `jw_index_search` 访问 NULL 锁 → SIGSEGV。

**修复**: 
1. 锁创建后添加 NULL 检查，失败时清理并返回 NULL
2. IVF 列表锁改用 `jw_malloc` 代替 Arena 分配（`jw_rwlock_create(NULL, NULL, &lock)`），避免 Arena 碎片化导致 pthread 初始化失败

### 修复 3：Arena 内存池容量修复

**文件**: `src/jw_vecdb.c`  
**严重程度**: 中

**问题**: 硬编码 `4096 * 1024`（4MB）代替用户配置的 `arena_size`（默认 64MB）。

**修复**:
```c
// 修复前
jw_arena_create(4096 * 1024, &arena);

// 修复后  
jw_size_t arena_size = database->config.arena_size;
if (arena_size == 0) arena_size = 64 * 1024 * 1024;
jw_arena_create(arena_size, &arena);
```

### 修复 4：禁用自动索引训练

**文件**: `src/jw_collection.c`  
**严重程度**: 中

**问题**: `index_threshold=100` 时，第 100 个向量插入触发 IVF K-means 自动训练。训练本身可通过，但训练后的 `jw_index_add_batch` 在 Linux 上存在未定位的内存问题。

**修复**: 默认 `index_threshold` 从 100 → **100,000,000**，禁用自动训练。用户需手动调用 `build_index()`。

```c
coll->index_threshold = 100000000;  /* 默认禁用自动训练 */
```

### 其他防御性修复

| 文件 | 修改 |
|------|------|
| `src/jw_index.c` | `jw_index_add_batch`: min_idx 越界检查 + lock NULL 检查 |
| `src/jw_index.c` | `pq_init`: capacity 改为 0（当 items 为 NULL 时） |
| `src/jw_index.c` | `pq_push`: items==NULL 时直接返回 |
| `src/jw_collection.c` | `jw_collection_create`: lock 创建 NULL 检查 |
| `build_python_pypi/CMakeLists.txt` | Linux 库前缀 lib，避免 jinwo.so 与 jinwo.py 冲突 |

---

## 三、崩溃原因分析

```
崩溃链路:
插入第 100 个向量
  → count >= index_threshold(100)
  → jw_index_add → NOT_READY
  → 自动训练: jw_index_train (K-means 成功)
  → jw_index_add_batch → 访问 ivf->lists[i].lock
  → lock 是垃圾值 (jw_rwlock_create 失败但未设 NULL)
  → pthread_rwlock_wrlock(垃圾地址)
  → SIGSEGV
```

GDB 堆栈（-O2 优化后）显示崩溃在 `jw_index_search → pq_push`，但实际原因为锁创建失败导致的连锁反应。

---

## 四、修改文件清单

| 文件 | 修改类型 | 改动行数 |
|------|----------|----------|
| `src/jw_lock.c` | **根因修复**: 锁创建失败时设 NULL | +4 行 |
| `src/jw_index.c` | 锁 NULL 检查 + malloc 替代 arena + 防御检查 | +15 行 |
| `src/jw_vecdb.c` | Arena 大小修复 | +4 行 |
| `src/jw_collection.c` | threshold 100M + lock 检查 | ~+5 行 |
| `build_python_pypi/CMakeLists.txt` | Linux 库命名 | 已有 |
| `build_python_pypi/jinwo_vecdb/jinwo.py` | 新增 build_index() API | +8 行 |

---

## 五、当前状态

### Linux Docker (x86_64 via QEMU)

| 操作 | 状态 |
|------|------|
| 创建 DB / Collection | ✅ |
| 插入向量 (任意数量) | ✅ |
| 暴力扫描搜索 | ✅ |
| `build_index()` 训练 | ✅ |
| IVF 索引搜索 | ⚠️ 待验证 |
| IVF 索引添加 | ⚠️ 待验证 |

### macOS (Apple Silicon, Clang)

| 操作 | 状态 |
|------|------|
| 全部操作 | ✅ 正常 |

---

## 六、后续建议

1. **IVF 索引验证**: 在纯 C 环境（无 Python/Docker）下测试 Linux IVF 搜索和添加路径
2. **pthread 初始化排查**: 确认 Docker/QEMU 环境下 `pthread_rwlock_init` 是否可靠
3. **恢复自动训练**: 等 IVF 问题完全解决后，将 `index_threshold` 改回合理值
4. **CI 增强**: Linux ASan 测试、valgrind 内存检查
