package jinwo_vecdb

import (
	"fmt"
	"math/rand"
	"os"
	"strings"
	"testing"
)

// ============================================================
// 边界测试：对应 Python test_jinwo_vecdb.py 的 Edge Cases
// ============================================================

func setupEdgeDB(t *testing.T) (*DB, *Collection, string) {
	dbPath := "/tmp/jw_go_edge_test"
	os.RemoveAll(dbPath)

	db, err := Open(dbPath, Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}

	coll, err := db.CreateCollection("edge_test", 128)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	// 插入一条数据并构建索引
	vec := make([]float32, 128)
	for i := range vec {
		vec[i] = rand.Float32()
	}
	_, err = coll.Insert(vec)
	if err != nil {
		t.Fatalf("Insert failed: %v", err)
	}
	err = coll.BuildIndex()
	if err != nil {
		t.Fatalf("BuildIndex failed: %v", err)
	}

	return db, coll, dbPath
}

// Edge 1: 搜索不存在的 collection
func TestEdgeSearchNonexistentCollection(t *testing.T) {
	db, _, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 1] 搜索不存在的 collection")
	fmt.Println(strings.Repeat("-", 40))
	_, err := db.Search("nonexistent_coll", make([]float32, 128), 5)
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		fmt.Println("  [INFO] 未报错 (C层可能返回空结果)")
	}
}

// Edge 2: 不存在的 collection - GetCollection 返回 nil
func TestEdgeGetNonexistentCollection(t *testing.T) {
	db, _, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 2] 获取不存在的 collection")
	fmt.Println(strings.Repeat("-", 40))
	coll := db.GetCollection("nonexistent_coll")
	if coll == nil {
		fmt.Println("  [OK] GetCollection 返回 nil (符合预期)")
	} else {
		t.Fatal("GetCollection should return nil for nonexistent collection")
	}
}

// Edge 3: 删除不存在的 collection
func TestEdgeDropNonexistentCollection(t *testing.T) {
	db, _, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 3] 删除不存在的 collection")
	fmt.Println(strings.Repeat("-", 40))
	err := db.DropCollection("nonexistent_coll")
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		fmt.Println("  [INFO] 未报错")
	}
}

// Edge 4: 删除不存在的 vid
func TestEdgeDeleteNonexistentVid(t *testing.T) {
	db, coll, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 4] 删除不存在的 vid")
	fmt.Println(strings.Repeat("-", 40))
	err := coll.Delete(99999)
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		t.Fatal("应该返回错误 (JW_NOT_FOUND)")
	}
}

// Edge 5: 插入错误维度向量
func TestEdgeInsertWrongDim(t *testing.T) {
	db, coll, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 5] 插入错误维度向量")
	fmt.Println(strings.Repeat("-", 40))
	_, err := coll.Insert([]float32{1.0, 2.0, 3.0}) // dim=3, expected 128
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		t.Fatal("应该返回维度不匹配错误")
	}
}

// Edge 6: 搜索时使用错误维度
func TestEdgeSearchWrongDim(t *testing.T) {
	db, coll, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 6] 搜索时使用错误维度")
	fmt.Println(strings.Repeat("-", 40))
	_, err := coll.Search([]float32{1.0, 2.0}, 5) // dim=2, expected 128
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		t.Fatal("应该返回维度不匹配错误")
	}
}

// Edge 7: 搜索空 collection
func TestEdgeSearchEmptyCollection(t *testing.T) {
	db, _, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 7] 搜索空 collection")
	fmt.Println(strings.Repeat("-", 40))
	emptyColl, err := db.CreateCollection("empty_coll_test", 128)
	if err != nil {
		t.Fatalf("创建空 collection 失败: %v", err)
	}

	results, searchErr := emptyColl.Search(make([]float32, 128), 10)
	if searchErr != nil {
		// 空 collection 可能没有索引
		fmt.Printf("  [INFO] 搜索空 collection: %v\n", searchErr)
	} else if len(results) == 0 {
		fmt.Println("  [OK] 空 collection 返回 0 条结果 (符合预期)")
	} else {
		fmt.Printf("  [INFO] 空 collection 返回 %d 条结果\n", len(results))
	}

	db.DropCollection("empty_coll_test")
}

