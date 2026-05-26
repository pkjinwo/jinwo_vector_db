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
        let ptr = jw_vecdb_strerror(Int32(rawValue))
        return ptr != nil ? String(cString: ptr!) : "unknown"
    }
}

// MARK: - C Bridge

import Foundation

// Opaque C types
typealias jw_vecdb_t = OpaquePointer
typealias jw_collection_t = OpaquePointer
typealias jw_status_t = Int32

// C function declarations
@_silgen_name("jw_vecdb_open")
func jw_vecdb_open(_ path: UnsafePointer<CChar>?, _ flags: Int32) -> OpaquePointer?

@_silgen_name("jw_vecdb_close")
func jw_vecdb_close(_ db: OpaquePointer?)

@_silgen_name("jw_version")
func jw_version() -> UnsafePointer<CChar>?

@_silgen_name("jw_vecdb_strerror")
func jw_vecdb_strerror(_ code: jw_status_t) -> UnsafePointer<CChar>?

@_silgen_name("jw_collection_create")
func jw_collection_create(_ db: OpaquePointer?, _ name: UnsafePointer<CChar>?, _ dim: UInt32) -> OpaquePointer?

@_silgen_name("jw_collection_close")
func jw_collection_close(_ coll: OpaquePointer?)

@_silgen_name("jw_collection_insert")
func jw_collection_insert(_ coll: OpaquePointer?, _ vec: UnsafePointer<Float>?, _ dim: UInt32) -> Int32

@_silgen_name("jw_collection_delete")
func jw_collection_delete(_ coll: OpaquePointer?, _ vid: UInt64) -> Int32

@_silgen_name("jw_collection_search")
func jw_collection_search(_ coll: OpaquePointer?, _ query: UnsafePointer<Float>?, _ dim: UInt32, _ k: Int32, _ results: UnsafeMutablePointer<jw_search_result>?) -> Int32

@_silgen_name("jw_collection_build_index")
func jw_collection_build_index(_ coll: OpaquePointer?) -> Int32

@_silgen_name("jw_collection_encoding_dim")
func jw_collection_encoding_dim(_ coll: OpaquePointer?) -> UInt32

@_silgen_name("jw_collection_list")
func jw_collection_list(_ db: OpaquePointer?) -> UnsafeMutablePointer<jw_strlist>?

@_silgen_name("jw_strlist_free")
func jw_strlist_free(_ list: OpaquePointer?)

// C structs
struct jw_search_result {
    var id: UInt64
    var distance: Float
}

struct jw_strlist {
    var items: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
    var count: UInt32
}
