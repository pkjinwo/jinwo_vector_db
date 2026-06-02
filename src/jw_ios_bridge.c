/**
 * jw_ios_bridge.c - iOS C bridge for JinWo VecDB
 *
 * This file provides simple C wrapper functions that match the
 * @_silgen_name declarations in CBridge.swift.
 *
 * The actual C library API uses jw_str_t, jw_status_t, etc.
 * This bridge flattens the API for easier Swift interop.
 */

#include "jw_vecdb.h"
#include "jw_collection.h"
#include "jw_error.h"
#include <string.h>
#include <stdlib.h>

/*
 * 桥接用的搜索结果结构体，必须与 Swift 的 jw_search_result 完全一致。
 * Swift 定义: { var id: UInt64; var distance: Float }
 */
typedef struct {
    jw_uint64_t  id;
    jw_float32_t distance;
} jw_ios_search_result_t;

/* ---- VecDB ---- */

JW_API jw_vecdb_t *jw_ios_vecdb_open(const char *path, jw_uint32_t flags) {
    jw_vecdb_t *db = NULL;
    jw_str_t spath = {(char *)path, path ? strlen(path) : 0};
    jw_status_t rc = jw_vecdb_open(&spath, flags, &db);
    if (rc != JW_SUCCESS) {
        return NULL;
    }
    return db;
}

JW_API jw_status_t jw_ios_vecdb_close(jw_vecdb_t *db) {
    return jw_vecdb_close(db);
}

JW_API const char *jw_ios_version(void) {
    static char buf[64] = {0};
    if (buf[0] == '\0') {
        jw_str_t ver = jw_vecdb_version();
        jw_size_t len = ver.slen < 63 ? ver.slen : 63;
        memcpy(buf, ver.ptr, len);
        buf[len] = '\0';
    }
    return buf;
}

JW_API const char *jw_ios_strerror(jw_status_t code) {
    return jw_error_message(code);
}

/* ---- Collection ---- */

JW_API jw_collection_t *jw_ios_collection_create(jw_vecdb_t *db,
                                                  const char *name,
                                                  jw_dim_t dim) {
    jw_collection_t *coll = NULL;
    jw_str_t sname = {(char *)name, name ? strlen(name) : 0};
    jw_status_t rc = jw_vecdb_create_collection(db, &sname, dim, &coll);
    if (rc != JW_SUCCESS) {
        return NULL;
    }
    return coll;
}

JW_API jw_collection_t *jw_ios_collection_get(jw_vecdb_t *db,
                                                const char *name) {
    jw_str_t sname = {(char *)name, name ? strlen(name) : 0};
    return jw_vecdb_get_collection(db, &sname);
}

JW_API jw_status_t jw_ios_collection_destroy(jw_collection_t *coll) {
    jw_collection_destroy(coll);  /* void return */
    return JW_SUCCESS;
}

JW_API jw_status_t jw_ios_collection_insert(jw_collection_t *coll,
                                             const jw_float32_t *vec,
                                             jw_dim_t dim) {
    return jw_collection_insert(coll, vec, NULL);
}

JW_API jw_vid_t jw_ios_collection_insert_vid(jw_collection_t *coll,
                                              const jw_float32_t *vec,
                                              jw_dim_t dim) {
    jw_vid_t vid = 0;
    jw_collection_insert(coll, vec, &vid);
    return vid;
}

JW_API jw_status_t jw_ios_collection_delete(jw_collection_t *coll, jw_vid_t vid) {
    return jw_collection_delete(coll, vid);
}

JW_API jw_status_t jw_ios_collection_get_vector(jw_collection_t *coll, jw_vid_t vid,
                                                  jw_float32_t *out_vec) {
    return jw_collection_get(coll, vid, out_vec);
}

JW_API jw_size_t jw_ios_collection_count(const jw_collection_t *coll) {
    jw_collection_stats_t stats;
    jw_collection_get_stats(coll, &stats);
    return stats.count;
}

JW_API jw_ssize_t jw_ios_collection_search(jw_collection_t *coll,
                                            const jw_float32_t *query,
                                            jw_dim_t dim,
                                            jw_size_t k,
                                            jw_ios_search_result_t *results) {
    jw_search_options_t opts = {0};
    opts.k = k;
    opts.include_vectors = 0;
    opts.include_meta = 0;
    opts.nprobe = 0;     /* use default */
    opts.ef_search = 0;  /* use default */

    jw_search_result_ex_t *results_ex =
        (jw_search_result_ex_t *)malloc(k * sizeof(jw_search_result_ex_t));
    if (!results_ex) return -1;

    jw_size_t count = jw_collection_search(coll, query, &opts, results_ex);
    /* copy vid+score to simplified results (only id and score fields!) */
    for (jw_size_t i = 0; i < count && i < k; i++) {
        results[i].id = results_ex[i].vid;
        results[i].distance = results_ex[i].score;
    }
    free(results_ex);
    return (jw_ssize_t)count;
}

JW_API jw_status_t jw_ios_collection_build_index(jw_collection_t *coll) {
    return jw_collection_build_index(coll);
}

JW_API jw_dim_t jw_ios_collection_encoding_dim(const jw_collection_t *coll) {
    return coll->dim;
}

/* ---- Collection list ---- */

typedef struct jw_ios_strlist {
    char **items;
    jw_size_t count;
} jw_ios_strlist_t;

JW_API jw_ios_strlist_t *jw_ios_collection_list(jw_vecdb_t *db) {
    /* Get collection list */
    jw_str_t names[256];
    jw_size_t count = jw_vecdb_list_collections(db, names, 256);

    jw_ios_strlist_t *list = (jw_ios_strlist_t *)malloc(sizeof(jw_ios_strlist_t));
    if (!list) return NULL;

    list->items = (char **)malloc(sizeof(char *) * (count + 1));
    if (!list->items) {
        free(list);
        return NULL;
    }

    for (jw_size_t i = 0; i < count; i++) {
        jw_size_t len = names[i].slen;
        list->items[i] = (char *)malloc(len + 1);
        if (list->items[i]) {
            memcpy(list->items[i], names[i].ptr, len);
            list->items[i][len] = '\0';
        }
    }
    list->items[count] = NULL;
    list->count = count;
    return list;
}

JW_API void jw_ios_strlist_free(jw_ios_strlist_t *list) {
    if (!list) return;
    if (list->items) {
        for (jw_size_t i = 0; list->items[i] != NULL; i++) {
            free(list->items[i]);
        }
        free(list->items);
    }
    free(list);
}
