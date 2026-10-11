// Package jinwo_vecdb_wasm - JinWo VecDB 的 Go 绑定（wazero + WASM）
//
// 通过纯 Go 的 wazero 运行时加载 Emscripten 构建的 jinwo.wasm，
// 提供与 Python / Node 绑定一致的高层 API，且完全不依赖 C 工具链 / C 链接器。
//
// 这是 Go 侧推荐的默认接入方式；若需要极致原生性能，可改用 bindings/jinwo_vecdb
// 的 cgo 静态库方案（需自备各平台 libjinwo.a）。
//
// 当前 wasm 构建仅导出精简 API（与 Node 绑定一致），因此：
//   - 支持：内存模式 Open、CreateCollection、Insert/Search、Collection 操作、BuildIndex
//   - 文件持久化（Open(path) 非内存模式）在 wazero 下暂不支持
//     （Emscripten 以 __syscall_*/_mmap_js 实现 FS，wazero 未实现该层）
//
// 使用示例：
//
//	import vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm"
//
//	db, err := vecdb.OpenMemory()
//	defer db.Close()
//
//	coll, err := db.CreateCollection("docs", 128)
//	vid, err := coll.Insert(make([]float32, 128))
//	res, err := db.Search("docs", query, 5)
package jinwo_vecdb_wasm

import (
	"context"
	"fmt"
)

// Error JinWo VecDB 错误（wasm 未导出 strerror，故仅含状态码）。
type Error struct {
	Code    int
	Message string
}

func (e *Error) Error() string {
	return fmt.Sprintf("jinwo_vecdb_wasm: %s (code=%d)", e.Message, e.Code)
}

func errorFromStatus(code int32) error {
	if code == 0 {
		return nil
	}
	return &Error{Code: int(code), Message: fmt.Sprintf("status=%d", code)}
}

// SearchResult 搜索结果。
type SearchResult struct {
	ID    uint64
	Score float32
}

// DB 数据库句柄。
type DB struct {
	r      *runtime
	handle uint32
}

// OpenMemory 打开/创建内存数据库。
func OpenMemory() (*DB, error) {
	return open("", flagReadWrite|flagMemory)
}

// Open 打开/创建数据库。
//
// 内存模式：path 传空字符串（等同于 OpenMemory）。
// 文件持久化模式：当前 wazero 绑定暂不支持，请使用 OpenMemory 或 cgo 绑定。
func Open(path string, _ ...uint32) (*DB, error) {
	if path == "" {
		return OpenMemory()
	}
	return nil, fmt.Errorf("jinwo_vecdb_wasm: 文件持久化模式在 wazero 下暂不支持，请使用 OpenMemory()（或改用 cgo 绑定 bindings/jinwo_vecdb）")
}

func open(path string, flags uint32) (*DB, error) {
	r, err := instance()
	if err != nil {
		return nil, err
	}
	pStr := r.writeJwStr(path)
	defer r.freeJwStr(pStr)
	dbOut := r.malloc(4)
	defer r.free(dbOut)

	res, err := r.fnOpen.Call(context.Background(), uint64(pStr), uint64(flags), uint64(dbOut))
	if err != nil {
		return nil, fmt.Errorf("jw_vecdb_open: %w", err)
	}
	if code := int32(res[0]); code != 0 {
		return nil, errorFromStatus(code)
	}
	dbPtr, _ := r.mod.Memory().ReadUint32Le(dbOut)
	if dbPtr == 0 {
		return nil, fmt.Errorf("jinwo_vecdb_wasm: jw_vecdb_open 返回空句柄")
	}
	return &DB{r: r, handle: dbPtr}, nil
}

// Close 关闭数据库（关闭前自动同步数据）。
func (db *DB) Close() error {
	if db.handle == 0 {
		return nil
	}
	res, err := db.r.fnClose.Call(context.Background(), uint64(db.handle))
	if err != nil {
		return fmt.Errorf("jw_vecdb_close: %w", err)
	}
	db.handle = 0
	return errorFromStatus(int32(res[0]))
}

// Version 返回版本字符串。
func Version() (string, error) {
	r, err := instance()
	if err != nil {
		return "", err
	}
	buf := r.malloc(szJwStrT)
	defer r.free(buf)
	if _, err := r.fnVersion.Call(context.Background(), uint64(buf)); err != nil {
		return "", fmt.Errorf("jw_vecdb_version: %w", err)
	}
	mem := r.mod.Memory()
	strPtr, _ := mem.ReadUint32Le(buf)
	strLen, _ := mem.ReadUint32Le(buf + 4)
	b, _ := mem.Read(strPtr, strLen)
	return string(b), nil
}

