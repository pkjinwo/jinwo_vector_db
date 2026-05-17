/*
 * py_jinwo.c - Python ctypes 绑定层
 * 
 * 将 JinWo VecDB C API 包装为 ctypes 可调用的函数
 */

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 包含原始 JinWo 头文件 */
#include "jw_types.h"
#include "jw_vecdb.h"
#include "jw_collection.h"

/*============================================================================
 * 辅助函数
 *============================================================================*/

/* 创建 jw_str_t 从 Python 字符串 */
static jw_str_t py_str_to_jw_str(PyObject *py_str) {
    jw_str_t str;
    if (py_str == NULL || py_str == Py_None) {
        str.ptr = "";
        str.slen = 0;
    } else {
        const char *s = PyUnicode_AsUTF8(py_str);
        str.ptr = s ? s : "";
        str.slen = s ? strlen(s) : 0;
    }
    return str;
}

/* 将 jw_str_t 转换为 Python 字符串 */
static PyObject *jw_str_to_py_str(jw_str_t str) {
    return PyUnicode_FromStringAndSize(str.ptr, str.slen);
}

/*============================================================================
 * 数据库操作
 *============================================================================*/

/* jw_vecdb_open(path, flags) -> db_handle */
static PyObject *py_jw_vecdb_open(PyObject *self, PyObject *args) {
    const char *path;
    int flags;
    jw_vecdb_t *db = NULL;
    jw_status_t status;
    jw_str_t path_str;

    if (!PyArg_ParseTuple(args, "si", &path, &flags)) {
        return NULL;
    }

    path_str.ptr = path;
    path_str.slen = path ? strlen(path) : 0;

    status = jw_vecdb_open(&path_str, (jw_uint32_t)flags, &db);
    if (status != JW_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, jw_vecdb_strerror(status));
        return NULL;
    }

    return PyLong_FromVoidPtr(db);
}

