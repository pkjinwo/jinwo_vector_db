/**
 * jinwo_vecdb_jni.cpp - JNI bridge for JinWo VecDB
 *
 * Binds Java classes in com.jinwo.vecdb to C API.
 */
#include <jni.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "jw_vecdb.h"

// ============================================================
// JinWoDB JNI
// ============================================================

extern "C"
JNIEXPORT jlong JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeOpen(
    JNIEnv *env, jclass /*clazz*/, jstring jpath, jboolean create)
{
    const char *path = env->GetStringUTFChars(jpath, nullptr);

    // Check for memory mode
    jw_uint32_t flags;
    jw_bool_t is_memory = JW_FALSE;
    if (path == nullptr || path[0] == '\0' || strcmp(path, ":memory:") == 0) {
        flags = JW_VECDB_MEMORY | JW_VECDB_CREATE;
        is_memory = JW_TRUE;
    } else if (create) {
        flags = JW_VECDB_CREATE | JW_VECDB_READWRITE;
    } else {
        flags = JW_VECDB_READWRITE;
    }

    // 不能用 jw_str() 宏，因为 path 是变量指针，sizeof 返回的是指针大小
    jw_str_t path_str;
    if (is_memory) {
        path_str.ptr = NULL;
        path_str.slen = 0;
    } else {
        path_str.ptr = (char *)path;
        path_str.slen = (jw_size_t)strlen(path);
    }

    jw_vecdb_t *db = nullptr;
    jw_status_t rc = jw_vecdb_open(&path_str, flags, &db);
    if (path != nullptr) {
        env->ReleaseStringUTFChars(jpath, path);
    }

    if (rc != JW_SUCCESS || db == nullptr) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, "jinwo_vecdb: failed to open database");
        return 0;
    }
    return reinterpret_cast<jlong>(db);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeClose(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr)
{
    if (ptr != 0) {
        jw_vecdb_close(reinterpret_cast<jw_vecdb_t *>(ptr));
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeGetVersion(
    JNIEnv *env, jclass /*clazz*/)
{
    jw_str_t ver = jw_vecdb_version();
    return env->NewStringUTF(ver.ptr);
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeListCollections(
    JNIEnv *env, jclass /*clazz*/, jlong dbPtr)
{
    auto *db = reinterpret_cast<jw_vecdb_t *>(dbPtr);
    if (!db) return nullptr;

    // First call: get count
    jw_size_t count = jw_vecdb_list_collections(db, nullptr, 0);
    if (count == 0) return nullptr;

    // Allocate names array
    jw_str_t *names = (jw_str_t *)malloc(count * sizeof(jw_str_t));
    if (!names) return nullptr;

    // Second call: get names
    jw_size_t actual = jw_vecdb_list_collections(db, names, count);

    jclass strClass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray((jsize)actual, strClass, nullptr);
    for (jw_size_t i = 0; i < actual; i++) {
        jstring s = env->NewStringUTF(names[i].ptr);
        env->SetObjectArrayElement(arr, (jsize)i, s);
        env->DeleteLocalRef(s);
    }
    free(names);
    return arr;
}

// ============================================================
// Collection JNI
// ============================================================

extern "C"
JNIEXPORT jlong JNICALL
Java_com_jinwo_vecdb_Collection_nativeCreateCollection(
    JNIEnv *env, jclass /*clazz*/, jlong dbPtr, jstring jname, jint dimension)
{
    auto *db = reinterpret_cast<jw_vecdb_t *>(dbPtr);
    const char *name = env->GetStringUTFChars(jname, nullptr);
    if (name == nullptr) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, "jinwo_vecdb: failed to get collection name");
        return 0;
    }
    jw_str_t name_str = {(char *)name, (jw_size_t)strlen(name)};

    jw_collection_t *coll = nullptr;
    jw_status_t rc = jw_vecdb_create_collection(db, &name_str, (jw_dim_t)dimension, &coll);
    env->ReleaseStringUTFChars(jname, name);

    if (rc != JW_SUCCESS || coll == nullptr) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, "jinwo_vecdb: failed to create collection");
        return 0;
    }
    return reinterpret_cast<jlong>(coll);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jinwo_vecdb_Collection_nativeCloseCollection(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr)
{
    if (ptr != 0) {
        jw_collection_destroy(reinterpret_cast<jw_collection_t *>(ptr));
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_jinwo_vecdb_Collection_nativeInsert(
    JNIEnv *env, jclass /*clazz*/, jlong ptr, jfloatArray jvec, jint dim)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    if (coll == nullptr) return -1;
    jfloat *vec = env->GetFloatArrayElements(jvec, nullptr);
    if (vec == nullptr) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, "jinwo_vecdb: failed to get float array");
        return -1;
    }

    jw_vid_t vid = 0;
    jw_status_t rc = jw_collection_insert(coll, vec, &vid);
    env->ReleaseFloatArrayElements(jvec, vec, JNI_ABORT);

    if (rc != JW_SUCCESS) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        char msg[128];
        snprintf(msg, sizeof(msg), "jinwo_vecdb: insert failed (code=%d)", (int)rc);
        env->ThrowNew(exClass, msg);
        return -1;
    }

    return (jlong)vid;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jinwo_vecdb_Collection_nativeDelete(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr, jlong vid)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    jw_status_t rc = jw_collection_delete(coll, (jw_vid_t)vid);
    return (jint)rc;
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_jinwo_vecdb_Collection_nativeSearch(
    JNIEnv *env, jclass /*clazz*/, jlong ptr, jfloatArray jquery, jint dim, jint k)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    if (coll == nullptr) return nullptr;
    jfloat *query = env->GetFloatArrayElements(jquery, nullptr);
    if (query == nullptr) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, "jinwo_vecdb: failed to get float array");
        return nullptr;
    }

    // Set up search options
    jw_search_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.k = (jw_size_t)k;
    opts.include_vectors = JW_FALSE;
    opts.include_meta = JW_FALSE;

    // Allocate results
    jw_search_result_ex_t *results =
        (jw_search_result_ex_t *)malloc(k * sizeof(jw_search_result_ex_t));
    if (!results) {
        env->ReleaseFloatArrayElements(jquery, query, JNI_ABORT);
        return nullptr;
    }

    jw_size_t count = jw_collection_search(coll, query, &opts, results);

    // Pack [vid, score-as-float-bits, vid, score-as-float-bits, ...]
    jlongArray arr = env->NewLongArray((jsize)(count * 2));
    jlong *buf = (jlong *)malloc(count * 2 * sizeof(jlong));
    if (buf) {
        for (jw_size_t i = 0; i < count; i++) {
            buf[i * 2]     = (jlong)results[i].vid;
            // Store float score as raw bits in jlong
            float s = results[i].score;
            memcpy(&buf[i * 2 + 1], &s, sizeof(float));
        }
        env->SetLongArrayRegion(arr, 0, (jsize)(count * 2), buf);
        free(buf);
    }

    free(results);
    env->ReleaseFloatArrayElements(jquery, query, JNI_ABORT);
    return arr;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jinwo_vecdb_Collection_nativeBuildIndex(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    jw_status_t rc = jw_collection_build_index(coll);
    return (jint)rc;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jinwo_vecdb_Collection_nativeGetDimension(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    return (jint)coll->dim;
}
