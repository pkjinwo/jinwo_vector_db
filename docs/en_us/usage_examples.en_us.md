# JinWo VecDB Usage Examples (Multi-language)

This document provides **minimal runnable examples** for JinWo VecDB in major
languages, covering open database, create collection, insert vectors and
similarity search.

> Chinese version: `docs/zh_cn/usage_examples.zh_cn.md`
> Local source build (produces `libjinwo` library and `include/` headers):
> see `docs/en_us/build_guide.en_us.md`.

All examples use dimension `dim = 4` for brevity (production may use 128/768/1536).

---

## 1. Python

Install:

```bash
pip install jinwo_vecdb
```

Minimal example (API matches `build_python_pypi/jinwo_vecdb/__init__.py`):

```python
import jinwo_vecdb

# open/create database (empty path = in-memory)
db = jinwo_vecdb.open("my_vecs.jwv")

# create collection
db.create_collection("docs", dim=4)

# single insert
vid = db.insert("docs", [0.1, 0.2, 0.3, 0.4])

# batch insert
vids = db.insert_batch("docs", [
    [0.1, 0.2, 0.3, 0.4],
    [0.5, 0.6, 0.7, 0.8],
])

# similarity search, returns [(vid, distance), ...]
results = db.search("docs", [0.1, 0.2, 0.3, 0.4], k=3)
for vid, distance in results:
    print(f"vid={vid}, distance={distance}")

db.close()
```

> Note: the Python binding is cross-compiled via CMake in `build_python_pypi/`,
> verified on macOS and Linux (ubuntu:22.04), including close/reopen persistence.

---

## 2. Go

The Go binding links the C library through cgo. Use `examples/go/go_demo.go`
or the higher-level `bindings/jinwo_vecdb/vecdb.go`.

Minimal example (cgo direct calls, from `examples/go/go_demo.go`):

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
    if C.jw_vecdb_open(&pathStr, C.uint32_t(C.JW_VECDB_CREATE), &db) != 0 { // create/open
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

Build the `libjinwo` library first per `build_guide` (ensure `-L../../build`
points to the library directory).

---

## 3. C / C++

Link the locally built `libjinwo` and `include/` headers (see `build_guide`).

```c
#include <jw_vecdb.h>
#include <stdio.h>

int main(void) {
    jw_vecdb_t *db = NULL;
    if (jw_vecdb_open(jw_str("my_vecs.jwv"), JW_VECDB_CREATE, &db) != 0) {  // create/open
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

Compile (library + headers installed to `/usr/local` via `cmake --install`):

```bash
gcc demo.c -o demo -ljinwo -lm -I/usr/local/include -L/usr/local/lib
```

---

## 4. Android (Java / Kotlin)

Load `libjinwo` via JNI (`System.loadLibrary("jinwo")`). Kotlin wrapper
fragment (from `android_integration.en_us.md`):

```kotlin
// JinWoVecDBWrapper.kt
class JinWoVecDBWrapper {
    private val vecdb = JinWoVecDB()   // native class, System.loadLibrary("jinwo")
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

Integration (AAR / JitPack / source) — see `docs/en_us/android_integration.en_us.md`.
JitPack: `implementation 'com.github.pkjinwo:jinwo_vector_db:0.1.34.spring'` (bundles the `.so` for four ABIs).

---

## 5. iOS (Swift)

Via CocoaPods (`pod 'JinWoVecDB'` — published to the public CocoaPods trunk, installable with a single line) or `JinWoVecDB.framework`. Swift wrapper
fragment (from `ios_integration.en_us.md`):

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

Integration (CocoaPods / Framework / source) — see `docs/en_us/ios_integration.en_us.md`.

---

## 6. JavaScript / TypeScript (Node.js)

Via npm package `jinwo-vecdb` (repo `npm_package/`).

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

> Export names follow the actual `npm_package/` build; refer to that package's
> README if they differ from this example.

---

## 6.1 PHP (FFI)

Published as the Composer package `jinwo/vecdb`. Under the hood it uses PHP-FFI to
call the **exact same `libjinwo` C interface as C/C++**, so it is fully compatible
with building and installing `libjinwo` from C source — whether the backend comes
from the Composer auto-download or a system-installed `libjinwo`, the API behaves
identically.

Install:

```bash
composer require jinwo/vecdb
# the libjinwo base is auto-downloaded per platform after install; or run manually:
php vendor/jinwo/vecdb/bin/install.php
# if libjinwo is already built/installed from C source on the system, skip the
# download and point to it instead:
# export JINWO_VECDB_LIB=/usr/local/lib/libjinwo.dylib   # macOS example
```

Minimal example:

```php
<?php
require 'vendor/autoload.php';

use JinWoVecDB\JinWoVecDB;

// open/create database (empty path = in-memory)
$db = new JinWoVecDB('', JinWoVecDB::CREATE | JinWoVecDB::READWRITE);

$db->createCollection('docs', 4);

$vid = $db->insert('docs', [0.1, 0.2, 0.3, 0.4]);

$results = $db->search('docs', [0.1, 0.2, 0.3, 0.4], 3);
foreach ($results as $r) {
    echo "id={$r['id']} score={$r['score']}\n";
}

$db->close();
```

Troubleshooting: `JinWoVecDB::loadedLibPath()` prints the actual `libjinwo` path
loaded by FFI, so you can confirm whether the Composer-downloaded prebuilt or a
system C-built `libjinwo` (reported as `libjinwo`, resolved by the OS loader) is
in use.

> The prebuilt base (`libjinwo-<platform>.tar.gz`, with `lib/` + `include/*.h`) is
> published on GitHub Release `v0.1.34`; the FFI type definitions (`jw_str_t` /
> `jw_search_result`, etc.) strictly match the ABI of `include/jw_types.h` and
> `include/jw_vecdb.h`.

---

## 7. FAQ

- **Library file name (single name `libjinwo`)**: all languages share the same native
  library name `libjinwo` (Linux `libjinwo.so` / macOS `libjinwo.dylib` / Windows `libjinwo.dll`).
  The cross-platform prebuilt base is produced by `.github/workflows/build_native_lib.yml`
  as `libjinwo-<platform>.tar.gz` (contains `libjinwo-<platform>.*` + `include/*.h`) for PHP(FFI) / C#(DllImport) reuse.
- **Build command**: use `cmake -S . -B build` (`-S`/`-B` explicitly set source
  and build dirs). Do **not** run `mkdir build && cd build && cmake ..` inside
  the repo, or you'll get "CMakeLists.txt not found". See `build_guide.en_us.md`.
- **C# / .NET**: the Windows integration sample's `DllName` is `libjinwo`. See `windows_integration.en_us.md` notes.
