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
        let flags: Int32 = create ? 0 : 1  // JW_OPEN_READONLY = 1
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
