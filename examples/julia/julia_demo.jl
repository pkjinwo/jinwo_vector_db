# JinWo VecDB Julia Demo

using Libdl

# 定义搜索结果结构体
typealias SearchResult struct"""
    uint64_t id;
    float distance;
"""

# 加载库
function load_library()
    if Sys.islinux()
        return "../../build/libjw_vecdb.so"
    elseif Sys.iswindows()
        return "../../build/jw_vecdb.dll"
    elseif Sys.isapple()
        return "../../build/libjw_vecdb.dylib"
    else
        error("Unsupported platform")
    end
end

function main()
    println("JinWo VecDB Julia Demo")
    println("====================")

    # 加载库
    library_path = load_library()
    println("Using library: $library_path")

    # 打开库
    lib = Libdl.dlopen(library_path)

    try
        # 1. 测试版本信息
        jw_version = Libdl.dlsym(lib, :jw_version)
        version_ptr = ccall(jw_version, Ptr{UInt8}, ())
        version = unsafe_string(version_ptr)
        println("Version: $version")

        # 2. 打开数据库
        jw_vecdb_open = Libdl.dlsym(lib, :jw_vecdb_open)
        db_ptr = Ref{Ptr{Cvoid}}(C_NULL)
        db_path = "test_db_julia"
        create = 1  # 创建新数据库

        result = ccall(jw_vecdb_open, Cint, (Ref{Ptr{Cvoid}}, Cstring, Cint), db_ptr, db_path, create)
        if result != 0
            println("Failed to open database, error: $result")
            return
        end

        db = db_ptr[]
        println("Database opened successfully")

        # 3. 创建集合
        jw_collection_create = Libdl.dlsym(lib, :jw_collection_create)
        collection_ptr = Ref{Ptr{Cvoid}}(C_NULL)
        collection_name = "test_collection"
        dimension = 128

        result2 = ccall(jw_collection_create, Cint, (Ref{Ptr{Cvoid}}, Ptr{Cvoid}, Cstring, Cint), collection_ptr, db, collection_name, dimension)
        if result2 != 0
            println("Failed to create collection, error: $result2")
            jw_vecdb_close = Libdl.dlsym(lib, :jw_vecdb_close)
            ccall(jw_vecdb_close, Cint, (Ptr{Cvoid},), db)
            return
        end

        collection = collection_ptr[]
        println("Collection created successfully")

        # 4. 插入向量
        jw_collection_insert = Libdl.dlsym(lib, :jw_collection_insert)
        vectors = []
        for i in 0:4
            vector = Float32[i * dimension + j for j in 0:dimension-1]
            push!(vectors, vector)
        end

        for (i, vector) in enumerate(vectors)
            id_ptr = Ref{UInt64}(0)
            result3 = ccall(jw_collection_insert, Cint, (Ptr{Cvoid}, Ptr{Float32}, Ref{UInt64}), collection, vector, id_ptr)
            if result3 != 0
                println("Failed to insert vector $(i-1), error: $result3")
            else
                id = id_ptr[]
                println("Inserted vector $(i-1) with ID: $id")
            end
        end

        # 5. 搜索向量
        jw_collection_search = Libdl.dlsym(lib, :jw_collection_search)
        query_vector = vectors[1]
        k = 3
        results_ptr = Ref{Ptr{Ptr{SearchResult}}}(C_NULL)
        count_ptr = Ref{UInt}(0)

        result4 = ccall(jw_collection_search, Cint, (Ptr{Cvoid}, Ptr{Float32}, UInt, Ref{Ptr{Ptr{SearchResult}}}, Ref{UInt}), collection, query_vector, k, results_ptr, count_ptr)
        if result4 != 0
            println("Failed to search, error: $result4")
        else
            count = count_ptr[]
            println("Search results (top $count):")
            results = results_ptr[]
            for i in 0:count-1
                result_ptr = unsafe_load(results, i+1)
                search_result = unsafe_load(result_ptr)
                println("  ID: $(search_result.id), Distance: $(round(search_result.distance, digits=4))")
            end
        end

        # 6. 列出集合
        jw_collection_list = Libdl.dlsym(lib, :jw_collection_list)
        names_ptr = Ref{Ptr{Ptr{UInt8}}}(C_NULL)
        count_ptr2 = Ref{UInt}(0)

        result5 = ccall(jw_collection_list, Cint, (Ptr{Cvoid}, Ref{Ptr{Ptr{UInt8}}}, Ref{UInt}), db, names_ptr, count_ptr2)
        if result5 != 0
            println("Failed to list collections, error: $result5")
        else
            count = count_ptr2[]
            println("Collections ($count):")
            names = names_ptr[]
            for i in 0:count-1
                name_ptr = unsafe_load(names, i+1)
                name = unsafe_string(name_ptr)
                println("  $name")
            end
        end

        # 7. 清理资源
        jw_collection_close = Libdl.dlsym(lib, :jw_collection_close)
        ccall(jw_collection_close, Cint, (Ptr{Cvoid},), collection)

        jw_vecdb_close = Libdl.dlsym(lib, :jw_vecdb_close)
        ccall(jw_vecdb_close, Cint, (Ptr{Cvoid},), db)

        println("Demo completed successfully")

    finally
        # 关闭库
        Libdl.dlclose(lib)
    end
end

# 运行主函数
main()
