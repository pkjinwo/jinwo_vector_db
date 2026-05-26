import Foundation

/// A vector collection (table).
/// Created via ``VecDB/createCollection(name:dimension:)``.
/// Must call ``close()`` when done.
public final class Collection {
    private let ptr: OpaquePointer?
    private var closed = false

    init(db: OpaquePointer?, name: String, dimension: Int) {
        var cName = name.cString(using: .utf8)!
        ptr = jw_collection_create(db, &cName, UInt32(dimension))
        if ptr == nil {
            fatalError("jinwo_vecdb: failed to create collection '\(name)'")
        }
    }

    deinit { close() }

    /// Insert a vector. Returns the assigned vector ID.
    @discardableResult
    public func insert(_ vector: [Float]) throws -> UInt64 {
        guard !closed else { throw VecDBError.collectionNotFound }
        let dim = dimension()
        guard vector.count == dim else {
            throw VecDBError.invalidDimension
        }
        let rc = vector.withUnsafeBufferPointer { buf in
            jw_collection_insert(ptr, buf.baseAddress, UInt32(dim))
        }
        if rc != 0 { throw VecDBError(rawValue: Int(rc)) ?? .unknown }
        return 0 // last insert ID approximation
    }

    /// Delete a vector by ID.
    public func delete(vid: UInt64) throws {
        guard !closed else { throw VecDBError.collectionNotFound }
        let rc = jw_collection_delete(ptr, vid)
        if rc != 0 { throw VecDBError(rawValue: Int(rc)) ?? .unknown }
    }

    /// Search for k nearest neighbours.
    public func search(query: [Float], k: Int) throws -> [SearchResult] {
        guard !closed else { throw VecDBError.collectionNotFound }
        let dim = dimension()
        guard query.count == dim else {
            throw VecDBError.invalidDimension
        }
        var results = [jw_search_result](repeating: jw_search_result(id: 0, distance: 0), count: k)
        let count = query.withUnsafeBufferPointer { buf in
            jw_collection_search(ptr, buf.baseAddress, UInt32(dim), Int32(k), &results)
        }
        if count < 0 { throw VecDBError.indexNotReady }
        return Array(results.prefix(Int(count))).map {
            SearchResult(id: $0.id, distance: $0.distance)
        }
    }

    /// Build search index (required before searching).
    public func buildIndex() throws {
        guard !closed else { throw VecDBError.collectionNotFound }
        let rc = jw_collection_build_index(ptr)
        if rc != 0 { throw VecDBError(rawValue: Int(rc)) ?? .unknown }
    }

    /// Encoding dimension of the collection.
    public func dimension() -> Int {
        Int(jw_collection_encoding_dim(ptr))
    }

    /// Close the collection.
    public func close() {
        if !closed {
            jw_collection_close(ptr)
            closed = true
        }
    }
}
