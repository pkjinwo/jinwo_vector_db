# JinWo VecDB Android Platform Integration Guide

**Version**: 1.0.0
**Last Updated**: 2026-04-26
**Platform**: Android

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [CMake NDK Configuration](#cmake-ndk-configuration)
4. [Android Studio Setup](#android-studio-setup)
5. [Gradle Integration](#gradle-integration)
6. [Java/Kotlin Usage](#javakotlin-usage)
7. [Native Library Loading](#native-library-loading)
8. [Memory Management](#memory-management)
9. [Performance Optimization](#performance-optimization)
10. [ProGuard Configuration](#proguard-configuration)
11. [Troubleshooting](#troubleshooting)

---

## Overview

JinWo VecDB provides native Android support through a C library that integrates with Android NDK.

### Supported Android Versions

- **Minimum**: Android API 21 (Lollipop 5.0)
- **Recommended**: Android API 24 (Nougat 7.0)+
- **Architectures**: armeabi-v7a, arm64-v8a, x86, x86_64

---

## Prerequisites

### Required Tools

1. **Android Studio** Arctic Fox or later
2. **Android NDK** version 21 or later
3. **CMake** 3.18 or later (bundled with NDK)
4. **Gradle** 7.0+

### SDK Configuration

```groovy
// build.gradle (app)
android {
    compileSdkVersion 34
    buildToolsVersion "34.0.0"

    defaultConfig {
        minSdkVersion 21
        targetSdkVersion 34

        ndk {
            abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
        }

        externalNativeBuild {
            cmake {
                cppFlags "-std=c99"
                arguments "-DJW_ANDROID=1"
            }
        }
    }

    buildTypes {
        release {
            ndk {
                abiFilters 'armeabi-v7a', 'arm64-v8a'
            }
        }
    }
}
```

---

## CMake NDK Configuration

### CMakeLists.txt

Create `CMakeLists.txt` in your app's jni directory:

```cmake
cmake_minimum_required(VERSION 3.18)

project("jinwo_vecdb_app")

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add JinWo VecDB
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/jinwo_vector_db jw_vecdb_build)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/jinwo_vector_db/include
)

# Create shared library
add_library(jinwo_vecdb_app SHARED
    native-lib.cpp
)

# Find libraries
find_library(log-lib log)
find_library(jni-lib jni)

# Link libraries
target_link_libraries(jinwo_vecdb_app
    jw_vecdb
    ${log-lib}
    ${jni-lib}
)
```

### Application.mk

Create `Application.mk` in `app/src/main/jni`:

```makefile
# Supported ABIs
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

# C++ standard
APP_CPPFLAGS := -std=c++17 -fexceptions

# Optional: restrict to specific NDK API level
APP_PLATFORM := android-21
```

### Build Script

Create `build_jw_vecdb.sh`:

```bash
#!/bin/bash
set -e

NDK_DIR=$ANDROID_NDK_HOME
TOOLCHAIN=$NDK_DIR/build/cmake/android.toolchain.cmake

mkdir -p build
cd build

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
    -DANDROID_ABI="arm64-v8a" \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_SHARED=OFF \
    -DJW_BUILD_TESTS=OFF

cmake --build . -- -j$(nproc)
```

---

## Android Studio Setup

### Step-by-Step Instructions

1. **Create New Project** with C++ support

2. **Add C++ Files**:
   - Right-click `app/src/main` → New → Directory → Name: `jni`
   - Add `native-lib.cpp` and `CMakeLists.txt`

3. **Link CMake**:
   ```groovy
   android {
       externalNativeBuild {
           cmake {
               path "src/main/jni/CMakeLists.txt"
               version "3.18.1"
           }
       }
   }
   ```

4. **Sync and Build**

---

## Gradle Integration

### Using Pre-built Library

If you have pre-built JinWo VecDB libraries:

```groovy
android {
    sourceSets {
        main {
            jniLibs.srcDirs = ['libs']
        }
    }
}

dependencies {
    implementation fileTree(dir: 'libs', include: ['*.aar', '*.jar'])
}
```

### Full build.gradle Example

```groovy
plugins {
    id 'com.android.library'
}

android {
    namespace 'com.example.jinwovecdb'
    compileSdk 34

    defaultConfig {
        minSdk 21
        targetSdk 34

        ndk {
            abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
            cFlags '-DJW_ANDROID=1', '-DWITH_SIMD'
        }

        externalNativeBuild {
            cmake {
                arguments '-DJW_ANDROID=1',
                          '-DCMAKE_BUILD_TYPE=Release'
                cppFlags '-std=c++17'
            }
        }

        cmake {
            path 'src/main/jni/CMakeLists.txt'
            version '3.18.1'
        }
    }

    buildTypes {
        release {
            minifyEnabled true
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
        }
        debug {
            minifyEnabled false
        }
    }

    compileOptions {
        sourceCompatibility JavaVersion.VERSION_17
        targetCompatibility JavaVersion.VERSION_17
    }
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
}
```

---

## Java/Kotlin Usage

### Native Interface

Create `native-lib.cpp`:

```cpp
#include <jni.h>
#include <string>
#include "jw_vecdb.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_example_jinwovecdb_VecDB_init(JNIEnv *env, jobject obj) {
    jw_init();
    return 0;
}

JNIEXPORT jlong JNICALL
Java_com_example_jinwovecdb_VecDB_open(JNIEnv *env, jobject obj,
                                        jstring jpath, jint flags) {
    jw_vecdb_t *db = NULL;
    jw_status_t status;

    if (jpath == NULL) {
        status = jw_vecdb_open(NULL, flags | JW_VECDB_MEMORY, &db);
    } else {
        const char *path = env->GetStringUTFChars(jpath, 0);
        status = jw_vecdb_open(jw_str_new(path), flags, &db);
        env->ReleaseStringUTFChars(jpath, path);
    }

    return (jlong)(intptr_t)db;
}

JNIEXPORT void JNICALL
Java_com_example_jinwovecdb_VecDB_close(JNIEnv *env, jobject obj, jlong handle) {
    if (handle != 0) {
        jw_vecdb_close((jw_vecdb_t *)(intptr_t)handle);
    }
}

JNIEXPORT jlong JNICALL
Java_com_example_jinwovecdb_VecDB_createCollection(JNIEnv *env, jobject obj,
                                                   jlong dbHandle, jstring jname,
                                                   jint dim) {
    if (dbHandle == 0) return 0;

    jw_collection_t *coll = NULL;
    const char *name = env->GetStringUTFChars(jname, 0);

    jw_status_t status = jw_vecdb_create_collection(
        (jw_vecdb_t *)(intptr_t)dbHandle,
        jw_str_new(name),
        dim,
        &coll
    );

    env->ReleaseStringUTFChars(jname, name);
    return (jlong)(intptr_t)coll;
}

JNIEXPORT jint JNICALL
Java_com_example_jinwovecdb_VecDB_insert(JNIEnv *env, jobject obj,
                                          jlong collHandle, jfloatArray jvec) {
    if (collHandle == 0) return JW_INVALID_PARAM;

    jsize len = env->GetArrayLength(jvec);
    jfloat *vec = env->GetFloatArrayElements(jvec, 0);

    jw_vid_t vid = 0;
    jw_status_t status = jw_collection_insert(
        (jw_collection_t *)(intptr_t)collHandle,
        vec,
        &vid
    );

    env->ReleaseFloatArrayElements(jvec, vec, 0);

    return (jint)status;
}

JNIEXPORT jintArray JNICALL
Java_com_example_jinwovecdb_VecDB_search(JNIEnv *env, jobject obj,
                                           jlong collHandle, jfloatArray jquery,
                                           jint k) {
    if (collHandle == 0) return NULL;

    jsize dim = env->GetArrayLength(jquery);
    jfloat *query = env->GetFloatArrayElements(jquery, 0);

    jw_search_result_t *results = new jw_search_result_t[k];
    jsize count = jw_collection_search(
        (jw_collection_t *)(intptr_t)collHandle,
        query,
        k,
        results
    );

    env->ReleaseFloatArrayElements(jquery, query, 0);

    jintArray resultArray = env->NewIntArray(count * 2);
    jint *resultData = new jint[count * 2];
    for (int i = 0; i < count; i++) {
        resultData[i * 2] = (jint)results[i].vid;
        resultData[i * 2 + 1] = (jint)(results[i].score * 10000);
    }
    env->SetIntArrayRegion(resultArray, 0, count * 2, resultData);

    delete[] results;
    delete[] resultData;

    return resultArray;
}

JNIEXPORT void JNICALL
Java_com_example_jinwovecdb_VecDB_cleanup(JNIEnv *env, jobject obj) {
    jw_cleanup();
}

}
```

### Java Wrapper Class

```java
package com.example.jinwovecdb;

public class VecDB implements AutoCloseable {
    static {
        System.loadLibrary("jw_vecdb");
        System.loadLibrary("native-lib");
    }

    private long dbHandle = 0;
    private boolean isOpen = false;

    public VecDB() {
        init();
    }

    private native void init();

    public void open(String path, boolean create) {
        int flags = 0;
        if (create) flags |= 0x04; // JW_VECDB_CREATE
        flags |= 0x02; // JW_VECDB_READWRITE

        dbHandle = openNative(path, flags);
        isOpen = (dbHandle != 0);
        if (!isOpen) {
            throw new RuntimeException("Failed to open database");
        }
    }

    public void openMemory() {
        dbHandle = openNative(null, 0x10 | 0x04); // JW_VECDB_MEMORY | JW_VECDB_CREATE
        isOpen = (dbHandle != 0);
        if (!isOpen) {
            throw new RuntimeException("Failed to open in-memory database");
        }
    }

    private native long openNative(String path, int flags);

    @Override
    public void close() {
        if (dbHandle != 0) {
            closeNative(dbHandle);
            dbHandle = 0;
            isOpen = false;
        }
    }

    private native void closeNative(long handle);

    public Collection createCollection(String name, int dimension) {
        long collHandle = createCollectionNative(dbHandle, name, dimension);
        if (collHandle == 0) {
            throw new RuntimeException("Failed to create collection");
        }
        return new Collection(collHandle);
    }

    private native long createCollectionNative(long dbHandle, String name, int dim);

    public boolean isOpen() {
        return isOpen;
    }

    public native void cleanup();

    @Override
    protected void finalize() throws Throwable {
        close();
        cleanup();
        super.finalize();
    }

    public static class Collection implements AutoCloseable {
        private final long handle;

        Collection(long handle) {
            this.handle = handle;
        }

        public int insert(float[] vector) {
            return insertNative(handle, vector);
        }

        private native int insertNative(long handle, float[] vector);

        public SearchResult[] search(float[] query, int k) {
            int[] results = searchNative(handle, query, k);
            if (results == null || results.length == 0) {
                return new SearchResult[0];
            }

            SearchResult[] searchResults = new SearchResult[results.length / 2];
            for (int i = 0; i < searchResults.length; i++) {
                searchResults[i] = new SearchResult(
                    results[i * 2],
                    results[i * 2 + 1] / 10000.0f
                );
            }
            return searchResults;
        }

        private native int[] searchNative(long handle, float[] query, int k);

        @Override
        public void close() {
            // Collection is managed by database
        }
    }

    public static class SearchResult {
        public final long id;
        public final float score;

        public SearchResult(long id, float score) {
            this.id = id;
            this.score = score;
        }
    }
}
```

### Kotlin Usage Example

```kotlin
package com.example.jinwovecdb

fun main() {
    VecDB().use { db ->
        // Open in-memory database
        db.openMemory()
        println("Database opened: ${db.isOpen}")

        // Create collection
        val coll = db.createCollection("documents", 128)
        coll.use {
            // Insert vectors
            val embedding = FloatArray(128) { Math.random().toFloat() }
            val status = it.insert(embedding)
            println("Insert status: $status")

            // Search
            val results = it.search(embedding, 5)
            println("Found ${results.size} results:")
            results.forEach { result ->
                println("  ID: ${result.id}, Score: ${result.score}")
            }
        }
    }
}
```

---

## Native Library Loading

### Static Initialization

```java
public class VecDBLoader {
    private static boolean initialized = false;

    public static synchronized void init(Context context) {
        if (initialized) return;

        try {
            // Load dependencies first
            System.loadLibrary("jw_vecdb");
            // Then load your app's native library
            System.loadLibrary("native-lib");
            initialized = true;
        } catch (UnsatisfiedLinkError e) {
            throw new RuntimeException("Failed to load JinWo VecDB native library", e);
        }
    }
}
```

### Application Class

```java
public class MyApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        VecDBLoader.init(this);
    }
}
```

---

## Memory Management

### Android-Specific Considerations

1. **Process Memory Limits**: Android enforces per-process memory limits
2. **GC Pressure**: Minimize allocations in native code
3. **Low Memory Killer**: Handle `onTrimMemory` callbacks

### Best Practices

```java
public class MainActivity extends AppCompatActivity {

    private VecDB db;

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        if (level >= TRIM_MEMORY_MODERATE) {
            // Release non-essential resources
            if (db != null) {
                // Sync and compact database
            }
        }
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        // Emergency memory release
    }
}
```

### Arena Size Configuration

```java
// For devices with limited memory (≤2GB RAM)
jw_vecdb_config_t config = JW_VECDB_CONFIG_DEFAULT;
config.arena_size = 16 * 1024 * 1024; // 16MB
config.cache_size = 32 * 1024 * 1024; // 32MB

// For high-end devices (>6GB RAM)
config.arena_size = 128 * 1024 * 1024; // 128MB
config.cache_size = 256 * 1024 * 1024; // 256MB
```

---

## Performance Optimization

### SIMD on Android

JinWo VecDB automatically detects and uses **NEON** SIMD instructions on ARM devices.

### Optimization Tips

1. **Batch Insert**: Use batch APIs for bulk operations
2. **Background Processing**: Use `AsyncTask` or `Coroutine` for heavy operations
3. **Quantization**: Use PQ/SQ for large datasets
4. **Memory Pool**: Pre-allocate arena for predictable workload

```kotlin
// Background vector insertion with Kotlin Coroutines
suspend fun insertVectors(vectors: List<FloatArray>) = withContext(Dispatchers.Default) {
    for (vec in vectors) {
        collection.insert(vec)
    }
}
```

### Build for Performance

```groovy
android {
    buildTypes {
        release {
            minifyEnabled true
            shrinkResources true
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'

            cmake {
                arguments '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
                          '-DJW_ENABLE_SIMD=ON'
            }
        }
    }
}
```

---

## ProGuard Configuration

Add to `proguard-rules.pro`:

```pro
# JinWo VecDB
-keep class com.example.jinwovecdb.** { *; }

# JNI
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep native methods
-keepclassmembers class * {
    @android.webkit.JavascriptInterface <methods>;
}

# If you use reflection for native library loading
-keepclassmembers class * {
    static {
        System.loadLibrary(...);
    }
}
```

---

## Troubleshooting

### Common Issues

#### 1. "dlopen failed" errors

**Solution**: Ensure NDK libraries are properly packaged:

```groovy
android {
    packagingOptions {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}
```

#### 2. "ABI incompatibility" errors

**Solution**: Ensure you're building for the correct architecture:

```bash
# Check available ABIs
adb shell getprop ro.product.cpu.abi
```

#### 3. Memory overflow on large datasets

**Solution**: Use quantization and streaming:

```java
// Enable quantization for memory efficiency
jw_collection_config_t config = JW_COLLECTION_CONFIG_DEFAULT;
config.quant_type = JW_QUANT_PQ;
```

#### 4. Slow build times

**Solution**: Use pre-built libraries for release builds:

```groovy
android {
    sourceSets {
        release {
            jniLibs.srcDirs = ['libs/release']
        }
    }
}
```

### Debugging

Enable verbose logging:

```java
jw_vecdb_set_log_callback((level, msg, userData) -> {
    Log.v("JinWoVecDB", msg);
}, null);
```

Check native library loading:

```bash
adb logcat | grep -E "(JinWoVecDB|dlopen|UnsatisfiedLinkError)"
```

### Performance Profiling

Use **Android Studio Profiler** to monitor:
- Native memory allocation
- CPU usage of native threads
- SIMD acceleration effectiveness

---

## Next Steps

1. [API Reference Documentation](./api_reference.md)
2. [iOS Integration Guide](./ios_integration.md)
3. [Windows Integration Guide](./windows_integration.md)

---

**Document Version**: 1.0.0
**Last Updated**: 2026-04-26
