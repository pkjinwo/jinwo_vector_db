/**
 * Edge case tests for JinWo VecDB Android/Java binding.
 * 对应 Python test_jinwo_vecdb.py 的边界测试
 *
 * Build and run:
 *   javac -cp . *.java com/jinwo/vecdb/*.java
 *   java -cp . -Djava.library.path=. TestEdge
 */
import com.jinwo.vecdb.*;
import java.io.File;

public class TestEdge {

    static final int DIM = 128;
    static final String DB_PATH = "./test_edge_java.jwv";
    static final String COLL_NAME = "edge_test";

    static int passed = 0, failed = 0;

    static void check(String label, boolean cond) {
        if (cond) {
            System.out.println("  [PASS] " + label);
            passed++;
        } else {
            System.out.println("  [FAIL] " + label);
            failed++;
        }
    }

    static float[] makeVec() {
        float[] v = new float[DIM];
        for (int i = 0; i < DIM; i++) v[i] = 0.5f;
        return v;
    }

    static void cleanup() {
        new File(DB_PATH).delete();
    }

    public static void main(String[] args) {
        cleanup();
        System.out.println("=== JinWo VecDB Java Edge Test ===");
        System.out.println("Version: " + JinWoDB.getVersion());
        System.out.println();

        // ============================================================
        // 新建数据库 + 插入一条 + 构建索引
        // ============================================================
        System.out.println("--- Setup: Open DB + Insert 1 + BuildIndex ---");
        JinWoDB db = new JinWoDB(DB_PATH, true);
        Collection coll = db.createCollection(COLL_NAME, DIM);
        coll.insert(makeVec());
        coll.buildIndex();
        System.out.println("  [OK] 数据库已就绪");
        System.out.println();

        // ============================================================
        // 边界用例
        // ============================================================
        System.out.println("============================================================");
        System.out.println("  EDGE CASES");
        System.out.println("============================================================");

        // ---- Edge 1: 获取不存在的 collection ----
        System.out.println("\n[Edge 1] 获取不存在的 collection");
        Collection nullColl = db.getCollection("nonexistent_coll");
        check("getCollection 返回 null", nullColl == null);

        // ---- Edge 2: listCollections 验证 ----
        System.out.println("\n[Edge 2] 验证 listCollections");
        String[] names = db.listCollections();
        check("listCollections 包含 " + COLL_NAME,
            names != null && names.length >= 1 && names[0].equals(COLL_NAME));

        // ---- Edge 3: 删除不存在的 vid ----
        System.out.println("\n[Edge 3] 删除不存在的 vid");
        try {
            coll.delete(99999);
            check("delete 不存在vid 应抛出异常", false);
        } catch (Exception e) {
            check("delete 不存在vid 抛出预期异常: " + e.getMessage(), true);
        }

        // ---- Edge 4: 插入错误维度向量 ----
        System.out.println("\n[Edge 4] 插入错误维度向量");
        try {
            coll.insert(new float[]{1.0f, 2.0f, 3.0f}); // dim=3, expected 128
            check("insert 错误维度应抛出异常", false);
        } catch (IllegalArgumentException e) {
            check("insert 错误维度抛出预期异常: " + e.getMessage(), true);
        }

        // ---- Edge 5: 搜索时使用错误维度 ----
        System.out.println("\n[Edge 5] 搜索时使用错误维度");
        try {
            coll.search(new float[]{1.0f, 2.0f}, 5); // dim=2, expected 128
            check("search 错误维度应抛出异常", false);
        } catch (IllegalArgumentException e) {
            check("search 错误维度抛出预期异常: " + e.getMessage(), true);
        }

        // ---- Edge 6: 搜索空 collection ----
        System.out.println("\n[Edge 6] 搜索空 collection");
        Collection emptyColl = db.createCollection("empty_coll_test", DIM);
        java.util.List<SearchResult> emptyResults = emptyColl.search(makeVec(), 10);
        check("空 collection 返回 0 条结果", emptyResults != null && emptyResults.size() == 0);
        emptyColl.close();

        // ---- Edge 7: 获取不存在的 vid ----
        System.out.println("\n[Edge 7] 获取不存在的 vid");
        try {
            coll.get(999999);
            check("get 不存在vid 应抛出异常", false);
        } catch (Exception e) {
            check("get 不存在vid 抛出预期异常: " + e.getMessage(), true);
        }

        // ---- Edge 8: 创建重复 collection ----
        System.out.println("\n[Edge 8] 创建重复 collection");
        try {
            db.createCollection(COLL_NAME, DIM);
            check("createCollection 重复名应抛出异常", false);
        } catch (Exception e) {
            check("createCollection 重复名抛出预期异常: " + e.getMessage(), true);
        }

        System.out.println();

        // ============================================================
        // 灾难场景：运行中删除数据库文件
        // Android 环境无子进程隔离，改为在当前进程中验证
        // ============================================================
        System.out.println("============================================================");
        System.out.println("  CATASTROPHE: 运行中删除数据库文件 (同进程验证)");
        System.out.println("============================================================");

        try {
            // 删除数据库文件
            new File(DB_PATH).delete();
            System.out.println("  [>] STAGE1: file_deleted");

            // 内存操作应该不受影响
            java.util.List<SearchResult> sr = coll.search(makeVec(), 5);
            System.out.println("  [>] STAGE2: search_ok results=" + sr.size());

            coll.insert(makeVec());
            System.out.println("  [>] STAGE3: insert_ok");

            System.out.println("  [OK] 灾难场景: 进程未崩溃 ✓");
        } catch (Exception e) {
            System.out.println("  [!] 操作失败 (但进程未崩溃): " + e.getMessage());
        }

        // 清理
        coll.close();
        db.close();
        cleanup();
        System.out.println();

        System.out.println("=======================================");
        System.out.println("Result: " + passed + " passed, " + failed + " failed");
        if (failed > 0) {
            System.exit(1);
        }
    }
}
