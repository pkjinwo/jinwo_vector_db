# JinWo VecDB Go 绑定（bindings/jinwo_vecdb_wasm）

通过 **纯 Go 的 wazero 运行时**加载并运行 Emscripten 构建的 `jinwo.wasm`，
提供与 Python / Node 绑定一致的高层 API。**完全不依赖 C 编译器 / C 链接器 / 预编译静态库**。

> 这是 **Go 侧推荐的默认接入方式**。若需要极致原生性能，可改用同级目录
> `bindings/jinwo_vecdb`（cgo 静态库方案，需自备各平台 `libjinwo.a`）。

## 特点

- **零 C 工具链**：`go build` 一键跨平台，无需 clang/gcc、无需交叉编译静态库。
- **单产物复用**：直接复用 Node 绑定同款 `jinwo.wasm`（`wasm/jinwo.wasm`，已 `go:embed` 进包），
  Python / Node / Go 三端共享同一份 WASM 产物。
- **真·跨平台**：wazero 是纯 Go WASI 运行时，`GOOS/GOARCH` 任意组合均可直接编译运行。

## 目录结构

```
bindings/jinwo_vecdb_wasm/
├── go.mod
├── go.sum
├── runtime.go          # wazero 运行时：WASI + env(host 桩) + 内存读写工具
├── vecdb.go            # 高层 Go API（DB / Collection / Search 等）
├── wasm.go             # go:embed 嵌入 jinwo.wasm
└── wasm/
    └── jinwo.wasm      # Emscripten 构建产物（与 npm_package/wasm 同源）
```

## 使用

```bash
go get github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm
```

```go
import vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm"

// 打开内存数据库（无需任何文件 / 静态库）
db, _ := vecdb.OpenMemory()
defer db.Close()

// 创建集合（128 维）
coll, _ := db.CreateCollection("docs", 128)

// 插入向量
vid, _ := coll.Insert([]float32{0.1, 0.2, /* ... */})

// 构建索引加速检索
_ = coll.BuildIndex()

// 相似度搜索 Top-5
results, _ := db.Search("docs", query, 5)
for _, r := range results {
    fmt.Printf("id=%d score=%.4f\n", r.ID, r.Score)
}

// 删除
_ = coll.Delete(vid)
```

运行示例：

```bash
cd examples/go_wasm
go run .
```

## 工作原理

- `jinwo.wasm` 是 Emscripten standalone 构建，导入两类符号：
  - `wasi_snapshot_preview1.*`（fd_read/write/seek/close/sync）→ 由 wazero 内置 WASI 满足；
  - `env.*`（Emscripten 专用：`emscripten_resize_heap`、`__syscall_*`、`_mmap_js` 等）→
    在 `runtime.go` 中以 Go host function 提供桩实现（内存模式下基本不会被调用）。
- wasm 带 `start` 段，wazero 实例化时自动执行 `__wasm_call_ctors` 完成 C 运行时初始化。
- 通过 wasm 线性内存按 wasm32 ABI 读写 `jw_str_t` / `jw_vid_t` / `jw_search_result`，
  与 `npm_package/dist/index.js` 的 JS 胶水层保持一致。

## 限制

- **仅支持内存模式**（`OpenMemory`）。文件持久化（`Open(path)` 非内存模式）在 wazero 下暂不支持——
  Emscripten 以 `__syscall_*` / `_mmap_js` 实现文件系统，wazero 未实现该层。
  需要落盘请使用 `OpenMemory()` 配合应用层持久化，或改用 cgo 绑定。
- wasm 导出的 API 为精简子集（与 Node 绑定一致），不支持 `DropCollection` / `ListCollections` /
  集合级 `Search` 等未导出函数；如需请走 cgo 绑定。
- wazero 默认使用解释器模式（纯 Go、无 CGO）；如需更高吞吐可启用 wazero 编译器
  （`wazero.NewRuntimeWithConfig` + `compiler`）——但编译器会引入 CGO，违背"零 C 工具链"初衷。
