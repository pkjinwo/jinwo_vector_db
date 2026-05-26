/**
 * jinwo_vecdb_jni.cpp - JNI bridge for JinWo VecDB
 *
 * Binds Java classes in com.jinwo.vecdb to C API.
 */
#include <jni.h>
#include <cstring>
#include "jw_vecdb.h"
#include "jw_collection.h"

// ============================================================
// JinWoDB JNI
// ============================================================

extern "C"
JNIEXPORT jlong JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeOpen(
    JNIEnv *env, jclass /*clazz*/, jstring jpath, jboolean create)
{
    const char *path = env->GetStringUTFChars(jpath, nullptr);
    jw_vecdb_t *db = jw_vecdb_open(path, create ? JW_OPEN_CREATE : JW_OPEN_READONLY);
    env->ReleaseStringUTFChars(jpath, path);
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
    return env->NewStringUTF(jw_version());
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_jinwo_vecdb_JinWoDB_nativeListCollections(
    JNIEnv *env, jclass /*clazz*/, jlong dbPtr)
{
    auto *db = reinterpret_cast<jw_vecdb_t *>(dbPtr);
    jw_strlist_t *list = jw_collection_list(db);
    if (!list) return nullptr;

    jclass strClass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(list->count, strClass, nullptr);
    for (uint32_t i = 0; i < list->count; i++) {
        jstring s = env->NewStringUTF(list->items[i]);
        env->SetObjectArrayElement(arr, i, s);
        env->DeleteLocalRef(s);
    }
    jw_strlist_free(list);
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
    jw_collection_t *coll = jw_collection_create(db, name, dimension);
    env->ReleaseStringUTFChars(jname, name);
    return reinterpret_cast<jlong>(coll);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_jinwo_vecdb_Collection_nativeCloseCollection(
    JNIEnv * /*env*/, jclass /*clazz*/, jlong ptr)
{
    if (ptr != 0) {
        jw_collection_close(reinterpret_cast<jw_collection_t *>(ptr));
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_jinwo_vecdb_Collection_nativeInsert(
    JNIEnv *env, jclass /*clazz*/, jlong ptr, jfloatArray jvec, jint dim)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    jfloat *vec = env->GetFloatArrayElements(jvec, nullptr);
    jw_status_t rc = jw_collection_insert(coll, vec, (uint32_t)dim);
    env->ReleaseFloatArrayElements(jvec, vec, JNI_ABORT);

    if (rc != JW_SUCCESS) {
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        char msg[128];
        snprintf(msg, sizeof(msg), "jinwo_vecdb: insert failed (code=%d)", (int)rc);
        env->ThrowNew(exClass, msg);
        return -1;
    }

    // Return the last inserted vector ID
    // (NB: C API jw_collection_insert doesn't return vid directly;
    //  we approximate with collection vector count)
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jinwo_vecdb_Collection_nativeDelete(
    JNIEnv *env, jclass /*clazz*/, jlong ptr, jlong vid)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    jw_status_t rc = jw_collection_delete(coll, (uint64_t)vid);
    return (jint)rc;
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_jinwo_vecdb_Collection_nativeSearch(
    JNIEnv *env, jclass /*clazz*/, jlong ptr, jfloatArray jquery, jint dim, jint k)
{
    auto *coll = reinterpret_cast<jw_collection_t *>(ptr);
    jfloat *query = env->GetFloatArrayElements(jquery, nullptr);

    jw_search_result_t *results = (jw_search_result_t *)malloc(k * sizeof(jw_search_result_t));
    int count = jw_collection_search(coll, query, (uint32_t)dim, k, results);

    // Pack [id, distance-float-as-int, id, distance-float-as-int, ...]
    jlongArray arr = env->NewLongArray(count * 2);

    jlong *buf = (jlong *)malloc(count * 2 * sizeof(jlong));
    for (int i = 0; i < count; i++) {
        buf[i * 2]     = (jlong)results[i].id;
        // Store float distance as raw bits in jlong
        float dist = results[i].distance;
        memcpy(&buf[i * 2 + 1], &dist, sizeof(float));
    }
    env->SetLongArrayRegion(arr, 0, count * 2, buf);

    free(buf);
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
    return (jint)jw_collection_encoding_dim(coll);
}
