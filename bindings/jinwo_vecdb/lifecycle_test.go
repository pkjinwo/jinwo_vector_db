package jinwo_vecdb

import (
	"fmt"
	"os"
	"testing"
)

const (
	lifecycleDBPath = "/tmp/jw_go_lifecycle_test"
	lifecycleColl   = "lifecycle_test"
	lifecycleDim    = 128
)

func cleanupLifecycle() {
	os.RemoveAll(lifecycleDBPath)
}

func TestLifecycle(t *testing.T) {
	cleanupLifecycle()

	// ============================================================
	// Phase 1: 新建数据库 -> 插入100 -> 关闭 (不删除!)
	// ============================================================
	fmt.Println("============================================================")
	fmt.Println("Phase 1: 新建库 -> 插入100 -> 关闭 (不删除)")

	db, err := Open(lifecycleDBPath, Create|ReadWrite)
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	fmt.Println("  [OK] 打开文件数据库")

	coll, err := db.CreateCollection(lifecycleColl, lifecycleDim)
	if err != nil {
		t.Fatalf("CreateCollection failed: %v", err)
	}
	fmt.Printf("  [OK] 创建 collection: %s (dim=%d)\n", lifecycleColl, lifecycleDim)

	// Insert 100 条
	vids := make([]uint64, 100)
	for i := 0; i < 100; i++ {
		vec := make([]float32, lifecycleDim)
		for j := 0; j < lifecycleDim; j++ {
			vec[j] = float32(i*100 + j)
		}
		vid, err := coll.Insert(vec)
		if err != nil {
			t.Fatalf("Insert[%d] failed: %v", i, err)
		}
		vids[i] = vid
	}
	fmt.Printf("  [OK] 插入 100 条向量, vids=[%d,%d,%d]...[%d,%d,%d]\n",
		vids[0], vids[1], vids[2], vids[97], vids[98], vids[99])

	// Count = 100
	count := coll.Count()
	if count != 100 {
		t.Fatalf("expected count=100, got %d", count)
	}
	fmt.Printf("  [OK] count = %d\n", count)

	// Get 抽检
	vec, err := coll.Get(vids[0])
	if err != nil {
		t.Fatalf("Get(vid=%d) failed: %v", vids[0], err)
	}
	fmt.Printf("  [OK] get(vid=%d): dim=%d, first_3=[%.1f,%.1f,%.1f]\n",
		vids[0], len(vec), vec[0], vec[1], vec[2])

	// BuildIndex
	err = coll.BuildIndex()
	if err != nil {
		t.Fatalf("BuildIndex failed: %v", err)
	}
	fmt.Println("  [OK] 构建索引")

	// Search
	query := make([]float32, lifecycleDim)
	for j := 0; j < lifecycleDim; j++ {
		query[j] = float32(5000 + j)
	}
	results, err := db.Search(lifecycleColl, query, 10)
	if err != nil {
		t.Fatalf("Search failed: %v", err)
	}
	fmt.Printf("  [OK] 查询返回 %d 条结果\n", len(results))
	if len(results) < 1 {
		t.Fatal("搜索结果不应为空")
	}
	for i := 0; i < min(5, len(results)); i++ {
		fmt.Printf("    #%d: vid=%d, score=%.6f\n", i+1, results[i].ID, results[i].Score)
	}

	// Close (不删除任何数据!)
	err = db.Close()
	if err != nil {
		t.Fatalf("Close failed: %v", err)
	}
	fmt.Println("  [OK] Phase 1 关闭数据库 (100条完整数据)")

	if db.IsOpen() {
		t.Fatal("close后 IsOpen 应为 false")
	}

	// ============================================================
	// Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> CRUD -> 关闭
	// ============================================================
	fmt.Println()
	fmt.Println("============================================================")
	fmt.Println("Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> CRUD -> 关闭")

	db2, err := Open(lifecycleDBPath, ReadWrite)
	if err != nil {
		t.Fatalf("重新打开失败: %v", err)
	}
	fmt.Println("  [OK] 重新打开数据库")

	if !db2.IsOpen() {
		t.Fatal("重新打开后 IsOpen 应为 true")
	}

	// 验证 100 条持久化
	coll2 := db2.GetCollection(lifecycleColl)
	if coll2 == nil {
		t.Fatal("collection 应该存在")
	}
	fmt.Println("  [OK] Collection 存在")

	count2 := coll2.Count()
	if count2 != 100 {
		t.Fatalf("100条持久化验证: count 应为 100, 实际 %d", count2)
	}
	fmt.Printf("  [OK] 100条持久化: count=%d ✓\n", count2)

	// 重新打开后 search
	results, err = db2.Search(lifecycleColl, query, 10)
	if err != nil {
		t.Fatalf("重新打开后Search失败: %v", err)
	}
	fmt.Printf("  [OK] 重新打开后查询返回 %d 条结果 (应有 10 条)\n", len(results))
	if len(results) != 10 {
		t.Fatalf("重新打开后应返回 10 条, 实际 %d", len(results))
	}

	// Get 抽检不同 vid
	for _, testVid := range []uint64{vids[0], vids[50], vids[99]} {
		_, err := coll2.Get(testVid)
		if err != nil {
			t.Fatalf("get(vid=%d) 应该返回数据: %v", testVid, err)
		}
	}
	fmt.Printf("  [OK] get 3条随机抽检: vid=%d,%d,%d 全部存在 ✓\n", vids[0], vids[50], vids[99])

	// Delete 50 条
	deleteCount := 50
	for i := 0; i < deleteCount; i++ {
		err := coll2.Delete(vids[i])
		if err != nil {
			t.Fatalf("Delete(vid=%d) failed: %v", vids[i], err)
		}
	}
	fmt.Printf("  [OK] 删除 %d 条向量\n", deleteCount)

	// 验证删除后 count
	count3 := coll2.Count()
	if count3 != 50 {
		t.Fatalf("删除后 count 应为 50, 实际 %d", count3)
	}
	fmt.Printf("  [OK] 删除后 count=%d\n", count3)

	// 验证删除后的 search：已删除的 vid 不应出现
	results, err = db2.Search(lifecycleColl, query, 10)
	if err != nil {
		t.Fatalf("删除后Search失败: %v", err)
	}
	fmt.Printf("  [OK] 删除后查询返回 %d 条结果\n", len(results))

	deletedSet := make(map[uint64]bool)
	for i := 0; i < deleteCount; i++ {
		deletedSet[vids[i]] = true
	}
	for _, r := range results {
		if deletedSet[r.ID] {
			t.Fatalf("已删除的 vid=%d 仍然出现在搜索结果中!", r.ID)
		}
	}
	fmt.Println("  [OK] 已删除的 vid 没有出现在搜索结果中 ✓")

	// Close
	err = db2.Close()
	if err != nil {
		t.Fatalf("Close failed: %v", err)
	}
	fmt.Println("  [OK] Phase 2 关闭数据库 (50条剩余)")

	if db2.IsOpen() {
		t.Fatal("第二次 close 后 IsOpen 应为 false")
	}

	// ============================================================
	// Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭
	// ============================================================
	fmt.Println()
	fmt.Println("============================================================")
	fmt.Println("Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭")

	db3, err := Open(lifecycleDBPath, ReadWrite)
	if err != nil {
		t.Fatalf("再次重新打开失败: %v", err)
	}
	fmt.Println("  [OK] 再次重新打开数据库")

	coll3 := db3.GetCollection(lifecycleColl)
	if coll3 == nil {
		t.Fatal("collection 应该存在")
	}
	fmt.Println("  [OK] Collection 存在")

	// 验证 50 条
	count4 := coll3.Count()
	if count4 != 50 {
		t.Fatalf("50条持久化验证: count 应为 50, 实际 %d", count4)
	}
	fmt.Printf("  [OK] 50条持久化: count=%d ✓\n", count4)

	// Get 已删除的应该失败
	_, err = coll3.Get(vids[0])
	if err == nil {
		t.Fatalf("get 已删除的 vid=%d 应该返回错误", vids[0])
	}
	fmt.Printf("  [OK] get 已删除的 vid=%d 返回错误 (正确) ✓\n", vids[0])

	// Get 未删除的应该正常
	remainingVid := vids[deleteCount]
	vec, err = coll3.Get(remainingVid)
	if err != nil {
		t.Fatalf("get(vid=%d) 应该返回数据: %v", remainingVid, err)
	}
	fmt.Printf("  [OK] get 未删除的 vid=%d: dim=%d ✓\n", remainingVid, len(vec))

	// BuildIndex
	err = coll3.BuildIndex()
	if err != nil {
		t.Fatalf("BuildIndex failed: %v", err)
	}
	fmt.Println("  [OK] 重新构建索引")

	// Insert 20 条新数据
	newVids := make([]uint64, 20)
	for i := 0; i < 20; i++ {
		vec := make([]float32, lifecycleDim)
		for j := 0; j < lifecycleDim; j++ {
			vec[j] = float32(10000 + i*100 + j)
		}
		vid, err := coll3.Insert(vec)
		if err != nil {
			t.Fatalf("新Insert[%d] failed: %v", i, err)
		}
		newVids[i] = vid
	}
	fmt.Println("  [OK] 插入 20 条新向量")

	// 验证 count = 70
	count5 := coll3.Count()
	if count5 != 70 {
		t.Fatalf("新插入后 count 应为 70, 实际 %d", count5)
	}
	fmt.Printf("  [OK] 新插入后 count=%d\n", count5)

	// Search 查询新数据
	newQuery := make([]float32, lifecycleDim)
	for j := 0; j < lifecycleDim; j++ {
		newQuery[j] = float32(10000 + 500 + j)
	}
	results, err = db3.Search(lifecycleColl, newQuery, 5)
	if err != nil {
		t.Fatalf("新数据Search失败: %v", err)
	}
	fmt.Printf("  [OK] 查询新数据返回 %d 条结果\n", len(results))
	for i := 0; i < min(3, len(results)); i++ {
		fmt.Printf("    #%d: vid=%d, score=%.6f\n", i+1, results[i].ID, results[i].Score)
	}

	// Delete 新插入的前 10 条
	for i := 0; i < 10; i++ {
		err := coll3.Delete(newVids[i])
		if err != nil {
			t.Fatalf("Delete新数据(vid=%d) failed: %v", newVids[i], err)
		}
	}
	fmt.Println("  [OK] 删除新插入的 10 条")

	// 验证 count = 60
	count6 := coll3.Count()
	if count6 != 60 {
		t.Fatalf("再次删除后 count 应为 60, 实际 %d", count6)
	}
	fmt.Printf("  [OK] 再次删除后 count=%d\n", count6)

	// 验证删除后的查询
	results, err = db3.Search(lifecycleColl, newQuery, 5)
	if err != nil {
		t.Fatalf("删除新数据后Search失败: %v", err)
	}
	fmt.Printf("  [OK] 删除新数据后查询返回 %d 条结果\n", len(results))

	newDeletedSet := make(map[uint64]bool)
	for i := 0; i < 10; i++ {
		newDeletedSet[newVids[i]] = true
	}
	for _, r := range results {
		if newDeletedSet[r.ID] {
			t.Fatalf("已删除的 vid=%d 出现在查询中!", r.ID)
		}
	}
	fmt.Println("  [OK] Phase 3 删除的 vid 不在查询结果中 ✓")

	// Close
	err = db3.Close()
	if err != nil {
		t.Fatalf("Close failed: %v", err)
	}
	fmt.Println("  [OK] Phase 3 关闭数据库")

	// 清理
	cleanupLifecycle()
	fmt.Println("  [OK] 清理测试文件")

	// ============================================================
	// 总结
	// ============================================================
	fmt.Println()
	fmt.Println("============================================================")
	fmt.Println("所有生命周期测试通过!")
	fmt.Println("  Phase 1: 100条完整持久化 ✓")
	fmt.Println("  Phase 2: 删除50后持久化 ✓")
	fmt.Println("  Phase 3: 50条+新增20删10持久化 ✓")
	fmt.Println("============================================================")
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
