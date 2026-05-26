/*
Package jinwo_vecdb - JinWo VecDB Go binding

嵌入式向量数据库的 Go 封装，通过 cgo 直接编译 C 源码。

使用示例：

	import vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/go"

	db, err := vecdb.Open("my_vecs.jwv", vecdb.Create|vecdb.ReadWrite)
	defer db.Close()

	coll, err := db.CreateCollection("docs", 1536)
	vec := make([]float32, 1536)
	vid, err := coll.Insert(vec)
	results, err := coll.Search(vec, 10)
*/
package jinwo_vecdb

/*
#cgo CFLAGS: -I${SRCDIR}/../../include
#cgo LDFLAGS: -lm -lpthread

#include <stdlib.h>
#include <string.h>
#include <jw_vecdb.h>

// 辅助: 创建 jw_str_t
static jw_str_t make_jw_str(const char *s) {
	jw_str_t str;
	str.ptr = s;
	str.slen = s ? strlen(s) : 0;
	return str;
}
*/
import "C"
import (
	"fmt"
	"math"
	"unsafe"
)

// ============================================================
// 标志常量
// ============================================================

// Flag 数据库打开标志
type Flag uint32

const (
	ReadOnly  Flag = C.JW_VECDB_READONLY
	ReadWrite Flag = C.JW_VECDB_READWRITE
	Create    Flag = C.JW_VECDB_CREATE
	Truncate  Flag = C.JW_VECDB_TRUNCATE
	Memory    Flag = C.JW_VECDB_MEMORY
	NoMmap    Flag = C.JW_VECDB_NOMMAP
	NoCache   Flag = C.JW_VECDB_NOCACHE
	Sync      Flag = C.JW_VECDB_SYNC
)

// ============================================================
// 错误码
// ============================================================

const (
	Success              = C.JW_SUCCESS              // 0
	ErrUnknown           = C.JW_UNKNOWN_ERROR        // -1
	ErrInvalidParam      = C.JW_INVALID_PARAM        // -2
	ErrOutOfMemory       = C.JW_OUT_OF_MEMORY        // -3
	ErrNotFound          = C.JW_NOT_FOUND            // -4
	ErrAlreadyExists     = C.JW_ALREADY_EXISTS       // -5
	ErrBufferTooSmall    = C.JW_BUFFER_TOO_SMALL     // -6
	ErrNotSupported      = C.JW_NOT_SUPPORTED        // -7
	ErrPermissionDenied  = C.JW_PERMISSION_DENIED    // -8
	ErrInvalidVector     = C.JW_INVALID_VECTOR       // -100
	ErrInvalidDimension  = C.JW_INVALID_DIMENSION    // -103
	ErrIndexNotReady     = C.JW_INDEX_NOT_READY      // -104
	ErrVectorExists      = C.JW_VECTOR_EXISTS        // -105
	ErrVectorNotFound    = C.JW_VECTOR_NOT_FOUND     // -106
	ErrCollectionExists  = C.JW_COLLECTION_EXISTS    // -107
	ErrCollectionNotFound = C.JW_COLLECTION_NOT_FOUND // -108
	ErrIOError           = C.JW_IO_ERROR             // -203
	ErrReadOnly          = C.JW_READ_ONLY            // -206
)

// Error 数据库错误
type Error struct {
	Code    int
	Message string
}

func (e *Error) Error() string {
	return fmt.Sprintf("jinwo_vecdb: %s (code=%d)", e.Message, e.Code)
}

func newError(code C.int) error {
	if code == Success {
		return nil
	}
	msg := C.GoString(C.jw_vecdb_strerror(C.jw_status_t(code)))
	return &Error{Code: int(code), Message: msg}
}

// ============================================================
// 版本
// ============================================================

// Version 返回版本字符串 "JinWo VecDB x.y.z"
func Version() string {
	cStr := C.jw_vecdb_version()
	return C.GoStringN(cStr.ptr, C.int(cStr.slen))
}

// ============================================================
// DB - 数据库句柄
// ============================================================

// DB 数据库句柄 (线程安全)
type DB struct {
	handle *C.jw_vecdb_t
}

// Open 打开或创建数据库
//
//	path: 文件路径，空字符串表示内存数据库
//	flags: 打开标志组合，如 Create|ReadWrite
func Open(path string, flags ...Flag) (*DB, error) {
	var f Flag
	for _, v := range flags {
		f |= v
	}

	var db *C.jw_vecdb_t
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))

	cs := C.make_jw_str(cPath)
	status := C.jw_vecdb_open(&cs, C.uint32_t(f), &db)
	if status != Success {
		return nil, newError(status)
	}
	return &DB{handle: db}, nil
}

