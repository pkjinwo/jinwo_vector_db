# JinWo VecDB Android 平台集成指南

**版本**：v1.0.0
**生成时间**：2026-04-26
**文档类型**：对外发布

---

## 1. 平台概述

### 1.1 支持的Android版本

| Android版本 | 支持状态 | 最低API级别 |
|------------|---------|------------|
| Android 6.0+ | ✅ 支持 | API 23 |
| Android 7.0+ | ✅ 支持 | API 24 |
| Android 8.0+ | ✅ 支持 | API 26 |
| Android 9.0+ | ✅ 支持 | API 28 |
| Android 10.0+ | ✅ 支持 | API 29 |
| Android 11.0+ | ✅ 支持 | API 30 |
| Android 12.0+ | ✅ 支持 | API 31 |
| Android 13.0+ | ✅ 支持 | API 33 |

### 1.2 系统要求

| 组件 | 要求 |
|------|------|
| CPU | ARMv8-A 64位架构 |
| 内存 | 至少 2GB RAM |
| 存储 | 至少 100MB 可用空间 |
| JDK | JDK 11+ |
| Android Studio | Android Studio 2022.2+ |
| NDK | NDK r25+ |

---

## 2. 集成方式

### 2.1 AAR 库集成

1. **下载 AAR 库**
   - 从 GitHub Release 页面下载最新的 `jinwo_vecdb-android-{version}.aar` 文件

2. **添加到项目**
   - 将 AAR 文件复制到 `app/libs` 目录
   - 在 `build.gradle` 文件中添加依赖：

   ```groovy
   dependencies {
       implementation files('libs/jinwo_vecdb-android-1.0.0.aar')
   }
   ```

3. **同步项目**
   - 点击 "Sync Now" 同步项目依赖

### 2.2 CMake 集成（源码编译）

1. **克隆源码**
   ```bash
   git clone https://github.com/pkjinwo/jinwo_vector_db.git
   cd jinwo_vecdb
   ```

2. **创建 CMakeLists.txt**
   - 在 Android 项目的 `app/src/main/cpp` 目录创建 `CMakeLists.txt`：

   ```cmake
   cmake_minimum_required(VERSION 3.22.1)
   
   project("jinwo_vecdb")
   
   # 设置 JinWo VecDB 源码路径
   set(JW_VECDB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../jinwo_vecdb)
   
   # 添加源码
   add_library(jw_vecdb SHARED
       ${JW_VECDB_SOURCE_DIR}/src/jw_vecdb.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_collection.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_vector.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_index.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_memory.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_storage.c
       ${JW_VECDB_SOURCE_DIR}/src/jw_util.c
       ${CMAKE_CURRENT_SOURCE_DIR}/jni_wrapper.cpp
   )
   
   # 包含头文件
   target_include_directories(jw_vecdb PRIVATE
       ${JW_VECDB_SOURCE_DIR}/include
   )
   
   # 链接库
   find_library(log-lib log)
   target_link_libraries(jw_vecdb ${log-lib})
   ```

3. **创建 JNI 包装器**
   - 在 `app/src/main/cpp` 目录创建 `jni_wrapper.cpp`：

   ```cpp
   #include <jni.h>
   #include "jw_vecdb.h"
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   
   // JNI 方法实现
   
   #ifdef __cplusplus
   }
   #endif
   ```

4. **配置 build.gradle**
   - 在 `app/build.gradle` 中添加：

   ```groovy
   android {
       externalNativeBuild {
           cmake {
               path "src/main/cpp/CMakeLists.txt"
           }
       }
   }
   ```

---

## 3. API 封装

### 3.1 Java 封装类

```java
// com/example/jinwovecdb/JinWoVecDB.java
package com.example.jinwovecdb;

public class JinWoVecDB {
    static {
        System.loadLibrary("jw_vecdb");
    }
    
    // 数据库操作
    public native long open(String path, boolean create);
    public native int close(long dbPtr);
    
    // 集合操作
    public native long createCollection(long dbPtr, String name, int dimension);
    public native long openCollection(long dbPtr, String name);
    public native int dropCollection(long dbPtr, String name);
    public native String[] listCollections(long dbPtr);
    public native int closeCollection(long colPtr);
    
    // 向量操作
    public native long insertVector(long colPtr, float[] vector);
    public native float[][] searchVectors(long colPtr, float[] query, int k);
    public native long[] searchVectorIds(long colPtr, float[] query, int k);
    public native int deleteVector(long colPtr, long id);
    
    // 索引操作
    public native int createIndex(long colPtr, String indexType, String params);
}
```