/* jw_vecdb_close(db_handle) -> None */
static PyObject *py_jw_vecdb_close(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    long long db_ptr;

    if (!PyArg_ParseTuple(args, "L", &db_ptr)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    jw_vecdb_close(db);

    Py_RETURN_NONE;
}

/* jw_vecdb_sync(db_handle) -> None */
static PyObject *py_jw_vecdb_sync(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    long long db_ptr;

    if (!PyArg_ParseTuple(args, "L", &db_ptr)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    jw_vecdb_sync(db);

    Py_RETURN_NONE;
}

/*============================================================================
 * Collection 操作
 *============================================================================*/

/* jw_vecdb_create_collection(db_handle, name, dim) -> coll_handle */
static PyObject *py_jw_vecdb_create_collection(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *name;
    int dim;
    jw_collection_t *coll = NULL;
    jw_status_t status;
    jw_str_t name_str;
    long long db_ptr;

    if (!PyArg_ParseTuple(args, "Lsi", &db_ptr, &name, &dim)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    name_str.ptr = name;
    name_str.slen = strlen(name);

    status = jw_vecdb_create_collection(db, &name_str, (jw_dim_t)dim, &coll);
    if (status != JW_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, jw_vecdb_strerror(status));
        return NULL;
    }

    return PyLong_FromVoidPtr(coll);
}

/* jw_vecdb_get_collection(db_handle, name) -> coll_handle or None */
static PyObject *py_jw_vecdb_get_collection(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *name;
    jw_collection_t *coll = NULL;
    jw_str_t name_str;
    long long db_ptr;

    if (!PyArg_ParseTuple(args, "Ls", &db_ptr, &name)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    name_str.ptr = name;
    name_str.slen = strlen(name);

    coll = jw_vecdb_get_collection(db, &name_str);
    if (coll == NULL) {
        Py_RETURN_NONE;
    }

    return PyLong_FromVoidPtr(coll);
}

/* jw_vecdb_drop_collection(db_handle, name) -> bool */
static PyObject *py_jw_vecdb_drop_collection(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *name;
    jw_status_t status;
    jw_str_t name_str;
    long long db_ptr;

    if (!PyArg_ParseTuple(args, "Ls", &db_ptr, &name)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    name_str.ptr = name;
    name_str.slen = strlen(name);

    status = jw_vecdb_drop_collection(db, &name_str);
    if (status != JW_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, jw_vecdb_strerror(status));
        return NULL;
    }

    Py_RETURN_TRUE;
}

/* jw_vecdb_list_collections(db_handle) -> list of names */
static PyObject *py_jw_vecdb_list_collections(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    jw_str_t names[100];
    jw_size_t count;
    long long db_ptr;
    int i;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "L", &db_ptr)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    count = jw_vecdb_list_collections(db, names, 100);

    list = PyList_New(count);
    for (i = 0; i < (int)count; i++) {
        PyList_SET_ITEM(list, i, jw_str_to_py_str(names[i]));
    }

    return list;
}

/*============================================================================
 * 向量操作
 *============================================================================*/

/* jw_vecdb_insert(db_handle, coll_name, vector, dim) -> vid */
static PyObject *py_jw_vecdb_insert(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *coll_name;
    float *vec;
    int dim;
    jw_vid_t vid;
    jw_status_t status;
    jw_str_t name_str;
    long long db_ptr;
    PyObject *vec_obj;

    if (!PyArg_ParseTuple(args, "LsO!i", &db_ptr, &coll_name, &PyList_Type, &vec_obj, &dim)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    name_str.ptr = coll_name;
    name_str.slen = strlen(coll_name);

    vec = (float *)malloc(sizeof(float) * dim);
    for (int i = 0; i < dim; i++) {
        vec[i] = (float)PyFloat_AsDouble(PyList_GET_ITEM(vec_obj, i));
    }

    status = jw_vecdb_insert(db, &name_str, vec, (jw_dim_t)dim, &vid);
    free(vec);

    if (status != JW_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, jw_vecdb_strerror(status));
        return NULL;
    }

    return PyLong_FromUnsignedLongLong(vid);
}

/* jw_vecdb_insert_batch(db_handle, coll_name, vectors, dim) -> list of vids */
static PyObject *py_jw_vecdb_insert_batch(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *coll_name;
    PyObject *vec_list;
    int dim;
    int count;
    float *vectors;
    jw_vid_t *vids;
    jw_status_t status;
    jw_str_t name_str;
    long long db_ptr;
    int i, j;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "LsO!i", &db_ptr, &coll_name, &PyList_Type, &vec_list, &dim)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_ptr;
    name_str.ptr = coll_name;
    name_str.slen = strlen(coll_name);

    count = (int)PyList_Size(vec_list);
    vectors = (float *)malloc(sizeof(float) * count * dim);
    vids = (jw_vid_t *)malloc(sizeof(jw_vid_t) * count);

    for (i = 0; i < count; i++) {
        PyObject *vec_obj = PyList_GET_ITEM(vec_list, i);
        for (j = 0; j < dim; j++) {
            vectors[i * dim + j] = (float)PyFloat_AsDouble(PyList_GET_ITEM(vec_obj, j));
        }
    }

    status = jw_vecdb_insert_batch(db, &name_str, vectors, (jw_dim_t)dim, (jw_size_t)count, vids);
    free(vectors);

    if (status != JW_SUCCESS) {
        free(vids);
        PyErr_SetString(PyExc_RuntimeError, jw_vecdb_strerror(status));
        return NULL;
    }

    result = PyList_New(count);
    for (i = 0; i < count; i++) {
        PyList_SET_ITEM(result, i, PyLong_FromUnsignedLongLong(vids[i]));
    }
    free(vids);

    return result;
}

/* jw_vecdb_search(db_handle, coll_name, query, dim, k) -> list of (vid, distance) */
static PyObject *py_jw_vecdb_search(PyObject *self, PyObject *args) {
    jw_vecdb_t *db;
    const char *coll_name;
    PyObject *query_obj;
    int dim;
    int k;
    float *query_vec;
    jw_search_result_t *results;
    jw_size_t result_count;
    jw_str_t name_str;
    long long db_ptr;
    int i;
    PyObject *result;
    long long db_handle;

    if (!PyArg_ParseTuple(args, "LsO!ii", &db_handle, &coll_name, &PyList_Type, &query_obj, &dim, &k)) {
        return NULL;
    }

    db = (jw_vecdb_t *)(intptr_t)db_handle;
    name_str.ptr = coll_name;
    name_str.slen = strlen(coll_name);

    query_vec = (float *)malloc(sizeof(float) * dim);
    for (int i = 0; i < dim; i++) {
        query_vec[i] = (float)PyFloat_AsDouble(PyList_GET_ITEM(query_obj, i));
    }

    results = (jw_search_result_t *)malloc(sizeof(jw_search_result_t) * k);
    result_count = jw_vecdb_search(db, &name_str, query_vec, (jw_dim_t)dim, (jw_size_t)k, results);

    free(query_vec);

    result = PyList_New(result_count);
    for (i = 0; i < (int)result_count; i++) {
        PyObject *item = PyTuple_Pack(2,
            PyLong_FromUnsignedLongLong(results[i].id),
            PyFloat_FromDouble(results[i].score));
        PyList_SET_ITEM(result, i, item);
    }
    free(results);

    return result;
}

/*============================================================================
 * 版本信息
 *============================================================================*/

/* jw_vecdb_version() -> version string */
static PyObject *py_jw_vecdb_version(PyObject *self, PyObject *args) {
    jw_str_t version = jw_vecdb_version();
    return jw_str_to_py_str(version);
}

/*============================================================================
 * 模块方法表
 *============================================================================*/

static PyMethodDef PyJinwoMethods[] = {
    {"vecdb_open", py_jw_vecdb_open, METH_VARARGS, "Open a JinWo VecDB database"},
    {"vecdb_close", py_jw_vecdb_close, METH_VARARGS, "Close a JinWo VecDB database"},
    {"vecdb_sync", py_jw_vecdb_sync, METH_VARARGS, "Sync a JinWo VecDB database to disk"},
    {"vecdb_create_collection", py_jw_vecdb_create_collection, METH_VARARGS, "Create a collection"},
    {"vecdb_get_collection", py_jw_vecdb_get_collection, METH_VARARGS, "Get a collection by name"},
    {"vecdb_drop_collection", py_jw_vecdb_drop_collection, METH_VARARGS, "Drop a collection"},
    {"vecdb_list_collections", py_jw_vecdb_list_collections, METH_VARARGS, "List all collections"},
    {"vecdb_insert", py_jw_vecdb_insert, METH_VARARGS, "Insert a vector"},
    {"vecdb_insert_batch", py_jw_vecdb_insert_batch, METH_VARARGS, "Insert vectors in batch"},
    {"vecdb_search", py_jw_vecdb_search, METH_VARARGS, "Search for similar vectors"},
    {"vecdb_version", py_jw_vecdb_version, METH_NOARGS, "Get JinWo VecDB version"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef pyjinwomodule = {
    PyModuleDef_HEAD_INIT,
    "_jinwo",
    "JinWo VecDB - Python bindings for the JinWo embedded vector database",
    -1,
    PyJinwoMethods
};

PyMODINIT_FUNC PyInit__jinwo(void) {
    return PyModule_Create(&pyjinwomodule);
}
