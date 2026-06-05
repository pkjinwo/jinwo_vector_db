/**
 * 边界用例测试：对应 Python test_jinwo_vecdb.py 的 Edge Cases + 灾难场景
 *
 * 编译运行方式: swift test_edge.swift -I. -L. -ljinwo_vecdb
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
    let dbPath = "/tmp/jw_ios_edge_test"
    let collName = "edge_test"
    let dim = 128

    // Cleanup
    try? FileManager.default.removeItem(atPath: dbPath)

    print("=== JinWo VecDB iOS Edge Test ===")
    print("Version: \(VecDB.version())")
    print()

    // ============================================================
    // 新建数据库 + 插入 1 条 + 构建索引
    // ============================================================
    print("--- Setup: Open DB + Insert 1 + BuildIndex ---")
    let db = try VecDB(path: dbPath, create: true)
    let coll = try db.createCollection(name: collName, dimension: dim)
    let vec = (0..<dim).map { _ in Float(0.5) }
    _ = try coll.insertVid(vec)
    try coll.buildIndex()
    log("[OK] 数据库已就绪")
    print()

    // ============================================================
    // 边界用例
    // ============================================================
    print(String(repeating: "=", count: 60))
    print("  EDGE CASES")
    print(String(repeating: "=", count: 60))

    // ---- Edge 1: 获取不存在的 collection ----
    print("\n[Edge 1] 获取不存在的 collection")
    var nullCollFound = false
    do {
        let _ = try db.getCollection(name: "nonexistent_coll")
    } catch {
        nullCollFound = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(nullCollFound, "getCollection 不存在应抛出错误")
    log("[OK] getCollection 不存在抛出异常 (符合预期) ✓")

    // ---- Edge 2: listCollections 验证 ----
    print("\n[Edge 2] 验证 listCollections")
    let names = try db.listCollections()
    try assert(names.count >= 1, "listCollections 应至少包含1个collection")
    try assert(names.contains(collName), "listCollections 应包含 \(collName)")
    log("[OK] listCollections 包含 \(collName) ✓")

    // ---- Edge 3: 删除不存在的 vid ----
    print("\n[Edge 3] 删除不存在的 vid")
    var deleteNotFound = false
    do {
        try coll.delete(vid: 99999)
    } catch {
        deleteNotFound = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(deleteNotFound, "delete 不存在的 vid 应抛出错误")
    log("[OK] delete 不存在vid 抛出预期异常 ✓")

    // ---- Edge 4: 插入错误维度向量 ----
    print("\n[Edge 4] 插入错误维度向量")
    var wrongDimInsert = false
    do {
        try coll.insert([1.0, 2.0, 3.0]) // dim=3, expected 128
    } catch {
        wrongDimInsert = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(wrongDimInsert, "insert 错误维度应抛出错误")
    log("[OK] insert 错误维度抛出预期异常 ✓")

    // ---- Edge 5: 搜索时使用错误维度 ----
    print("\n[Edge 5] 搜索时使用错误维度")
    var wrongDimSearch = false
    do {
        let _ = try coll.search(query: [1.0, 2.0], k: 5) // dim=2, expected 128
    } catch {
        wrongDimSearch = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(wrongDimSearch, "search 错误维度应抛出错误")
    log("[OK] search 错误维度抛出预期异常 ✓")

    // ---- Edge 6: 搜索空 collection ----
    print("\n[Edge 6] 搜索空 collection")
    let emptyColl = try db.createCollection(name: "empty_coll_test", dimension: dim)
    let emptyQuery = (0..<dim).map { _ in Float(0.5) }
    var emptyResults: [SearchResult] = []
    do {
        emptyResults = try emptyColl.search(query: emptyQuery, k: 10)
    } catch {
        log("[INFO] 搜索空 collection: \(error)")
    }
    try assert(emptyResults.isEmpty, "空 collection 应返回 0 条结果")
    log("[OK] 空 collection 返回 \(emptyResults.count) 条结果 (符合预期) ✓")
    emptyColl.close()

    // ---- Edge 7: 获取不存在的 vid ----
    print("\n[Edge 7] 获取不存在的 vid")
    var getNotFound = false
    do {
        let _ = try coll.get(vid: 999999)
    } catch {
        getNotFound = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(getNotFound, "get 不存在的 vid 应抛出错误")
    log("[OK] get 不存在vid 抛出预期异常 ✓")

    // ---- Edge 8: 创建重复 collection ----
    print("\n[Edge 8] 创建重复 collection")
    var dupCollError = false
    do {
        let _ = try db.createCollection(name: collName, dimension: dim)
    } catch {
        dupCollError = true
        log("[OK] 预期错误: \(error)")
    }
    try assert(dupCollError, "创建重复 collection 应抛出错误")
    log("[OK] 创建重复 collection 抛出预期异常 ✓")

    print()

    // ============================================================
    // 灾难场景：运行中删除数据库文件
    // ============================================================
    print(String(repeating: "=", count: 60))
    print("  CATASTROPHE: 运行中删除数据库文件")
    print(String(repeating: "=", count: 60))

    // 删除数据库文件
    try? FileManager.default.removeItem(atPath: dbPath)
    log("[>] STAGE1: file_deleted")

    // 内存操作应该不受影响
    do {
        let sr = try coll.search(query: vec, k: 5)
        log("[>] STAGE2: search_ok results=\(sr.count)")
    } catch {
        log("[!] STAGE2: search_failed: \(error)")
    }

    do {
        _ = try coll.insertVid(vec)
        log("[>] STAGE3: insert_ok")
    } catch {
        log("[!] STAGE3: insert_failed: \(error)")
    }

    do {
        try coll.delete(vid: 1)
        log("[>] STAGE4: delete_ok")
    } catch {
        log("[!] STAGE4: delete_failed: \(error)")
    }

    // Close
    do {
        coll.close()
        db.close()
        log("[>] STAGE5: close_ok")
    } catch {
        log("[!] STAGE5: close_failed: \(error)")
    }

    log("[OK] 灾难场景: 进程未崩溃 ✓")

    // 清理
    try? FileManager.default.removeItem(atPath: dbPath)
    log("[OK] 清理测试文件")

    print()
    print(String(repeating: "=", count: 60))
    print("所有边缘测试通过! ✓")
    print(String(repeating: "=", count: 60))
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