### 3.2 Kotlin 扩展

```kotlin
// com/example/jinwovecdb/JinWoVecDB.kt
package com.example.jinwovecdb

class JinWoVecDBWrapper {
    private val vecdb = JinWoVecDB()
    private var dbPtr: Long = 0
    private var colPtr: Long = 0
    
    fun open(path: String, create: Boolean = true): Boolean {
        dbPtr = vecdb.open(path, create)
        return dbPtr != 0L
    }
    
    fun close() {
        if (colPtr != 0L) {
            vecdb.closeCollection(colPtr)
            colPtr = 0
        }
        if (dbPtr != 0L) {
            vecdb.close(dbPtr)
            dbPtr = 0
        }
    }
    
    fun createCollection(name: String, dimension: Int): Boolean {
        colPtr = vecdb.createCollection(dbPtr, name, dimension)
        return colPtr != 0L
    }
    
    fun openCollection(name: String): Boolean {
        colPtr = vecdb.openCollection(dbPtr, name)
        return colPtr != 0L
    }
    
    fun insertVector(vector: FloatArray): Long {
        return vecdb.insertVector(colPtr, vector)
    }
    
    fun searchVectors(query: FloatArray, k: Int = 10): Pair<Array<FloatArray>, LongArray> {
        val vectors = vecdb.searchVectors(colPtr, query, k)
        val ids = vecdb.searchVectorIds(colPtr, query, k)
        return Pair(vectors, ids)
    }
    
    fun deleteVector(id: Long): Boolean {
        return vecdb.deleteVector(colPtr, id) == 0
    }
}
```

---

## 4. 示例代码

### 4.1 基本用法

```kotlin
// MainActivity.kt
import com.example.jinwovecdb.JinWoVecDBWrapper

class MainActivity : AppCompatActivity() {
    private lateinit var vecdb: JinWoVecDBWrapper
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        vecdb = JinWoVecDBWrapper()
        initVectorDB()
    }
    
    private fun initVectorDB() {
        // 打开数据库
        val dbPath = filesDir.absolutePath + "/vecdb"
        if (!vecdb.open(dbPath)) {
            Log.e("VecDB", "Failed to open database")
            return
        }
        
        // 创建集合
        if (!vecdb.createCollection("test", 128)) {
            Log.e("VecDB", "Failed to create collection")
            return
        }
        
        // 插入向量
        val vector = FloatArray(128) {
            (Math.random() * 2 - 1).toFloat() // 随机向量
        }
        val id = vecdb.insertVector(vector)
        Log.d("VecDB", "Inserted vector with id: $id")
        
        // 搜索向量
        val (results, resultIds) = vecdb.searchVectors(vector)
        Log.d("VecDB", "Search found ${results.size} results")
        
        // 关闭数据库
        vecdb.close()
    }
}
```

### 4.2 高级用法

```kotlin
// VectorSearchManager.kt
class VectorSearchManager {
    private val vecdb = JinWoVecDBWrapper()
    private val executor = Executors.newSingleThreadExecutor()
    
    fun initialize(callback: (Boolean) -> Unit) {
        executor.execute {
            val success = vecdb.open("/data/data/com.example.app/vecdb")
            runOnUiThread { callback(success) }
        }
    }
    
    fun insertVectors(vectors: List<FloatArray>, callback: (List<Long>) -> Unit) {
        executor.execute {
            val ids = vectors.map { vecdb.insertVector(it) }
            runOnUiThread { callback(ids) }
        }
    }
    
    fun search(query: FloatArray, k: Int = 10, callback: (List<Pair<FloatArray, Long>>) -> Unit) {
        executor.execute {
            val (vectors, ids) = vecdb.searchVectors(query, k)
            val results = vectors.zip(ids.toList())
            runOnUiThread { callback(results) }
        }
    }
    
    fun shutdown() {
        executor.execute {
            vecdb.close()
        }
        executor.shutdown()
    }
}
```

---

## 5. 性能优化

### 5.1 内存管理

