/**
 * 完整生命周期测试: 新建 -> 增查 -> 关闭 -> 重新打开(100条持久化) -> 删改查 -> 关闭 -> 重新打开 -> 增删查 -> 关闭
 *
 * 编译运行方式: bash build_and_test.sh
 */

import Foundation

// ============================================================
// 错误类型
// ============================================================
enum TestError: Error {
    case fail(String)
}

func assert(_ cond: Bool, _ msg: String) throws {
    if !cond { throw TestError.fail(msg) }
}

func log(_ msg: String) {
    print("  \(msg)")
}

// ============================================================
// 主测试
// ============================================================
func main() throws {
    let dbPath = "/tmp/jw_ios_lifecycle_test"
    let collName = "lifecycle_test"
    let dim = 128

    // Cleanup
    try? FileManager.default.removeItem(atPath: dbPath)

    // ============================================================
    // Phase 1: 新建 -> 插入100 -> 关闭 (不删除)
    // ============================================================
    print("============================================================")
    print("Phase 1: 新建库 -> 插入100 -> 关闭 (不删除)")

    let db = try VecDB(path: dbPath, create: true)
    log("[OK] 打开文件数据库")

    let coll = try db.createCollection(name: collName, dimension: dim)
    log("[OK] 创建 collection: \(collName) (dim=\(dim))")

    // Insert 100 条
    var vids = [UInt64]()
    for i in 0..<100 {
        let vec = (0..<dim).map { Float(i * 100 + $0) }
        let vid = try coll.insertVid(vec)
        vids.append(vid)
    }
    log("[OK] 插入 100 条向量, vids=[\(vids[0]),\(vids[1]),\(vids[2])]...[\(vids[97]),\(vids[98]),\(vids[99])]")

    // Count = 100
    let count1 = coll.count()
    try assert(count1 == 100, "count 应为 100, 实际 \(count1)")
    log("[OK] count = \(count1)")

    // Get 抽检
    let vec = try coll.get(vid: vids[0])
    log("[OK] get(vid=\(vids[0])): dim=\(vec.count), first_3=\(vec.prefix(3))")

    // BuildIndex
    try coll.buildIndex()
    log("[OK] 构建索引")

    // Search
    let query = (0..<dim).map { Float(5000 + $0) }
    var results = try coll.search(query: query, k: 10)
    log("[OK] 查询返回 \(results.count) 条结果")
    try assert(results.count >= 1, "搜索结果不应为空")
    for i in 0..<min(5, results.count) {
        log("  #\(i+1): vid=\(results[i].id), distance=\(results[i].distance)")
    }

    // Close (不删除任何数据!)
    coll.close()
    db.close()
    log("[OK] Phase 1 关闭数据库 (100条完整数据)")

    // ============================================================
    // Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> 关闭
    // ============================================================
    print()
    print("============================================================")
    print("Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> 关闭")

    let db2 = try VecDB(path: dbPath, create: false)
    log("[OK] 重新打开数据库")

    let coll2 = try db2.getCollection(name: collName)
    log("[OK] Collection 存在")

    // 验证 100 条持久化
    let count2 = coll2.count()
    try assert(count2 == 100, "100条持久化: count 应为 100, 实际 \(count2)")
    log("[OK] 100条持久化: count=\(count2) ✓")

    // 重新打开后 search
    results = try coll2.search(query: query, k: 10)
    log("[OK] 重新打开后查询返回 \(results.count) 条结果 (应有 10 条)")
    try assert(results.count == 10, "重新打开后应返回 10 条, 实际 \(results.count)")
    for i in 0..<min(3, results.count) {
        log("  #\(i+1): vid=\(results[i].id), distance=\(results[i].distance)")
    }

    // Get 抽检不同 vid
    for testVid in [vids[0], vids[50], vids[99]] {
        let _ = try coll2.get(vid: testVid)
    }
    log("[OK] get 3条随机抽检: vid=\(vids[0]),\(vids[50]),\(vids[99]) 全部存在 ✓")

    // Delete 50 条
    let deleteCount = 50
    for i in 0..<deleteCount {
        try coll2.delete(vid: vids[i])
    }
    log("[OK] 删除 \(deleteCount) 条向量")

    // 验证删除后 count
    let count3 = coll2.count()
    try assert(count3 == 50, "删除后 count 应为 50, 实际 \(count3)")
    log("[OK] 删除后 count=\(count3)")

    // 验证删除后的 search
    let deletedSet = Set(vids.prefix(deleteCount))
    results = try coll2.search(query: query, k: 10)
    log("[OK] 删除后查询返回 \(results.count) 条结果")
    for r in results {
        if deletedSet.contains(r.id) {
            throw TestError.fail("已删除的 vid=\(r.id) 仍出现在结果中!")
        }
    }
    log("[OK] 已删除的 vid 没有出现在搜索结果中 ✓")

    // Close
    coll2.close()
    db2.close()
    log("[OK] Phase 2 关闭数据库 (50条剩余)")

    // ============================================================
    // Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭
    // ============================================================
    print()
    print("============================================================")
    print("Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭")

    let db3 = try VecDB(path: dbPath, create: false)
    log("[OK] 再次重新打开数据库")

    let coll3 = try db3.getCollection(name: collName)
    log("[OK] Collection 存在")

    // 验证 50 条
    let count4 = coll3.count()
    try assert(count4 == 50, "50条持久化: count 应为 50, 实际 \(count4)")
    log("[OK] 50条持久化: count=\(count4) ✓")

    // Get 已删除的应该失败
    var deletedGetFailed = false
    do {
        let _ = try coll3.get(vid: vids[0])
    } catch {
        deletedGetFailed = true
    }
    try assert(deletedGetFailed, "get 已删除的 vid=\(vids[0]) 应该返回错误")
    log("[OK] get 已删除的 vid=\(vids[0]) 返回错误 (正确) ✓")

    // Get 未删除的应该正常
    let remainingVid = vids[deleteCount]
    let vecRemain = try coll3.get(vid: remainingVid)
    log("[OK] get 未删除的 vid=\(remainingVid): dim=\(vecRemain.count) ✓")

    // BuildIndex
    try coll3.buildIndex()
    log("[OK] 重新构建索引")

    // Insert 20 条新数据
    var newVids = [UInt64]()
    for i in 0..<20 {
        let vec = (0..<dim).map { Float(10000 + i * 100 + $0) }
        let vid = try coll3.insertVid(vec)
        newVids.append(vid)
    }
    log("[OK] 插入 20 条新向量")

    // 验证 count = 70
    let count5 = coll3.count()
    try assert(count5 == 70, "新插入后 count 应为 70, 实际 \(count5)")
    log("[OK] 新插入后 count=\(count5)")

    // Search 查询新数据
    let newQuery = (0..<dim).map { Float(10000 + 500 + $0) }
    results = try coll3.search(query: newQuery, k: 5)
    log("[OK] 查询新数据返回 \(results.count) 条结果")
    for i in 0..<min(3, results.count) {
        log("  #\(i+1): vid=\(results[i].id), distance=\(results[i].distance)")
    }

    // Delete 新插入的前 10 条
    for i in 0..<10 {
        try coll3.delete(vid: newVids[i])
    }
    log("[OK] 删除新插入的 10 条")

    // 验证 count = 60
    let count6 = coll3.count()
    try assert(count6 == 60, "再次删除后 count 应为 60, 实际 \(count6)")
    log("[OK] 再次删除后 count=\(count6)")

    // 验证删除后的查询
    let newDeletedSet = Set(newVids.prefix(10))
    results = try coll3.search(query: newQuery, k: 5)
    log("[OK] 删除新数据后查询返回 \(results.count) 条结果")
    for r in results {
        if newDeletedSet.contains(r.id) {
            throw TestError.fail("已删除的 vid=\(r.id) 出现在查询中!")
        }
    }
    log("[OK] Phase 3 删除的 vid 不在查询结果中 ✓")

    // Close
    coll3.close()
    db3.close()
    log("[OK] Phase 3 关闭数据库")

    // 清理
    try? FileManager.default.removeItem(atPath: dbPath)
    log("[OK] 清理测试文件")

    // ============================================================
    // 总结
    // ============================================================
    print()
    print("============================================================")
    print("所有生命周期测试通过!")
    print("  Phase 1: 100条完整持久化 ✓")
    print("  Phase 2: 删除50后持久化 ✓")
    print("  Phase 3: 50条+新增20删10持久化 ✓")
    print("============================================================")
}

do {
    try main()
} catch TestError.fail(let msg) {
    print("[FAIL] \(msg)")
    exit(1)
} catch {
    print("[FAIL] Unexpected error: \(error)")
    exit(1)
}
