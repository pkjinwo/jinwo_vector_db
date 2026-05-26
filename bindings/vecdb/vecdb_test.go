package jinwo_vecdb

import (
	"fmt"
	"math/rand"
	"os"
	"testing"
)

func TestOpenClose(t *testing.T) {
	db, err := Open("/tmp/jw_go_test", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test")

	if !db.IsOpen() {
		t.Fatal("db should be open")
	}
	fmt.Println("Open/Close: ✅")
}

func TestVersion(t *testing.T) {
	db, err := Open("/tmp/jw_go_test2", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test2")

	v := db.Version()
	fmt.Println("Version:", v)
	if v == "" {
		t.Fatal("version should not be empty")
	}
	fmt.Println("Version: ✅")
}

func TestCreateCollection(t *testing.T) {
	db, err := Open("/tmp/jw_go_test3", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test3")

	coll, err := db.CreateCollection("test", 128)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}
	if coll.Count() != 0 {
		t.Fatal("new collection should be empty")
	}
	fmt.Println("CreateCollection: ✅")
}

func TestInsertSearch(t *testing.T) {
	db, err := Open("/tmp/jw_go_test4", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test4")

	coll, err := db.CreateCollection("test", 128)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	// 插入
	vec := make([]float32, 128)
	for i := range vec {
		vec[i] = rand.Float32()
	}
	vid, err := coll.Insert(vec)
	if err != nil {
		t.Fatalf("Insert failed: %v", err)
	}
	fmt.Println("Inserted vid:", vid)

	if coll.Count() != 1 {
		t.Fatalf("expected count=1, got %d", coll.Count())
	}

	// 建索引
	err = coll.BuildIndex()
	if err != nil {
		t.Fatalf("BuildIndex failed: %v", err)
	}

	// 搜索
	results, err := coll.Search(vec, 5)
	if err != nil {
		t.Fatalf("Search failed: %v", err)
	}
	if len(results) == 0 {
		t.Fatal("search should return results")
	}
	fmt.Printf("Search results: id=%d score=%f\n", results[0].ID, results[0].Score)
	fmt.Println("Insert/Search: ✅")
}

func TestInsertBatchDelete(t *testing.T) {
	db, err := Open("/tmp/jw_go_test5", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test5")

	coll, err := db.CreateCollection("test", 64)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	// 批量插入
	vectors := make([][]float32, 10)
	for i := range vectors {
		v := make([]float32, 64)
		for j := range v {
			v[j] = rand.Float32()
		}
		vectors[i] = v
	}
	vids, err := coll.InsertBatch(vectors)
	if err != nil {
		t.Fatalf("InsertBatch failed: %v", err)
	}
	fmt.Println("Batch inserted:", len(vids), "vectors")

	if coll.Count() != 10 {
		t.Fatalf("expected count=10, got %d", coll.Count())
	}

	// 删除
	err = coll.Delete(vids[0])
	if err != nil {
		t.Fatalf("Delete failed: %v", err)
	}
	fmt.Println("Delete: ✅")
}

func TestMemoryDB(t *testing.T) {
	db, err := OpenMemory()
	if err != nil {
		t.Fatalf("OpenMemory failed: %v", err)
	}
	defer db.Close()

	coll, err := db.CreateCollection("test", 32)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	vec := make([]float32, 32)
	for i := range vec {
		vec[i] = 0.5
	}
	vid, err := coll.Insert(vec)
	if err != nil {
		t.Fatalf("Insert failed: %v", err)
	}
	fmt.Println("MemoryDB insert: vid =", vid)
	fmt.Println("MemoryDB: ✅")
}

func TestDimMismatch(t *testing.T) {
	db, err := Open("/tmp/jw_go_test6", Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer db.Close()
	defer os.RemoveAll("/tmp/jw_go_test6")

	coll, err := db.CreateCollection("test", 128)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	// 错误的维度
	badVec := make([]float32, 64)
	_, err = coll.Insert(badVec)
	if err == nil {
		t.Fatal("should fail on dimension mismatch")
	}
	fmt.Println("Dim mismatch check:", err)
	fmt.Println("DimMismatch: ✅")
}