| 优化项 | 建议 |
|-------|------|
| 向量批处理 | 使用批处理插入，减少JNI调用次数 |
| 内存分配 | 重用向量数组，避免频繁创建 |
| 缓存策略 | 缓存常用查询结果 |
| 索引优化 | 根据数据量选择合适的索引类型 |

### 5.2 并发处理

| 优化项 | 建议 |
|-------|------|
| 线程管理 | 使用单线程执行器处理数据库操作 |
| 异步操作 | 使用回调或协程处理长时间操作 |
| 批量操作 | 合并多个操作减少线程切换 |
| 超时处理 | 设置合理的操作超时时间 |

### 5.3 存储优化

| 优化项 | 建议 |
|-------|------|
| 存储路径 | 使用应用私有目录 |
| 数据压缩 | 启用向量压缩减少存储 |
| 定期清理 | 定期清理不需要的数据 |
| 备份策略 | 实现数据备份机制 |

---

## 6. 权限配置

### 6.1 Android 权限

| 权限 | 用途 | 是否必需 |
|------|------|----------|
| `WRITE_EXTERNAL_STORAGE` | 外部存储访问 | 可选 |
| `READ_EXTERNAL_STORAGE` | 外部存储读取 | 可选 |
| `ACCESS_FINE_LOCATION` | 位置权限（如果需要） | 可选 |

### 6.2 权限申请代码

```kotlin
// 权限申请
if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
    if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
        requestPermissions(arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE), 1)
    }
}

// 权限回调
override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    if (requestCode == 1 && grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
        // 权限授予，初始化数据库
        initVectorDB()
    }
}
```

---

## 7. 常见问题

### 7.1 编译问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| NDK 版本不匹配 | NDK 版本过低 | 更新到 NDK r25+ |
| 编译错误 | 缺少依赖 | 检查 CMake 配置 |
| 链接错误 | 库文件缺失 | 确保所有源文件都被包含 |

### 7.2 运行时问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 找不到库 | 库未正确加载 | 检查库名和路径 |
| 内存不足 | 向量数据过大 | 减少批处理大小 |
| 权限错误 | 缺少存储权限 | 申请存储权限 |
| 崩溃 | 空指针或内存错误 | 检查参数和错误处理 |

### 7.3 性能问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 插入慢 | 单次插入次数过多 | 使用批处理 |
| 搜索慢 | 索引未优化 | 调整索引参数 |
| 内存占用高 | 向量未压缩 | 启用 PQ 压缩 |
| 响应慢 | 主线程操作 | 移至后台线程 |

---

## 8. 调试技巧

### 8.1 日志调试

```kotlin
// 启用详细日志
System.setProperty("jinwo.vecdb.log.level", "debug")

// 自定义日志处理
class VecDBLogger {
    companion object {
        fun log(message: String) {
            Log.d("VecDB", message)
        }
        
        fun logError(message: String, e: Exception? = null) {
            if (e != null) {
                Log.e("VecDB", message, e)
            } else {
                Log.e("VecDB", message)
            }
        }
    }
}
```

### 8.2 性能分析

| 工具 | 用途 | 使用方法 |
|------|------|----------|
| Android Profiler | 内存和CPU分析 | Android Studio → Profiler |
| Firebase Performance | 性能监控 | 集成 Firebase SDK |
| Traceview | 方法执行时间 | `Debug.startMethodTracing()` |

### 8.3 错误处理

```kotlin
try {
    val id = vecdb.insertVector(vector)
    // 处理成功
} catch (e: Exception) {
    VecDBLogger.logError("Insert failed", e)
    // 处理错误
}
```

---

## 9. 发布注意事项

### 9.1 混淆配置

- 在 `proguard-rules.pro` 中添加：

```
# JinWo VecDB
-keep class com.example.jinwovecdb.JinWoVecDB { *; }
-keepclassmembers class com.example.jinwovecdb.JinWoVecDB { *; }
```

### 9.2 多架构支持

- 在 `build.gradle` 中配置：

```groovy
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### 9.3 应用商店合规

| 合规项 | 要求 |
|--------|------|
| 权限声明 | 只申请必要权限 |
| 数据安全 | 敏感数据加密存储 |
| 隐私政策 | 符合 GDPR/CCPA 要求 |
| 性能要求 | 启动时间 < 5秒 |

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