// Edge 8: 获取不存在的 vid
func TestEdgeGetNonexistentVid(t *testing.T) {
	db, coll, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 8] 获取不存在的 vid")
	fmt.Println(strings.Repeat("-", 40))
	_, err := coll.Get(999999)
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		t.Fatal("get 不存在的 vid 应该返回错误")
	}
}

// Edge 9: 创建重复 collection
func TestEdgeCreateDuplicateCollection(t *testing.T) {
	db, _, dbPath := setupEdgeDB(t)
	defer db.Close()
	defer os.RemoveAll(dbPath)

	fmt.Println("\n[Edge 9] 创建重复 collection")
	fmt.Println(strings.Repeat("-", 40))
	_, err := db.CreateCollection("edge_test", 128)
	if err != nil {
		fmt.Printf("  [OK] 预期错误: %v\n", err)
	} else {
		t.Fatal("重复创建 collection 应该返回错误")
	}
}

// ============================================================
// 灾难场景：运行中文件被删除
// Go 编译型语言无法像 Python 一样子进程隔离测试
// 改为直接在当前进程内模拟：删除文件后验证内存操作正常
// ============================================================
func TestCatastropheFileDeleted(t *testing.T) {
	fmt.Println("\n" + strings.Repeat("=", 60))
	fmt.Println("  CATASTROPHE: 运行中删除数据库文件 (同进程验证)")
	fmt.Println(strings.Repeat("=", 60))

	dbPath := "/tmp/jw_go_catastrophe_test"
	os.RemoveAll(dbPath)

	db, err := Open(dbPath, Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	defer os.RemoveAll(dbPath)

	coll, err := db.CreateCollection("cat_coll", 64)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}

	// 插入多条数据
	vids := make([]uint64, 10)
	for i := 0; i < 10; i++ {
		vec := make([]float32, 64)
		for j := 0; j < 64; j++ {
			vec[j] = float32(i*10 + j)
		}
		vid, insertErr := coll.Insert(vec)
		if insertErr != nil {
			t.Fatalf("Insert failed: %v", insertErr)
		}
		vids[i] = vid
	}
	fmt.Println("  [OK] 插入 10 条数据")

	// 删除数据库目录（模拟文件被删）
	os.RemoveAll(dbPath)
	fmt.Println("  [>] STAGE1: directory_deleted")

	// 内存操作应该不受影响
	query := make([]float32, 64)
	for j := 0; j < 64; j++ {
		query[j] = float32(50 + j)
	}

	results, searchErr := db.Search("cat_coll", query, 5)
	if searchErr != nil {
		fmt.Printf("  [!] STAGE2: search_error %v\n", searchErr)
	} else {
		fmt.Printf("  [>] STAGE2: search_ok results=%d\n", len(results))
	}

	_, insertErr := coll.Insert(query)
	if insertErr != nil {
		fmt.Printf("  [!] STAGE3: insert_error %v\n", insertErr)
	} else {
		fmt.Println("  [>] STAGE3: insert_ok")
	}

	deleteErr := coll.Delete(vids[0])
	if deleteErr != nil {
		fmt.Printf("  [!] STAGE4: delete_error %v\n", deleteErr)
	} else {
		fmt.Println("  [>] STAGE4: delete_ok")
	}

	// Close - 目录已删除，close 应正常处理
	closeErr := db.Close()
	if closeErr != nil {
		fmt.Printf("  [!] STAGE5: close_error %v\n", closeErr)
	} else {
		fmt.Println("  [>] STAGE5: close_ok")
	}

	fmt.Println("  [OK] 灾难场景: 进程未崩溃 ✓")
}
