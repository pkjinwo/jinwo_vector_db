/*
 * jw_sort.c - JinWo VecDB 排序工具实现
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_sort.h"
#include "jw_string.h"
#include "jw_stdlib.h"

static void swap_bytes(void *a, void *b, jw_size_t size)
{
    jw_uint8_t *pa = (jw_uint8_t *)a;
    jw_uint8_t *pb = (jw_uint8_t *)b;
    for (jw_size_t i = 0; i < size; i++) {
        jw_uint8_t tmp = pa[i];
        pa[i] = pb[i];
        pb[i] = tmp;
    }
}

static jw_size_t partition(void *base, jw_size_t nmemb, jw_size_t size,
                          jw_int32_t (*compar)(const void *, const void *, void *),
                          void *user_data)
{
    jw_uint8_t *arr = (jw_uint8_t *)base;
    jw_size_t pivot_idx = nmemb / 2;
    jw_uint8_t *pivot = arr + pivot_idx * size;
    jw_size_t last_idx = nmemb - 1;

    swap_bytes(pivot, arr + last_idx * size, size);

    jw_size_t store_idx = 0;
    for (jw_size_t j = 0; j < last_idx; j++) {
        jw_uint8_t *elem = arr + j * size;
        if (compar(elem, arr + last_idx * size, user_data) < 0) {
            swap_bytes(elem, arr + store_idx * size, size);
            store_idx++;
        }
    }

    swap_bytes(arr + store_idx * size, arr + last_idx * size, size);

    return store_idx;
}

static void quick_sort(void *base, jw_size_t nmemb, jw_size_t size,
                      jw_int32_t (*compar)(const void *, const void *, void *),
                      void *user_data)
{
    if (nmemb <= 1) {
        return;
    }

    if (nmemb <= 16) {
        for (jw_size_t i = 1; i < nmemb; i++) {
            jw_uint8_t *arr = (jw_uint8_t *)base;
            jw_size_t j = i;
            while (j > 0) {
                jw_uint8_t *curr = arr + j * size;
                jw_uint8_t *prev = arr + (j - 1) * size;
                if (compar(prev, curr, user_data) > 0) {
                    swap_bytes(prev, curr, size);
                    j--;
                } else {
                    break;
                }
            }
        }
        return;
    }

    jw_uint8_t *arr = (jw_uint8_t *)base;
    jw_size_t pivot_idx = nmemb / 2;
    pivot_idx = partition(base, nmemb, size, compar, user_data);

    if (pivot_idx > 0) {
        quick_sort(arr, pivot_idx, size, compar, user_data);
    }
    if (pivot_idx + 1 < nmemb) {
        quick_sort(arr + (pivot_idx + 1) * size, nmemb - pivot_idx - 1, size, compar, user_data);
    }
}

static jw_int32_t wrap_compar_simple(const void *a, const void *b, void *user_data)
{
    jw_int32_t (*compar)(const void *, const void *) = (jw_int32_t (*)(const void *, const void *))user_data;
    return compar(a, b);
}

JW_API void jw_qsort(void *base,
                     jw_size_t nmemb,
                     jw_size_t size,
                     jw_int32_t (*compar)(const void *, const void *, void *),
                     void *user_data)
{
    if (base == NULL || nmemb == 0 || size == 0 || compar == NULL) {
        return;
    }

    quick_sort(base, nmemb, size, compar, user_data);
}

JW_API void jw_qsort_simple(void *base,
                             jw_size_t nmemb,
                             jw_size_t size,
                             jw_int32_t (*compar)(const void *, const void *))
{
    if (base == NULL || nmemb == 0 || size == 0 || compar == NULL) {
        return;
    }

    quick_sort(base, nmemb, size, wrap_compar_simple, (void *)compar);
}

JW_API void jw_insertion_sort(void *base,
                              jw_size_t nmemb,
                              jw_size_t size,
                              jw_int32_t (*compar)(const void *, const void *, void *),
                              void *user_data)
{
    if (base == NULL || nmemb <= 1 || size == 0 || compar == NULL) {
        return;
    }

    jw_uint8_t *arr = (jw_uint8_t *)base;
    jw_uint8_t *temp = (jw_uint8_t *)jw_malloc(size);
    if (temp == NULL) {
        return;
    }

    for (jw_size_t i = 1; i < nmemb; i++) {
        jw_size_t j = i;
        jw_memcpy(temp, arr + i * size, size);

        while (j > 0 && compar(arr + (j - 1) * size, temp, user_data) > 0) {
            jw_memcpy(arr + j * size, arr + (j - 1) * size, size);
            j--;
        }

        jw_memcpy(arr + j * size, temp, size);
    }

    jw_free(temp);
}
