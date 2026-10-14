# JinWo VecDB 使用示例（多语言）

本文给出 JinWo VecDB 在主流语言下的**最小可运行示例**，覆盖
打开数据库、创建集合、插入向量、相似度搜索等核心操作。

> 英文版：`docs/en_us/usage_examples.en_us.md`
> 本地源码编译（产出 `libjinwo` 库与 `include/` 头文件）见
> `docs/zh_cn/build_guide.zh_cn.md`。

所有示例默认维度 `dim = 4`（仅为演示；生产可用 128/768/1536 等）。

---

## 1. Python

安装：

```bash
pip install jinwo_vecdb
```

最小示例（API 以 `build_python_pypi/jinwo_vecdb/__init__.py` 为准）：

```python
import jinwo_vecdb

# 打开/创建数据库（path 为空字符串表示内存数据库）
db = jinwo_vecdb.open("my_vecs.jwv")

# 创建集合
db.create_collection("docs", dim=4)

# 单条插入
vid = db.insert("docs", [0.1, 0.2, 0.3, 0.4])

# 批量插入
vids = db.insert_batch("docs", [
    [0.1, 0.2, 0.3, 0.4],
    [0.5, 0.6, 0.7, 0.8],
])

# 相似度搜索，返回 [(vid, distance), ...]
results = db.search("docs", [0.1, 0.2, 0.3, 0.4], k=3)
for vid, distance in results:
    print(f"vid={vid}, distance={distance}")

db.close()
```

> 说明：Python 绑定底层由本仓库 `build_python_pypi/` 用 CMake 跨平台编译，
> 已在 macOS / Linux (ubuntu:22.04) 实测通过，含 close 后 reopen 持久化校验。

---

## 2. Go

JinWo VecDB 提供两套官方 Go binding：

- **`bindings/jinwo_vecdb_wasm`（推荐，零 C 工具链）**：基于纯 Go 的 wazero 运行时加载
  `jinwo.wasm`，`go build` 一键跨平台，无需 C 编译器 / 预编译静态库。
- **`bindings/jinwo_vecdb`（可选，原生高性能）**：cgo 链接预编译静态库 `libjinwo.a`，
  仅在需要极致原生吞吐时使用（需各平台 `lib/<goos>_<goarch>/libjinwo.a` 与 C 链接器）。

### 方式一：wazero + WASM（推荐，零 C 工具链）

```bash
go get github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm
```

```go
package main

import (
    "fmt"

    vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm"
)

func main() {
    db, err := vecdb.OpenMemory() // 内存数据库，无需任何静态库 / C 工具链
    if err != nil {
        panic(err)
    }
    defer db.Close()

    coll, err := db.CreateCollection("test", 4)
    if err != nil {
        panic(err)
    }

    vid, err := coll.Insert([]float32{0.1, 0.2, 0.3, 0.4})
    if err != nil {
        panic(err)
    }

    results, err := db.Search("test", []float32{0.1, 0.2, 0.3, 0.4}, 5)
    if err != nil {
        panic(err)
    }
    fmt.Printf("inserted vid=%d, got %d results (top id=%d score=%.4f)\n",
        vid, len(results), results[0].ID, results[0].Score)
}
```

### 方式二：cgo 静态库（可选，原生高性能）

各平台（linux/darwin/windows × amd64/arm64）的预编译静态库 `libjinwo.a` 发布在
`bindings/jinwo_vecdb/lib/<goos>_<goarch>/`，cgo 会在 `go build` 时按当前 `GOOS/GOARCH`
自动链接（类似 Python 的多平台 wheel，用户需本机具备 C 链接器）。

```bash
go get github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb
```

```go
package main

import (
    "fmt"

    vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb"
)

func main() {
    db, err := vecdb.Open("./vecdb_go", vecdb.Create|vecdb.ReadWrite)
    if err != nil {
        panic(err)
    }
    defer db.Close()

    coll, err := db.CreateCollection("test", 4)
    if err != nil {
        panic(err)
    }

    vid, err := coll.Insert([]float32{0.1, 0.2, 0.3, 0.4})
    if err != nil {
        panic(err)
    }

    results, err := coll.Search([]float32{0.1, 0.2, 0.3, 0.4}, 5)
    if err != nil {
        panic(err)
    }
    fmt.Printf("inserted vid=%d, got %d results (top id=%d score=%.4f)\n",
        vid, len(results), results[0].ID, results[0].Score)
}
```

若想从源码构建 `libjinwo` 后由 cgo 直调，可参考 `examples/go/go_demo.go`（需先按 `build_guide`
构建，使 `-L../../build` 指向库所在目录）。

> 平台静态库由 `bindings/jinwo_vecdb/scripts/build_static_libs.sh` 生成；
> macOS 可本地构建 darwin/amd64 与 darwin/arm64，linux/windows 需对应交叉工具链
> （建议在 CI / 对应平台上构建后提交产物）。

---

## 3. C / C++

直接链接本地编译出的 `libjinwo` 与 `include/` 头文件（参见 `build_guide`）。

