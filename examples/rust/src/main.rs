use libc::{c_char, c_float, c_int, c_uint, c_void, size_t, free};
use std::ffi::{CStr, CString};
use std::ptr;

// 定义搜索结果结构体
#[repr(C)]
struct jw_search_result_t {
    id: u64,
    distance: f32,
}

// 声明C函数
extern "C" {
    fn jw_vecdb_open(db: *mut *mut c_void, path: *const c_char, create: c_int) -> c_int;
    fn jw_vecdb_close(db: *mut c_void) -> c_int;
    fn jw_version() -> *const c_char;
    fn jw_collection_create(
        collection: *mut *mut c_void,
        db: *mut c_void,
        name: *const c_char,
        dimension: c_int,
    ) -> c_int;
    fn jw_collection_close(collection: *mut c_void) -> c_int;
    fn jw_collection_list(
        db: *mut c_void,
        names: *mut *mut *mut c_char,
        count: *mut size_t,
    ) -> c_int;
    fn jw_collection_insert(
        collection: *mut c_void,
        vector: *const c_float,
        id: *mut u64,
    ) -> c_int;
    fn jw_collection_search(
        collection: *mut c_void,
        query: *const c_float,
        k: size_t,
        results: *mut *mut *mut jw_search_result_t,
        count: *mut size_t,
    ) -> c_int;
}

fn main() {
    println!("JinWo VecDB Rust Demo");
    println!("====================");

    // 加载库
    #[cfg(target_os = "linux")]
    let library_path = "../../build/libjw_vecdb.so";
    #[cfg(target_os = "windows")]
    let library_path = "../../build/jw_vecdb.dll";
    #[cfg(target_os = "macos")]
    let library_path = "../../build/libjw_vecdb.dylib";

    // 尝试加载库（Rust会自动处理）
    println!("Using library: {}", library_path);

    // 1. 测试版本信息
    unsafe {
        let version = jw_version();
        if !version.is_null() {
            let version_str = CStr::from_ptr(version).to_str().unwrap();
            println!("Version: {}", version_str);
        } else {
            println!("Failed to get version");
        }
    }

    // 2. 打开数据库
    let mut db: *mut c_void = ptr::null_mut();
    let db_path = CString::new("test_db_rust").unwrap();
    let create = 1; // 创建新数据库

    let result = unsafe {
        jw_vecdb_open(&mut db, db_path.as_ptr(), create)
    };

    if result != 0 {
        println!("Failed to open database, error: {}", result);
        return;
    }

    println!("Database opened successfully");

    // 3. 创建集合
    let mut collection: *mut c_void = ptr::null_mut();
    let collection_name = CString::new("test_collection").unwrap();
    let dimension = 128;

    let result = unsafe {
        jw_collection_create(&mut collection, db, collection_name.as_ptr(), dimension)
    };

    if result != 0 {
        println!("Failed to create collection, error: {}", result);
        unsafe {
            jw_vecdb_close(db);
        }
        return;
    }

    println!("Collection created successfully");

    // 4. 插入向量
    let mut vectors = Vec::new();
    for i in 0..5 {
        let mut vector = Vec::with_capacity(dimension);
        for j in 0..dimension {
            vector.push((i * dimension + j) as f32);
        }
        vectors.push(vector);
    }

    for (i, vector) in vectors.iter().enumerate() {
        let mut id: u64 = 0;
        let result = unsafe {
            jw_collection_insert(collection, vector.as_ptr(), &mut id)
        };

        if result != 0 {
            println!("Failed to insert vector {}, error: {}", i, result);
        } else {
            println!("Inserted vector {} with ID: {}", i, id);
        }
    }

    // 5. 搜索向量
    let query_vector = vectors[0].clone();
    let k = 3;
    let mut results: *mut *mut jw_search_result_t = ptr::null_mut();
    let mut count: size_t = 0;

    let result = unsafe {
        jw_collection_search(collection, query_vector.as_ptr(), k, &mut results, &mut count)
    };

    if result != 0 {
        println!("Failed to search, error: {}", result);
    } else {
        println!("Search results (top {}):", count);
        unsafe {
            for i in 0..count {
                let result_ptr = *results.offset(i as isize);
                println!("  ID: {}, Distance: {:.4}", (*result_ptr).id, (*result_ptr).distance);
            }
            // 释放结果内存
            for i in 0..count {
                let result_ptr = *results.offset(i as isize);
                free(result_ptr as *mut c_void);
            }
            free(results as *mut c_void);
        }
    }

    // 6. 列出集合
    let mut names: *mut *mut c_char = ptr::null_mut();
    let mut count: size_t = 0;

    let result = unsafe {
        jw_collection_list(db, &mut names, &mut count)
    };

    if result != 0 {
        println!("Failed to list collections, error: {}", result);
    } else {
        println!("Collections ({}):", count);
        unsafe {
            for i in 0..count {
                let name_ptr = *names.offset(i as isize);
                let name = CStr::from_ptr(name_ptr).to_str().unwrap();
                println!("  {}", name);
                free(name_ptr as *mut c_void);
            }
            free(names as *mut c_void);
        }
    }

    // 7. 清理资源
    unsafe {
        jw_collection_close(collection);
        jw_vecdb_close(db);
    }

    println!("Demo completed successfully");
}