// OpenMemory 创建内存数据库
func OpenMemory() (*DB, error) {
	return Open("", Create|ReadWrite|Memory)
}

// Close 关闭数据库
func (db *DB) Close() error {
	status := C.jw_vecdb_close(db.handle)
	db.handle = nil
	if status != Success {
		return newError(status)
	}
	return nil
}

// Version 返回数据库版本字符串
func (db *DB) Version() string {
	return Version()
}

// Sync 同步到磁盘
func (db *DB) Sync() error {
	return newError(C.jw_vecdb_sync(db.handle))
}

// IsOpen 数据库是否已打开
func (db *DB) IsOpen() bool {
	return int(C.jw_vecdb_is_open(db.handle)) != 0
}

// ============================================================
// Collection 管理
// ============================================================

// CreateCollection 创建集合
func (db *DB) CreateCollection(name string, dim int) (*Collection, error) {
	var coll *C.jw_collection_t
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cs := C.make_jw_str(cName)
	status := C.jw_vecdb_create_collection(db.handle, &cs, C.uint32_t(dim), &coll)
	if status != Success {
		return nil, newError(status)
	}
	return &Collection{handle: coll, dim: dim}, nil
}

// GetCollection 获取现有集合
func (db *DB) GetCollection(name string) *Collection {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cs := C.make_jw_str(cName)
	coll := C.jw_vecdb_get_collection(db.handle, &cs)
	if coll == nil {
		return nil
	}
	return &Collection{handle: coll}
}

// DropCollection 删除集合
func (db *DB) DropCollection(name string) error {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cs := C.make_jw_str(cName)
	return newError(C.jw_vecdb_drop_collection(db.handle, &cs))
}

// HasCollection 检查集合是否存在
func (db *DB) HasCollection(name string) bool {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cs := C.make_jw_str(cName)
	return int(C.jw_vecdb_has_collection(db.handle, &cs)) != 0
}

// ListCollections 列出所有集合名称
func (db *DB) ListCollections() []string {
	// 先获取数量
	count := C.jw_vecdb_list_collections(db.handle, nil, 0)
	if count == 0 {
		return nil
	}

	names := make([]C.jw_str_t, count)
	actual := C.jw_vecdb_list_collections(db.handle, &names[0], count)

	result := make([]string, 0, actual)
	for i := C.size_t(0); i < actual; i++ {
		result = append(result, C.GoStringN(names[i].ptr, C.int(names[i].slen)))
	}
	return result
}

// ============================================================
// 便捷操作
// ============================================================

// Insert 快速插入向量 (自动使用/创建集合)
func (db *DB) Insert(collName string, vec []float32) (uint64, error) {
	var vid C.uint64_t
	cName := C.CString(collName)
	defer C.free(unsafe.Pointer(cName))

	cs := C.make_jw_str(cName)
	status := C.jw_vecdb_insert(
		db.handle, &cs,
		(*C.float)(unsafe.Pointer(&vec[0])),
		C.uint32_t(len(vec)),
		&vid,
	)
	if status != Success {
		return 0, newError(status)
	}
	return uint64(vid), nil
}

// Search 快速搜索
func (db *DB) Search(collName string, query []float32, k int) ([]SearchResult, error) {
	cName := C.CString(collName)
	defer C.free(unsafe.Pointer(cName))

	results := make([]C.jw_search_result_t, k)
	cs := C.make_jw_str(cName)
	actual := C.jw_vecdb_search(
		db.handle, &cs,
		(*C.float)(unsafe.Pointer(&query[0])),
		C.uint32_t(len(query)),
		C.size_t(k),
		&results[0],
	)

	out := make([]SearchResult, 0, actual)
	for i := C.size_t(0); i < actual; i++ {
		out = append(out, SearchResult{
			ID:    uint64(results[i].id),
			Score: float32(results[i].score),
		})
	}
	return out, nil
}

// ============================================================
// SearchResult
// ============================================================

// SearchResult 搜索结果
type SearchResult struct {
	ID    uint64
	Score float32
}

// ============================================================
// Collection - 集合句柄
// ============================================================

// Collection 向量集合
type Collection struct {
	handle *C.jw_collection_t
	dim    int
}

// Name 返回集合名称
func (c *Collection) Name() string {
	return C.GoString(C.jw_collection_get_name(c.handle))
}

// Dim 返回向量维度
func (c *Collection) Dim() int {
	return c.dim
}