```c
#include <jw_vecdb.h>
#include <stdio.h>

int main(void) {
    jw_vecdb_t *db = NULL;
    if (jw_vecdb_open(jw_str("my_vecs.jwv"), JW_VECDB_CREATE, &db) != 0) {  // 创建/打开
        return 1;
    }

    jw_collection_t *coll = NULL;
    if (jw_vecdb_create_collection(db, jw_str("docs"), 4, &coll) != 0) {
        jw_vecdb_close(db);
        return 1;
    }

    float vec[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    jw_vec_id_t id = 0;
    jw_collection_insert(coll, vec, &id);

    jw_search_result_t results[5];
    jw_size_t count = jw_vecdb_search(db, jw_str("docs"), vec, 4, 5, results);
    for (jw_size_t i = 0; i < count; i++) {
        printf("id=%llu score=%.4f\n",
               (unsigned long long)results[i].id, results[i].score);
    }

    jw_vecdb_close(db);
    return 0;
}
```

编译（库与头文件已 `cmake --install` 到 `/usr/local`）：

```bash
gcc demo.c -o demo -ljinwo -lm -I/usr/local/include -L/usr/local/lib
```

---

## 4. Android（Java / Kotlin）

通过 JNI 加载 `libjinwo`（so 文件名 `libjw_vecdb.so` 或 `libjinwo.so`，
以实际产物为准）。下面为 Kotlin 封装片段（取自 `android_integration.zh_cn.md`）：

```kotlin
// JinWoVecDBWrapper.kt
class JinWoVecDBWrapper {
    private val vecdb = JinWoVecDB()   // native 类，System.loadLibrary("jw_vecdb")
    private var dbPtr: Long = 0
    private var colPtr: Long = 0

    fun open(path: String, create: Boolean = true): Boolean {
        dbPtr = vecdb.open(path, create)
        return dbPtr != 0L
    }
    fun createCollection(name: String, dimension: Int): Boolean {
        colPtr = vecdb.createCollection(dbPtr, name, dimension)
        return colPtr != 0L
    }
    fun insertVector(vector: FloatArray): Long =
        vecdb.insertVector(colPtr, vector)
    fun searchVectors(query: FloatArray, k: Int = 10): Pair<Array<FloatArray>, LongArray> {
        val vectors = vecdb.searchVectors(colPtr, query, k)
        val ids = vecdb.searchVectorIds(colPtr, query, k)
        return Pair(vectors, ids)
    }
    fun close() {
        if (colPtr != 0L) { vecdb.closeCollection(colPtr); colPtr = 0 }
        if (dbPtr != 0L)  { vecdb.close(dbPtr);          dbPtr = 0 }
    }
}
```

集成方式（AAR / JitPack / 源码）详见 `docs/zh_cn/android_integration.zh_cn.md`。
JitPack 在线安装：`implementation 'com.github.pkjinwo:jinwo_vector_db:0.1.34-beta'`（已内含四架构 `.so`）。

---

## 5. iOS（Swift）

通过 CocoaPods（`pod 'JinWoVecDB'`，已发布到公共 trunk 仓库，一行即可安装）或引入 `JinWoVecDB.framework`。
Swift 封装片段（取自 `ios_integration.zh_cn.md`）：

```swift
import Foundation

let db: JinWoVecDB
do {
    db = try JinWoVecDB(path: "my_vecs.jwv", create: true)
    let col = try db.createCollection(name: "docs", dimension: 4)

    let vec: [Float] = [0.1, 0.2, 0.3, 0.4]
    _ = try col.insert(vector: vec)

    let results = try col.search(query: vec, k: 5)
    for r in results {
        print("id=\(r.id) distance=\(r.distance)")
    }
} catch {
    print("VecDB error: \(error)")
}
```

集成方式（CocoaPods / Framework / 源码）详见 `docs/zh_cn/ios_integration.zh_cn.md`。

---

## 6. JavaScript / TypeScript（Node.js）

通过 npm 包 `jinwo-vecdb`（仓库 `npm_package/`，WASM 实现，高层 API）。

```bash
npm install jinwo-vecdb
```

```ts
import { open } from "jinwo-vecdb";

const db = await open("my_vecs.jwv");            // 打开/创建数据库
console.log("version:", db.version());

const coll = db.createCollection("docs", 4);     // 创建 Collection
coll.insert([0.1, 0.2, 0.3, 0.4]);               // 插入向量
coll.buildIndex();                               // 构建索引，加速检索

const results = db.search("docs", [0.1, 0.2, 0.3, 0.4], 5); // Top-5 相似向量
for (const r of results) {
    console.log(`id=${r.id} score=${r.score}`);
}
db.close();
```

> 包为 ESM（`"type": "module"`），Node.js 示例需用 `import`；
> 具体导出名以 `npm_package/` 实际构建产物为准。

---

## 7. 常见问题

- **库文件名（跨平台底座）**：面向**尚未发布**语言（C#/PHP 等）的跨平台 FFI 底座
  统一命名为 `libjinwo_vecdb.*`（Linux `libjinwo_vecdb.so` / macOS `libjinwo_vecdb.dylib` /
  Windows `libjinwo_vecdb.dll`），由 `.github/workflows/build_native_lib.yml` 跨平台编译并
  发布，各平台预编译包见 CI 产物 `libjinwo_vecdb-<平台>.tar.gz`。
  已发布语言（C/C++/Go/Python/Android）当前沿用 `libjinwo`，计划在 0.1.35 统一切到 `libjinwo_vecdb`。
- **构建命令**：请使用 `cmake -S . -B build`（`-S`/`-B` 显式指定源与构建
  目录），**不要**在仓库内 `mkdir build && cd build && cmake ..`，否则会
  提示找不到 `CMakeLists.txt`。详见 `build_guide.zh_cn.md`。
- **C# / .NET**：Windows 集成示例的 `DllName` 应为 `libjinwo_vecdb`（跨平台 FFI 底座），
  详见 `windows_integration.zh_cn.md`。
