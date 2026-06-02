/**
 * Full lifecycle test for JinWo VecDB Android/Java binding.
 *
 * Phase 1: Create DB -> Insert 100 -> Verify counts -> Close (no delete)
 * Phase 2: Reopen DB -> Verify 100 persisted -> Search -> Delete 50 -> Update checks -> Close
 * Phase 3: Reopen DB -> Verify 50 -> Insert 20 -> Delete 10 -> Close
 *
 * Build and run:
 *   javac -cp . *.java com/jinwo/vecdb/*.java
 *   java -cp . -Djava.library.path=. TestLifecycle
 */

import com.jinwo.vecdb.*;
import java.io.File;
import java.util.List;

public class TestLifecycle {

    static final int DIM = 128;
    static final String DB_PATH = "./test_lifecycle_java.jwv";

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

    static float[] makeVec(long seed, int dim) {
        float[] v = new float[dim];
        for (int i = 0; i < dim; i++) {
            v[i] = ((seed * 1103515245L + 12345) * i + seed) % 1000 / 1000.0f;
        }
        return v;
    }

    static void cleanup() {
        new File(DB_PATH).delete();
    }

    public static void main(String[] args) {
        cleanup();
        System.out.println("=== JinWo VecDB Java Lifecycle Test ===");
        System.out.println("Version: " + JinWoDB.getVersion());
        System.out.println();

        // ====== Phase 1: Create, insert 100, close without deleting ======
        System.out.println("--- Phase 1: Create + Insert 100 + Close (no delete) ---");
        {
            JinWoDB db = new JinWoDB(DB_PATH, true);
            Collection coll = db.createCollection("test", DIM);
            String[] names = db.listCollections();
            check("listCollections returns 1", names != null && names.length == 1);

            long[] vids = new long[100];
            for (int i = 0; i < 100; i++) {
                vids[i] = coll.insert(makeVec(i, DIM));
                check("Phase1 insert[" + i + "] vid=" + vids[i], vids[i] > 0);
            }
            check("Phase1 count = 100", coll.count() == 100);

            // Verify get for first, middle, last
            for (int i : new int[]{0, 49, 99}) {
                float[] vec = coll.get(vids[i]);
                float[] expected = makeVec(i, DIM);
                boolean ok = true;
                for (int j = 0; j < DIM; j++) {
                    if (Math.abs(vec[j] - expected[j]) > 0.0001f) { ok = false; break; }
                }
                check("Phase1 get vid=" + vids[i], ok);
            }

            coll.close();
            db.close();
            System.out.println("  Phase1 done. File exists: " + new File(DB_PATH).exists());
        }
        System.out.println();

        // ====== Phase 2: Reopen, verify 100 persisted, search, delete 50, close ======
        System.out.println("--- Phase 2: Reopen + Verify 100 + Search + Delete 50 + Close ---");
        {
            JinWoDB db = new JinWoDB(DB_PATH, false);
            Collection coll = db.getCollection("test");
            check("Phase2 getCollection not null", coll != null);
            check("Phase2 count = 100", coll.count() == 100);

            // Verify get still works for some vectors
            float[] vec0 = coll.get(1);
            check("Phase2 get vid=1 not null", vec0 != null && vec0.length == DIM);
            float[] vec50 = coll.get(50);
            check("Phase2 get vid=50 not null", vec50 != null && vec50.length == DIM);

            // Build index and search
            coll.buildIndex();
            float[] query = makeVec(42, DIM);
            List<SearchResult> results = coll.search(query, 10);
            check("Phase2 search returns 10 results", results != null && results.size() == 10);
            check("Phase2 search result[0] has id>0", results.get(0).getId() > 0);
            check("Phase2 search result[0] distance non-negative", results.get(0).getDistance() >= 0);

            // Delete 50 entries (vids 51-100)
            for (int i = 51; i <= 100; i++) {
                coll.delete(i);
            }
            check("Phase2 count after delete 50 = 50", coll.count() == 50);

            // Verify deleted entries are gone (get should throw)
            boolean deletedOk = true;
            try {
                coll.get(51);
                deletedOk = false;
            } catch (Exception e) {
                // expected
            }
            check("Phase2 get deleted vid=51 throws", deletedOk);

            coll.close();
            db.close();
        }
        System.out.println();

        // ====== Phase 3: Reopen, verify 50, insert 20, delete 10, close ======
        System.out.println("--- Phase 3: Reopen + Verify 50 + Insert 20 + Delete 10 + Close ---");
        {
            JinWoDB db = new JinWoDB(DB_PATH, false);
            Collection coll = db.getCollection("test");
            check("Phase3 getCollection not null", coll != null);
            check("Phase3 count = 50", coll.count() == 50);

            // Insert 20 new vectors
            for (int i = 0; i < 20; i++) {
                long vid = coll.insert(makeVec(200 + i, DIM));
                check("Phase3 insert[" + i + "] vid>0", vid > 0);
            }
            check("Phase3 count after insert 20 = 70", coll.count() == 70);

            // Delete 10 more
            for (int i = 1; i <= 10; i++) {
                coll.delete(i);
            }
            check("Phase3 count after delete 10 = 60", coll.count() == 60);

            coll.close();
            db.close();
        }
        System.out.println();

        cleanup();
        System.out.println("=======================================");
        System.out.println("Result: " + passed + " passed, " + failed + " failed");
        if (failed > 0) {
            System.exit(1);
        }
    }
}