// Insert 插入向量，返回分配的向量ID
func (c *Collection) Insert(vec []float32) (uint64, error) {
	if len(vec) != c.dim {
		return 0, &Error{
			Code:    ErrInvalidDimension,
			Message: fmt.Sprintf("dimension mismatch: expected %d, got %d", c.dim, len(vec)),
		}
	}
	var vid C.uint64_t
	status := C.jw_collection_insert(
		c.handle,
		(*C.float)(unsafe.Pointer(&vec[0])),
		&vid,
	)
	if status != Success {
		return 0, newError(status)
	}
	return uint64(vid), nil
}

// InsertBatch 批量插入向量
func (c *Collection) InsertBatch(vectors [][]float32) ([]uint64, error) {
	if len(vectors) == 0 {
		return nil, nil
	}
	dim := len(vectors[0])
	// 展平为连续内存
	flat := make([]float32, len(vectors)*dim)
	for i, v := range vectors {
		if len(v) != c.dim {
			return nil, &Error{
				Code:    ErrInvalidDimension,
				Message: fmt.Sprintf("dimension mismatch: expected %d, got %d at index %d", c.dim, len(v), i),
			}
		}
		copy(flat[i*dim:], v)
	}

	vids := make([]C.uint64_t, len(vectors))
	status := C.jw_collection_insert_batch(
		c.handle,
		(*C.float)(unsafe.Pointer(&flat[0])),
		C.size_t(len(vectors)),
		&vids[0],
	)
	if status != Success {
		return nil, newError(status)
	}

	out := make([]uint64, len(vids))
	for i, v := range vids {
		out[i] = uint64(v)
	}
	return out, nil
}

// Delete 删除向量
func (c *Collection) Delete(vid uint64) error {
	return newError(C.jw_collection_delete(c.handle, C.uint64_t(vid)))
}

// DeleteBatch 批量删除
func (c *Collection) DeleteBatch(vids []uint64) error {
	cVids := make([]C.uint64_t, len(vids))
	for i, v := range vids {
		cVids[i] = C.uint64_t(v)
	}
	return newError(C.jw_collection_delete_batch(c.handle, &cVids[0], C.size_t(len(vids))))
}

// Clear 清空集合
func (c *Collection) Clear() error {
	return newError(C.jw_collection_clear(c.handle))
}

// Get 根据ID获取向量
func (c *Collection) Get(vid uint64) ([]float32, error) {
	vec := make([]float32, c.dim)
	status := C.jw_collection_get(c.handle, C.uint64_t(vid), (*C.float)(unsafe.Pointer(&vec[0])))
	if status != Success {
		return nil, newError(status)
	}
	return vec, nil
}

// Search 向量搜索
func (c *Collection) Search(query []float32, k int) ([]SearchResult, error) {
	if len(query) != c.dim {
		return nil, &Error{
			Code:    ErrInvalidDimension,
			Message: fmt.Sprintf("dimension mismatch: expected %d, got %d", c.dim, len(query)),
		}
	}
	results := make([]C.jw_search_result_t, k)
	opts := C.jw_search_options_t{
		k:              C.size_t(k),
		filter:         nil,
		include_vectors: C.jw_bool_t(0),
		include_meta:   C.jw_bool_t(0),
		nprobe:         0,
		ef_search:       0,
	}
	actual := C.jw_collection_search(
		c.handle,
		(*C.float)(unsafe.Pointer(&query[0])),
		&opts,
		(*C.jw_search_result_ex_t)(unsafe.Pointer(&results[0])),
	)

	out := make([]SearchResult, 0, actual)
	for i := C.size_t(0); i < actual; i++ {
		out = append(out, SearchResult{
			ID:    uint64(results[i].id),
			Score: float32(results[i].score),
		})
	}
	return out, nil
}

// Count 返回集合中的向量数量
func (c *Collection) Count() int {
	var stats C.jw_collection_stats_t
	C.jw_collection_get_stats(c.handle, &stats)
	return int(stats.count)
}

// BuildIndex 构建索引
func (c *Collection) BuildIndex() error {
	return newError(C.jw_collection_build_index(c.handle))
}

// HasIndex 索引是否就绪
func (c *Collection) HasIndex() bool {
	return int(C.jw_collection_has_index(c.handle)) != 0
}

// DropIndex 删除索引
func (c *Collection) DropIndex() error {
	return newError(C.jw_collection_drop_index(c.handle))
}

// ============================================================
// 内部工具
// ============================================================

func init() {
	// 确保 math 包被引用（编译时需要 -lm）
	_ = math.Abs
}
