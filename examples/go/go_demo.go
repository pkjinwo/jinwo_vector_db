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

// jwStr 把 Go 字符串构造成 C 的 jw_str_t（ptr + slen）。
func jwStr(s string) C.jw_str_t {
	cs := C.CString(s)
	// 注意：返回的 jw_str_t 持有 cs，调用方需负责 C.free(cs)。
	return C.jw_str_t{ptr: cs, slen: C.size_t(len(s))}
}

func main() {
	// 打开/创建数据库
	cPath := jwStr("./vecdb_go")
	defer C.free(unsafe.Pointer(cPath.ptr))

	var db *C.jw_vecdb_t
	if C.jw_vecdb_open(&cPath, C.uint32_t(C.JW_VECDB_CREATE), &db) != 0 {
		panic("jw_vecdb_open failed")
	}
	defer C.jw_vecdb_close(db)

	// 创建集合
	cName := jwStr("test")
	defer C.free(unsafe.Pointer(cName.ptr))

	var coll *C.jw_collection_t
	if C.jw_vecdb_create_collection(db, &cName, C.uint32_t(4), &coll) != 0 {
		panic("jw_vecdb_create_collection failed")
	}

	// 插入向量
	vec := []float32{0.1, 0.2, 0.3, 0.4}
	cVec := (*C.float)(unsafe.Pointer(&vec[0]))
	var id C.uint64_t
	if C.jw_collection_insert(coll, cVec, &id) != 0 {
		panic("jw_collection_insert failed")
	}
	fmt.Printf("inserted id=%d\n", uint64(id))

	// 相似度搜索（结果写入 Go 分配的 jw_search_result_t 数组）
	q := (*C.float)(unsafe.Pointer(&vec[0]))
	var results [5]C.jw_search_result_t
	count := C.jw_vecdb_search(
		db, &cName, q,
		C.uint32_t(4), C.size_t(5),
		(*C.jw_search_result_t)(unsafe.Pointer(&results[0])),
	)
	if count > 0 {
		fmt.Printf("got %d results, top id=%d score=%.4f\n",
			uint64(count), uint64(results[0].id), float32(results[0].score))
	} else {
		fmt.Println("search returned 0 results")
	}
}
