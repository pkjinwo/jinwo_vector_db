import com.sun.jna.Library
import com.sun.jna.Native
import com.sun.jna.Pointer
import com.sun.jna.Structure

// 定义搜索结果结构体
class SearchResult : Structure() {
    @JvmField var id: Long = 0
    @JvmField var distance: Float = 0f

    override fun getFieldOrder(): List<String> {
        return listOf("id", "distance")
    }
}

// 定义C API接口
interface JinWoVecDB : Library {
    fun jw_vecdb_open(db: PointerByReference, path: String, create: Int): Int
    fun jw_vecdb_close(db: Pointer): Int
    fun jw_version(): String
    fun jw_collection_create(collection: PointerByReference, db: Pointer, name: String, dimension: Int): Int
    fun jw_collection_close(collection: Pointer): Int
    fun jw_collection_list(db: Pointer, names: PointerByReference, count: LongByReference): Int
    fun jw_collection_insert(collection: Pointer, vector: FloatArray, id: LongByReference): Int
    fun jw_collection_search(collection: Pointer, query: FloatArray, k: Long, results: PointerByReference, count: LongByReference): Int
}

// 指针引用类
class PointerByReference : Structure() {
    @JvmField var value: Pointer? = null

    override fun getFieldOrder(): List<String> {
        return listOf("value")
    }
}

// 长整型引用类
class LongByReference : Structure() {
    @JvmField var value: Long = 0

    override fun getFieldOrder(): List<String> {
        return listOf("value")
    }
}

fun main() {
    println("JinWo VecDB Kotlin Demo")
    println("=====================")

    // 加载库
    val libraryPath = when (System.getProperty("os.name").lowercase()) {
        "linux" -> "../../build/libjw_vecdb.so"
        "windows" -> "../../build/jw_vecdb.dll"
        "mac os x" -> "../../build/libjw_vecdb.dylib"
        else -> throw UnsupportedOperationException("Unsupported OS")
    }

    println("Using library: $libraryPath")

    try {
        val jwVecDB = Native.load(libraryPath, JinWoVecDB::class.java) as JinWoVecDB

        // 1. 测试版本信息
        val version = jwVecDB.jw_version()
        println("Version: $version")

        // 2. 打开数据库
        val dbRef = PointerByReference()
        val dbPath = "test_db_kotlin"
        val create = 1 // 创建新数据库

        val result = jwVecDB.jw_vecdb_open(dbRef, dbPath, create)
        if (result != 0) {
            println("Failed to open database, error: $result")
            return
        }

        val db = dbRef.value
        println("Database opened successfully")

        // 3. 创建集合
        val collectionRef = PointerByReference()
        val collectionName = "test_collection"
        val dimension = 128

        val result2 = jwVecDB.jw_collection_create(collectionRef, db, collectionName, dimension)
        if (result2 != 0) {
            println("Failed to create collection, error: $result2")
            jwVecDB.jw_vecdb_close(db)
            return
        }

        val collection = collectionRef.value
        println("Collection created successfully")

        // 4. 插入向量
        val vectors = mutableListOf<FloatArray>()
        for (i in 0 until 5) {
            val vector = FloatArray(dimension)
            for (j in 0 until dimension) {
                vector[j] = (i * dimension + j).toFloat()
            }
            vectors.add(vector)
        }

        for ((i, vector) in vectors.withIndex()) {
            val idRef = LongByReference()
            val result3 = jwVecDB.jw_collection_insert(collection, vector, idRef)
            if (result3 != 0) {
                println("Failed to insert vector $i, error: $result3")
            } else {
                println("Inserted vector $i with ID: ${idRef.value}")
            }
        }

        // 5. 搜索向量
        val queryVector = vectors[0]
        val k = 3L
        val resultsRef = PointerByReference()
        val countRef = LongByReference()

        val result4 = jwVecDB.jw_collection_search(collection, queryVector, k, resultsRef, countRef)
        if (result4 != 0) {
            println("Failed to search, error: $result4")
        } else {
            println("Search results (top ${countRef.value}):")
            val results = resultsRef.value
            for (i in 0 until countRef.value) {
                val resultPtr = results.getPointer(i * SearchResult.SIZE)
                val searchResult = SearchResult()
                searchResult.useMemory(resultPtr)
                println("  ID: ${searchResult.id}, Distance: ${searchResult.distance}")
            }
        }

        // 6. 列出集合
        val namesRef = PointerByReference()
        val countRef2 = LongByReference()

        val result5 = jwVecDB.jw_collection_list(db, namesRef, countRef2)
        if (result5 != 0) {
            println("Failed to list collections, error: $result5")
        } else {
            println("Collections (${countRef2.value}):")
            val names = namesRef.value
            for (i in 0 until countRef2.value) {
                val namePtr = names.getPointer(i * Native.POINTER_SIZE)
                val name = namePtr.getString(0)
                println("  $name")
            }
        }

        // 7. 清理资源
        jwVecDB.jw_collection_close(collection)
        jwVecDB.jw_vecdb_close(db)

        println("Demo completed successfully")

    } catch (e: Exception) {
        println("Error: ${e.message}")
        e.printStackTrace()
    }
}
