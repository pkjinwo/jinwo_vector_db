# JinWo VecDB Go 绑定（bindings/jinwo_vecdb）

> ⚠️ **可选 / 原生高性能方案**：默认推荐走 `bindings/jinwo_vecdb_wasm`（wazero + WASM，
> 零 C 工具链、真·跨平台 `go build`）。本目录是 cgo 方案，仅在你需要**极致原生性能**、
> 且本机具备 C 链接器与各平台 **`libjinwo.a`** 预编译静态库时使用。

官方 Go binding，通过 cgo 链接预编译的 C 静态库 **`libjinwo.a`**。

## 特点

- **无需从源码编译 C 库**：各平台静态库已随仓库发布在 `lib/<goos>_<goarch>/`，
  `go build` 时 cgo 按当前 `GOOS/GOARCH` 自动链接对应静态库（类似 Python 的多平台
  wheel，用户只需本机具备 C 链接器）。
- 覆盖 linux / darwin / windows × amd64 / arm64。

## 目录结构

```
bindings/jinwo_vecdb/
├── vecdb.go                      # cgo 封装 + 高层 Go API
├── c_bridge.c.legacy             # 旧版（cgo 内联全部 C 源）方案，已弃用，仅作参考
├── go.mod
├── lib/                          # 预编译静态库（随仓库提交）
│   ├── darwin_amd64/libjinwo.a
│   ├── darwin_arm64/libjinwo.a
│   ├── linux_amd64/libjinwo.a    # 需在 Linux 或 CI 中构建
│   ├── linux_arm64/libjinwo.a
│   ├── windows_amd64/libjinwo.a
│   └── windows_arm64/libjinwo.a
└── scripts/
    └── build_static_libs.sh      # 生成上述静态库
```

## 使用

```bash
go get github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb
```

```go
import vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb"

db, _ := vecdb.Open("./vecdb_go", vecdb.Create|vecdb.ReadWrite)
defer db.Close()
coll, _ := db.CreateCollection("test", 4)
vid, _ := coll.Insert([]float32{0.1, 0.2, 0.3, 0.4})
results, _ := coll.Search([]float32{0.1, 0.2, 0.3, 0.4}, 5)
```

## 重新生成静态库

```bash
bash scripts/build_static_libs.sh
```

- macOS 本地可同时产出 `darwin_amd64` 与 `darwin_arm64`（Apple clang `-arch`）。
- `linux` / `windows` 需要对应交叉工具链
  （`x86_64-linux-gnu-gcc`、`aarch64-linux-gnu-gcc`、`x86_64-w64-mingw32-gcc` 等）；
  缺失时脚本会自动跳过，建议在其对应平台或 CI 中构建后提交 `lib/` 产物。
- 源文件清单与根 `CMakeLists.txt` 的 `JINWO_SOURCES` 保持一致（不含 `jw_ios_bridge.c`）。
