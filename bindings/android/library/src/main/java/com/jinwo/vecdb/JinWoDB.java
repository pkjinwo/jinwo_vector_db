package com.jinwo.vecdb;

/**
 * JinWo VecDB main class.
 * <p>
 * Usage:
 * <pre>{@code
 *   JinWoDB db = new JinWoDB("/data/data/.../mydb.jwv", true);
 *   Collection coll = db.createCollection("docs", 384);
 *   coll.insert(vector);
 *   coll.buildIndex();
 *   List<SearchResult> results = coll.search(query, 5);
 *   coll.close();
 *   db.close();
 * }</pre>
 */
public class JinWoDB implements AutoCloseable {

    static {
        System.loadLibrary("jinwo_vecdb");
    }

    private long ptr;
    private boolean closed = false;

    /** JNI: open/create database */
    private static native long nativeOpen(String path, boolean create);

    /** JNI: close database */
    private static native void nativeClose(long ptr);

    /** JNI: get version string */
    private static native String nativeGetVersion();

    /** JNI: list collections */
    private static native String[] nativeListCollections(long ptr);

    /** JNI: get existing collection by name (for reopen) */
    private static native long nativeGetCollection(long ptr, String name);

    /**
     * Open or create a database.
     * @param path   file path, or ":memory:" for in-memory
     * @param create true to create if not exists
     */
    public JinWoDB(String path, boolean create) {
        ptr = nativeOpen(path, create);
        if (ptr == 0) {
            throw new RuntimeException("jinwo_vecdb: failed to open database '" + path + "'");
        }
    }

    /**
     * Get the library version string.
     */
    public static String getVersion() {
        return nativeGetVersion();
    }

    /**
     * Create a vector collection.
     */
    public Collection createCollection(String name, int dimension) {
        checkClosed();
        return new Collection(ptr, name, dimension);
    }

    /**
     * List all collection names.
     */
    public String[] listCollections() {
        checkClosed();
        return nativeListCollections(ptr);
    }

    /**
     * Get an existing collection by name (for reopen scenarios).
     * @return the Collection, or null if not found
     */
    public Collection getCollection(String name) {
        checkClosed();
        long collPtr = nativeGetCollection(ptr, name);
        if (collPtr == 0) return null;
        return new Collection(collPtr);
    }

    @Override
    public void close() {
        if (!closed) {
            nativeClose(ptr);
            closed = true;
        }
    }

    private void checkClosed() {
        if (closed) throw new IllegalStateException("Database is closed");
    }

    @Override
    protected void finalize() throws Throwable {
        close();
        super.finalize();
    }
}
