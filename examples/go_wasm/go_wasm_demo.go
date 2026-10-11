// JinWo VecDB Go 演示（wazero + WASM 实现，零 C 工具链）
//
// 运行前：
//   cd examples/go_wasm
//   go run .
//
// 该示例演示推荐方式：通过 bindings/jinwo_vecdb_wasm 在纯 Go 运行时中
// 加载 jinwo.wasm，无需 C 编译器或预编译静态库。

package main

import (
	"fmt"
	"math/rand"

	vecdb "github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm"
)

func randVec(dim int) []float32 {
	v := make([]float32, dim)
	for i := range v {
		v[i] = rand.Float32()*2 - 1
	}
	return v
}

func main() {
	fmt.Println("========================================")
	fmt.Println("  JinWo VecDB Go Demo (wazero + WASM)")
	fmt.Println("========================================")

	ver, err := vecdb.Version()
	if err != nil {
		panic(err)
	}
	fmt.Println("version:", ver)

	// 打开内存数据库
	db, err := vecdb.OpenMemory()
	if err != nil {
		panic(err)
	}
	defer db.Close()

	// 创建集合（128 维）
	const dim = 128
	coll, err := db.CreateCollection("docs", dim)
	if err != nil {
		panic(err)
	}

	// 插入向量
	for i := 0; i < 10; i++ {
		vid, err := coll.Insert(randVec(dim))
		if err != nil {
			panic(err)
		}
		fmt.Printf("inserted vector %d, id=%d\n", i, vid)
	}

	// 构建索引加速检索
	if err := coll.BuildIndex(); err != nil {
		panic(err)
	}

	// 相似度搜索 Top-5
	query := randVec(dim)
	results, err := db.Search("docs", query, 5)
	if err != nil {
		panic(err)
	}
	fmt.Println("search results (top 5):")
	for _, r := range results {
		fmt.Printf("  id=%d, score=%.4f\n", r.ID, r.Score)
	}

	// 删除一条向量
	if len(results) > 0 {
		if err := coll.Delete(results[0].ID); err != nil {
			panic(err)
		}
		fmt.Printf("deleted id=%d\n", results[0].ID)
	}

	fmt.Println("Demo completed successfully!")
}
