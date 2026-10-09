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

Go binding 通过 cgo 直接链接 C 库。可使用仓库内 `examples/go/go_demo.go`，
或 `bindings/jinwo_vecdb/vecdb.go` 高层封装。

最小示例（cgo 直调，取自 `examples/go/go_demo.go`）：

```go
package main

/*
#cgo CFLAGS: -I../../include
#cgo linux LDFLAGS: -L../../build -ljinwo -lm -pthread
#cgo darwin LDFLAGS: -L../../build -ljinwo -lm
#cgo windows LDFLAGS: -L../../build -ljinwo -lm
#include <jw_vecdb.h>
#include <stdlib.h>
*/
import "C"
import (
    "fmt"
    "unsafe"
)

func main() {
    cPath := C.CString("./vecdb_go")
    defer C.free(unsafe.Pointer(cPath))
    pathStr := C.jw_str_t{ptr: cPath, slen: C.size_t(len("./vecdb_go"))}

    var db *C.jw_vecdb_t
    if C.jw_vecdb_open(&pathStr, C.uint32_t(C.JW_VECDB_CREATE), &db) != 0 { // 创建/打开
        panic("open failed")
    }
    defer C.jw_vecdb_close(db)

    cName := C.CString("test")
    defer C.free(unsafe.Pointer(cName))
    nameStr := C.jw_str_t{ptr: cName, slen: C.size_t(len("test"))}

    var coll *C.jw_collection_t
    if C.jw_vecdb_create_collection(db, &nameStr, C.uint32_t(4), &coll) != 0 {
        panic("create collection failed")
    }

    vec := []float32{0.1, 0.2, 0.3, 0.4}
    cVec := (*C.float)(unsafe.Pointer(&vec[0]))
    var id C.uint64_t
    if C.jw_collection_insert(coll, cVec, &id) != 0 {
        panic("insert failed")
    }

    var results [5]C.jw_search_result_t
    count := C.jw_vecdb_search(
        db, &nameStr, cVec,
        C.uint32_t(4), C.size_t(5),
        (*C.jw_search_result_t)(unsafe.Pointer(&results[0])),
    )
    fmt.Printf("got %d results, top id=%d score=%.4f\n",
        uint64(count), uint64(results[0].id), float32(results[0].score))
}
```

编译前请先按 `build_guide` 构建并安装 `libjinwo`（确保 `-L../../build` 指向库所在目录）。

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

---

## 5. iOS（Swift）

通过 CocoaPods（`pod 'JinWoVecDB'`）或引入 `JinWoVecDB.framework`。
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

通过 npm 包 `jinwo-vecdb`（仓库 `npm_package/`）。

```bash
npm install jinwo-vecdb
```

```ts
import { JinWoDB } from "jinwo-vecdb";

const db = new JinWoDB("my_vecs.jwv");
const col = db.createCollection("docs", 4);

const vid = col.insert([0.1, 0.2, 0.3, 0.4]);
const results = col.search([0.1, 0.2, 0.3, 0.4], 5);
for (const r of results) {
    console.log(`id=${r.id} distance=${r.distance}`);
}
db.close();
```

> 具体导出名以 `npm_package/` 实际构建产物为准；如与示例不符请参照该包 README。

---

## 7. 常见问题

- **库文件名**：本仓库统一产出 `libjinwo.*`（Windows 亦为 `libjinwo.dll` /
  `libjinwo.lib`）。旧文档出现的 `jinwo_vecdb.dll` / `jinwo_vecdb.lib`
  已废弃，请勿沿用。
- **构建命令**：请使用 `cmake -S . -B build`（`-S`/`-B` 显式指定源与构建
  目录），**不要**在仓库内 `mkdir build && cd build && cmake ..`，否则会
  提示找不到 `CMakeLists.txt`。详见 `build_guide.zh_cn.md`。
- **C# / .NET**：Windows 集成示例的 `DllName` 应改为 `libjinwo` 而非
  `jinwo_vecdb`，详见 `windows_integration.zh_cn.md` 修正说明。