// CreateCollection 创建集合。
func (db *DB) CreateCollection(name string, dim int) (*Collection, error) {
	nStr := db.r.writeJwStr(name)
	defer db.r.freeJwStr(nStr)
	collOut := db.r.malloc(4)
	defer db.r.free(collOut)

	res, err := db.r.fnCreateColl.Call(context.Background(), uint64(db.handle), uint64(nStr), uint64(dim), uint64(collOut))
	if err != nil {
		return nil, fmt.Errorf("jw_vecdb_create_collection: %w", err)
	}
	if code := int32(res[0]); code != 0 {
		return nil, errorFromStatus(code)
	}
	collPtr, _ := db.r.mod.Memory().ReadUint32Le(collOut)
	return &Collection{r: db.r, handle: collPtr, dim: dim}, nil
}

// GetCollection 获取已存在的集合；不存在返回 nil（不报错）。
func (db *DB) GetCollection(name string) *Collection {
	nStr := db.r.writeJwStr(name)
	defer db.r.freeJwStr(nStr)
	res, err := db.r.fnGetColl.Call(context.Background(), uint64(db.handle), uint64(nStr))
	if err != nil {
		return nil
	}
	collPtr := uint32(res[0])
	if collPtr == 0 {
		return nil
	}
	return &Collection{r: db.r, handle: collPtr, dim: 0}
}

// Insert 快速插入向量（自动创建/获取集合），返回分配的向量 ID。
func (db *DB) Insert(collName string, vec []float32) (uint64, error) {
	nStr := db.r.writeJwStr(collName)
	defer db.r.freeJwStr(nStr)
	vPtr := db.r.writeFloats(vec)
	defer db.r.free(vPtr)
	vidOut := db.r.malloc(szJwVidT)
	defer db.r.free(vidOut)

	res, err := db.r.fnInsert.Call(context.Background(), uint64(db.handle), uint64(nStr), uint64(vPtr), uint64(len(vec)), uint64(vidOut))
	if err != nil {
		return 0, fmt.Errorf("jw_vecdb_insert: %w", err)
	}
	if code := int32(res[0]); code != 0 {
		return 0, errorFromStatus(code)
	}
	vid, _ := db.r.mod.Memory().ReadUint64Le(vidOut)
	return vid, nil
}

// Search 快速搜索，返回 Top-K 结果。
func (db *DB) Search(collName string, query []float32, k int) ([]SearchResult, error) {
	if k <= 0 {
		return nil, nil
	}
	nStr := db.r.writeJwStr(collName)
	defer db.r.freeJwStr(nStr)
	qPtr := db.r.writeFloats(query)
	defer db.r.free(qPtr)
	resBuf := db.r.malloc(uint32(k) * szJwSearchRes)
	defer db.r.free(resBuf)

	res, err := db.r.fnSearch.Call(context.Background(), uint64(db.handle), uint64(nStr), uint64(qPtr), uint64(len(query)), uint64(k), uint64(resBuf))
	if err != nil {
		return nil, fmt.Errorf("jw_vecdb_search: %w", err)
	}
	count := int32(res[0])
	mem := db.r.mod.Memory()
	out := make([]SearchResult, 0, count)
	for i := int32(0); i < count; i++ {
		base := resBuf + uint32(i)*szJwSearchRes
		id, _ := mem.ReadUint64Le(base)
		score, _ := mem.ReadFloat32Le(base + 8)
		out = append(out, SearchResult{
			ID:    id,
			Score: score,
		})
	}
	return out, nil
}

// Collection 向量集合句柄。
type Collection struct {
	r      *runtime
	handle uint32
	dim    int
}

// Dim 返回向量维度。
func (c *Collection) Dim() int { return c.dim }

// Insert 插入向量，返回分配的向量 ID。
func (c *Collection) Insert(vec []float32) (uint64, error) {
	if c.dim != 0 && len(vec) != c.dim {
		return 0, &Error{Code: -103, Message: fmt.Sprintf("维度不匹配: 期望 %d, 实际 %d", c.dim, len(vec))}
	}
	vPtr := c.r.writeFloats(vec)
	defer c.r.free(vPtr)
	vidOut := c.r.malloc(szJwVidT)
	defer c.r.free(vidOut)

	res, err := c.r.fnCollInsert.Call(context.Background(), uint64(c.handle), uint64(vPtr), uint64(vidOut))
	if err != nil {
		return 0, fmt.Errorf("jw_collection_insert: %w", err)
	}
	if code := int32(res[0]); code != 0 {
		return 0, errorFromStatus(code)
	}
	vid, _ := c.r.mod.Memory().ReadUint64Le(vidOut)
	return vid, nil
}

// Delete 删除向量。
func (c *Collection) Delete(vid uint64) error {
	res, err := c.r.fnCollDelete.Call(context.Background(), uint64(c.handle), uint64(vid))
	if err != nil {
		return fmt.Errorf("jw_collection_delete: %w", err)
	}
	return errorFromStatus(int32(res[0]))
}

// BuildIndex 构建索引以加速检索。
func (c *Collection) BuildIndex() error {
	res, err := c.r.fnCollBuildIndex.Call(context.Background(), uint64(c.handle))
	if err != nil {
		return fmt.Errorf("jw_collection_build_index: %w", err)
	}
	return errorFromStatus(int32(res[0]))
}
