import Foundation

/// JinWo VecDB main class.
///
/// Usage:
/// ```swift
/// let db = try VecDB(path: "mydb.jwv", create: true)
/// let coll = try db.createCollection(name: "docs", dimension: 384)
/// try coll.insert(vector)
/// try coll.buildIndex()
/// let results = try coll.search(query: query, k: 5)
/// coll.close()
/// db.close()
/// ```
public final class VecDB {
    private let ptr: OpaquePointer?
    private var closed = false

    /// Open or create a database.
    /// - Parameters:
    ///   - path: File path, or ":memory:" for in-memory mode.
    ///   - create: If `true`, create the database if it doesn't exist.
    public init(path: String, create: Bool) throws {
        var cPath = path.cString(using: .utf8)!
        // JW_VECDB_CREATE(0x04) | JW_VECDB_READWRITE(0x02) for new DB
        // JW_VECDB_READWRITE(0x02) alone to reopen existing DB
        let flags: Int32 = create ? (0x04 | 0x02) : 0x02
        ptr = jw_vecdb_open(&cPath, flags)
        if ptr == nil {
            throw VecDBError.ioError
        }
    }

    deinit { close() }

    /// Library version string.
    public static func version() -> String {
        let cStr = jw_version()
        return cStr != nil ? String(cString: cStr!) : "unknown"
    }

    /// Create a new vector collection.
    public func createCollection(name: String, dimension: Int) throws -> Collection {
        guard !closed else { throw VecDBError.ioError }
        return Collection(db: ptr, name: name, dimension: dimension)
    }

    /// Get an existing collection by name (for reopen scenario).
    public func getCollection(name: String) throws -> Collection {
        guard !closed else { throw VecDBError.ioError }
        var cName = name.cString(using: .utf8)!
        guard let collPtr = jw_collection_get(ptr, &cName) else {
            throw VecDBError.collectionNotFound
        }
        return Collection(existing: collPtr, name: name)
    }

    /// List all collection names.
    public func listCollections() throws -> [String] {
        guard !closed else { throw VecDBError.ioError }
        guard let raw = jw_collection_list(ptr) else { return [] }
        let list = raw.pointee
        var names: [String] = []
        for i in 0..<Int(list.count) {
            if let item = list.items?.advanced(by: i).pointee {
                names.append(String(cString: item))
            }
        }
        jw_strlist_free(OpaquePointer(raw))
        return names
    }

    /// Close the database.
    public func close() {
        if !closed {
            jw_vecdb_close(ptr)
            closed = true
        }
    }
}
