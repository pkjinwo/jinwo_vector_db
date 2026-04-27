#include <jni.h>
#include <string>
#include "jw_vecdb.h"

extern "C" {

JNIEXPORT jlong JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeOpenDatabase(
        JNIEnv *env, jobject obj, jstring path, jboolean create) {
    const char *pathStr = env->GetStringUTFChars(path, NULL);
    if (!pathStr) return 0;

    jw_vecdb_t *db = NULL;
    int result = jw_vecdb_open(&db, pathStr, create);

    env->ReleaseStringUTFChars(path, pathStr);
    return (jlong)db;
}

JNIEXPORT void JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeCloseDatabase(
        JNIEnv *env, jobject obj, jlong dbPtr) {
    if (dbPtr != 0) {
        jw_vecdb_close((jw_vecdb_t *)dbPtr);
    }
}

JNIEXPORT jstring JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeGetVersion(
        JNIEnv *env, jobject obj) {
    const char *version = jw_version();
    return env->NewStringUTF(version);
}

JNIEXPORT jlong JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeCreateCollection(
        JNIEnv *env, jobject obj, jlong dbPtr, jstring name, jint dimension) {
    if (dbPtr == 0) return 0;

    const char *nameStr = env->GetStringUTFChars(name, NULL);
    if (!nameStr) return 0;

    jw_collection_t *collection = NULL;
    int result = jw_collection_create(&collection, (jw_vecdb_t *)dbPtr, nameStr, dimension);

    env->ReleaseStringUTFChars(name, nameStr);
    return (jlong)collection;
}

JNIEXPORT void JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeCloseCollection(
        JNIEnv *env, jobject obj, jlong collectionPtr) {
    if (collectionPtr != 0) {
        jw_collection_close((jw_collection_t *)collectionPtr);
    }
}

JNIEXPORT jlong JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeInsertVector(
        JNIEnv *env, jobject obj, jlong collectionPtr, jfloatArray vector) {
    if (collectionPtr == 0) return 0;

    jfloat *vectorData = env->GetFloatArrayElements(vector, NULL);
    if (!vectorData) return 0;

    uint64_t id;
    int result = jw_collection_insert((jw_collection_t *)collectionPtr, vectorData, &id);

    env->ReleaseFloatArrayElements(vector, vectorData, 0);
    return result == JW_OK ? (jlong)id : 0;
}

JNIEXPORT jlongArray JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeSearchVectorIds(
        JNIEnv *env, jobject obj, jlong collectionPtr, jfloatArray query, jint k) {
    if (collectionPtr == 0) return NULL;

    jfloat *queryData = env->GetFloatArrayElements(query, NULL);
    if (!queryData) return NULL;

    jw_search_result_t *results = NULL;
    size_t result_count;
    int result = jw_collection_search((jw_collection_t *)collectionPtr, queryData, k, &results, &result_count);

    env->ReleaseFloatArrayElements(query, queryData, 0);

    if (result != JW_OK || result_count == 0) {
        if (results) free(results);
        return NULL;
    }

    jlongArray resultArray = env->NewLongArray(result_count);
    jlong *resultIds = new jlong[result_count];
    for (size_t i = 0; i < result_count; i++) {
        resultIds[i] = (jlong)results[i].id;
    }

    env->SetLongArrayRegion(resultArray, 0, result_count, resultIds);
    delete[] resultIds;
    free(results);

    return resultArray;
}

JNIEXPORT jfloatArray JNICALL Java_com_jinwo_vecdb_demo_MainActivity_nativeGetSearchDistances(
        JNIEnv *env, jobject obj, jlong collectionPtr, jfloatArray query, jint k) {
    if (collectionPtr == 0) return NULL;

    jfloat *queryData = env->GetFloatArrayElements(query, NULL);
    if (!queryData) return NULL;

    jw_search_result_t *results = NULL;
    size_t result_count;
    int result = jw_collection_search((jw_collection_t *)collectionPtr, queryData, k, &results, &result_count);

    env->ReleaseFloatArrayElements(query, queryData, 0);

    if (result != JW_OK || result_count == 0) {
        if (results) free(results);
        return NULL;
    }

    jfloatArray resultArray = env->NewFloatArray(result_count);
    jfloat *resultDistances = new jfloat[result_count];
    for (size_t i = 0; i < result_count; i++) {
        resultDistances[i] = results[i].distance;
    }

    env->SetFloatArrayRegion(resultArray, 0, result_count, resultDistances);
    delete[] resultDistances;
    free(results);

    return resultArray;
}

}
