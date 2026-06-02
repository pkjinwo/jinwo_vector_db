package com.jinwo.vecdb;

import java.util.ArrayList;
import java.util.List;

/**
 * VecDB collection (table of vectors).
 * <p>
 * Create via {@link JinWoDB#createCollection(String, int)}.
 * Must call {@link #close()} when done.
 */
public class Collection implements AutoCloseable {

    private final long ptr;
    private boolean closed = false;

    /** JNI: create collection */
    private static native long nativeCreateCollection(long dbPtr, String name, int dimension);

    /** JNI: close collection */
    private static native void nativeCloseCollection(long ptr);

    /** JNI: insert vector, returns vector ID */
    private static native long nativeInsert(long ptr, float[] vector, int dim);

    /** JNI: delete vector by ID */
    private static native int nativeDelete(long ptr, long vid);

    /** JNI: search KNN */
    private static native long[] nativeSearch(long ptr, float[] query, int dim, int k);

    /** JNI: build index */
    private static native int nativeBuildIndex(long ptr);

    /** JNI: get encoding dimension */
    private static native int nativeGetDimension(long ptr);

    /** JNI: get vector by ID */
    private static native float[] nativeGetVector(long ptr, long vid);

    /** JNI: get collection count */
    private static native long nativeGetCount(long ptr);

    Collection(long dbPtr, String name, int dimension) {
        this.ptr = nativeCreateCollection(dbPtr, name, dimension);
        if (this.ptr == 0) {
            throw new RuntimeException("jinwo_vecdb: failed to create collection '" + name + "'");
        }
    }

    /** Construct from existing collection pointer (for getCollection / reopen). */
    Collection(long ptr) {
        this.ptr = ptr;
    }

    long getPtr() { return ptr; }

    /**
     * Insert a vector.
     * @return the assigned vector ID.
     */
    public long insert(float[] vector) {
        checkClosed();
        int dim = getDimension();
        if (vector.length != dim) {
            throw new IllegalArgumentException("jinwo_vecdb: dimension mismatch: expected " + dim + ", got " + vector.length);
        }
        return nativeInsert(ptr, vector, dim);
    }

    /**
     * Delete a vector by its ID.
     */
    public void delete(long vid) {
        checkClosed();
        int rc = nativeDelete(ptr, vid);
        if (rc != 0) {
            throw new RuntimeException("jinwo_vecdb: delete failed (code=" + rc + ")");
        }
    }

    /**
     * Search for the k nearest neighbours.
     */
    public List<SearchResult> search(float[] query, int k) {
        checkClosed();
        int dim = getDimension();
        if (query.length != dim) {
            throw new IllegalArgumentException("jinwo_vecdb: dimension mismatch: expected " + dim + ", got " + query.length);
        }
        long[] raw = nativeSearch(ptr, query, dim, k);
        if (raw == null) return new ArrayList<>();

        List<SearchResult> results = new ArrayList<>(raw.length / 2);
        for (int i = 0; i < raw.length; i += 2) {
            long id = raw[i];
            float dist = Float.intBitsToFloat((int) raw[i + 1]);
            results.add(new SearchResult(id, dist));
        }
        return results;
    }

    /**
     * Build search index (required before searching).
     */
    public void buildIndex() {
        checkClosed();
        int rc = nativeBuildIndex(ptr);
        if (rc != 0) {
            throw new RuntimeException("jinwo_vecdb: buildIndex failed (code=" + rc + ")");
        }
    }

    /**
     * Get a vector by its ID.
     */
    public float[] get(long vid) {
        checkClosed();
        float[] vec = nativeGetVector(ptr, vid);
        if (vec == null) {
            throw new RuntimeException("jinwo_vecdb: get failed for vid=" + vid);
        }
        return vec;
    }

    /**
     * Get number of vectors in the collection.
     */
    public long count() {
        checkClosed();
        return nativeGetCount(ptr);
    }

    /**
     * Get the encoding dimension of this collection.
     */
    public int getDimension() {
        checkClosed();
        return nativeGetDimension(ptr);
    }

    @Override
    public void close() {
        if (!closed) {
            nativeCloseCollection(ptr);
            closed = true;
        }
    }

    private void checkClosed() {
        if (closed) throw new IllegalStateException("Collection is closed");
    }
}
