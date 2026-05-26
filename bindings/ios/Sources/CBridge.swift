/**
 * VecDBError — error codes from the C library.
 */
public enum VecDBError: Int, Error {
    case success           = 0
    case unknown           = -1
    case invalidParam      = -2
    case outOfMemory       = -3
    case notFound          = -4
    case alreadyExists     = -5
    case bufferTooSmall    = -6
    case notSupported      = -7
    case permissionDenied  = -8
    case invalidVector     = -100
    case invalidDimension  = -103
    case indexNotReady     = -104
    case vectorExists      = -105
    case vectorNotFound    = -106
    case collectionExists  = -107
    case collectionNotFound = -108
    case ioError           = -203
    case readOnly          = -206

    public var message: String {
        let ptr = jw_vecdb_strerror(jw_status_t(rawValue: Int32(rawValue)))
        return ptr != nil ? String(cString: ptr!) : "unknown"
    }
}

// MARK: - C Bridge

import Foundation

// Opaque C types
private typealias jw_vecdb_t = OpaquePointer
private typealias jw_collection_t = OpaquePointer
private typealias jw_status_t = Int32

// C function declarations
@_silgen_name("jw_vecdb_open")
private func jw_vecdb_open(_ path: UnsafePointer<CChar>?, _ flags: Int32) -> UnsafeMutablePointer<jw_vecdb_t>?

@_silgen_name("jw_vecdb_close")
private func jw_vecdb_close(_ db: OpaquePointer?)

@_silgen_name("jw_version")
private func jw_version() -> UnsafePointer<CChar>?

@_silgen_name("jw_vecdb_strerror")
private func jw_vecdb_strerror(_ code: jw_status_t) -> UnsafePointer<CChar>?

@_silgen_name("jw_collection_create")
private func jw_collection_create(_ db: OpaquePointer?, _ name: UnsafePointer<CChar>?, _ dim: UInt32) -> UnsafeMutablePointer<jw_collection_t>?

@_silgen_name("jw_collection_close")
private func jw_collection_close(_ coll: OpaquePointer?)

@_silgen_name("jw_collection_insert")
private func jw_collection_insert(_ coll: OpaquePointer?, _ vec: UnsafePointer<Float>?, _ dim: UInt32) -> Int32

@_silgen_name("jw_collection_delete")
private func jw_collection_delete(_ coll: OpaquePointer?, _ vid: UInt64) -> Int32

@_silgen_name("jw_collection_search")
private func jw_collection_search(_ coll: OpaquePointer?, _ query: UnsafePointer<Float>?, _ dim: UInt32, _ k: Int32, _ results: UnsafeMutablePointer<jw_search_result>?) -> Int32

@_silgen_name("jw_collection_build_index")
private func jw_collection_build_index(_ coll: OpaquePointer?) -> Int32

@_silgen_name("jw_collection_encoding_dim")
private func jw_collection_encoding_dim(_ coll: OpaquePointer?) -> UInt32

@_silgen_name("jw_collection_list")
private func jw_collection_list(_ db: OpaquePointer?) -> UnsafeMutablePointer<jw_strlist>?

@_silgen_name("jw_strlist_free")
private func jw_strlist_free(_ list: OpaquePointer?)

// C structs
private struct jw_search_result {
    var id: UInt64
    var distance: Float
}

private struct jw_strlist {
    var items: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
    var count: UInt32
}
