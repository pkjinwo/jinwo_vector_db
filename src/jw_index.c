/*
 * jw_index.c - JinWo VecDB 索引实现
 *
 * Copyright 2026 北京金幄科技有限公司
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * =============================================================================
 *
 * 索引算法实现详解
 *
 * 本文件实现了两种主流向量索引算法:
 *
 * 1. IVF (Inverted File Index) - 倒排索引
 *    原理:
 *    - 通过K-means将向量空间划分为nlist个聚类中心
 *    - 每个向量归属于距离最近的聚类中心对应的倒排列表
 *    - 查询时只搜索nprobe个最近的聚类中心对应的列表
 *    优点: 内存占用小，适合超大规模数据
 *    缺点: 精确度依赖nprobe参数
 *
 * 2. HNSW (Hierarchical Navigable Small World) - 层级导航小世界图
 *    原理:
 *    - 构建多层图结构，上层稀疏、下层稠密
 *    - 查询时从上层入口点向下层贪婪搜索
 *    - 使用优先队列维护候选结果
 *    优点: 查询速度快，精度高
 *    缺点: 内存占用相对较高
 *
 * =============================================================================
 */

#include "jw_index.h"
#include "jw_quant.h"
#include "jw_vector.h"
#include "jw_string.h"
#include "jw_stdio.h"
#include "jw_file.h"
#include "jw_storage.h"
#include "jw_arena.h"
#include "jw_sort.h"
#include "jw_math.h"
#include <stdio.h>
#include <fcntl.h>
#include <inttypes.h>

#ifdef __ANDROID__
#include <android/log.h>
#define JW_LOG_TAG "jinwo_vecdb"
#define JW_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, JW_LOG_TAG, __VA_ARGS__)
#define JW_LOGW(...) __android_log_print(ANDROID_LOG_WARN,  JW_LOG_TAG, __VA_ARGS__)
#define JW_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  JW_LOG_TAG, __VA_ARGS__)
#else
#define JW_LOGE(...) ((void)0)
#define JW_LOGW(...) ((void)0)
#define JW_LOGI(...) ((void)0)
#endif

/**
 * 判断一个指针是否像是合法的堆/分配地址。
 * 在 64 位系统上，低于 0x10000 的地址不可能是合法的用户态内存地址。
 * 如果 arena 的 bump 分配失败或 memset 被优化掉，vec 可能会残留这种垃圾值。
 */
static int is_valid_heap_ptr(const void *ptr) {
    return (ptr != NULL && ((uintptr_t)ptr) >= (uintptr_t)0x10000);
}

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
#include <io.h>
#ifndef ssize_t
typedef SSIZE_T ssize_t;
#endif
#define open  _open
#define close _close
#define read  _read
#define write _write
#define lseek _lseeki64
#else
#include <unistd.h>
#endif

/*
 * =============================================================================
 * 内部数据类型定义
 * =============================================================================
 */

/**
 * 搜索结果项结构
 * 用于优先队列中的元素
 */
typedef struct {
    jw_vid_t vid;               /**< 向量ID */
    jw_score_t score;           /**< 距离分数 (距离越小分数越低) */
    jw_uint32_t level;          /**< 节点层级 (仅HNSW使用) */
} search_candidate_t;

/* 前向声明 */
static jw_float32_t distance_sq(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim);
static jw_status_t kmeans_cluster(jw_arena_t *arena,
                                   jw_cvec_t vectors,
                                   jw_size_t count,
                                   jw_dim_t dim,
                                   jw_uint32_t k,
                                   jw_uint32_t max_iter,
                                   jw_metric_t metric,
                                   jw_vec_t centroids);

/*
 * HNSW_PQ 支持: 判断节点是否携带可用数据
 * PQ 模式下节点只存 code (vec 为 NULL, 与 IVF_PQ 一致, 用于省内存)
 */
static int hnsw_node_usable(const jw_hnsw_index_t *hnsw,
                            const jw_hnsw_node_t *node)
{
    if (node == NULL) {
        return 0;
    }
    if (hnsw->pq != NULL) {
        return node->code != NULL;
    }
    return node->vec != NULL;
}

/*
 * HNSW_PQ 支持: 查询向量与节点间的距离
 *   PQ 模式: 用码本近似距离 (query 为原始向量, node 侧为编码)
 *   普通模式: 用原始向量精确距离
 * 插入建边时也可复用本函数, 把"新节点的原始向量"当 query 传入。
 */
static jw_float32_t hnsw_node_dist(const jw_hnsw_index_t *hnsw,
                                   jw_cvec_t query,
                                   const jw_hnsw_node_t *node)
{
    if (hnsw->pq != NULL) {
        if (node->code != NULL) {
            return jw_pq_distance(hnsw->pq, query, node->code, hnsw->metric);
        }
        return JW_FLT_MAX;
    }
    if (node->vec != NULL) {
        return jw_vec_distance(query, node->vec, hnsw->dim, hnsw->metric);
    }
    return JW_FLT_MAX;
}

/*
 * =============================================================================
 * 向量量化实现
 * =============================================================================
 */

/**
 * 计算两个节点之间的距离 (双向, 不依赖"谁是查询")
 *
 * 用于 HNSW 插入时的"替换最远连接"决策: 必须站在"被挤占的邻居"视角
 * 比较它的各条现有连接, 而不是站在新插入向量的视角。
 *
 * 优先级: 双方都有原始 vec 时用 jw_vec_distance;
 *         否则退化为用 a 的 vec/code 作查询算到 b 的距离
 *         (PQ 模式下 vec 为 NULL, 只能用 code 侧, 结果为近似)。
 */
static jw_float32_t hnsw_node_pair_dist(const jw_hnsw_index_t *hnsw,
                                        const jw_hnsw_node_t *a,
                                        const jw_hnsw_node_t *b)
{
    if (a == NULL || b == NULL) {
        return JW_FLT_MAX;
    }
    if (a->vec != NULL && b->vec != NULL) {
        return jw_vec_distance(a->vec, b->vec, hnsw->dim, hnsw->metric);
    }
    /* 至少一方只有 code (PQ 模式) */
    if (a->vec != NULL && b->code != NULL && hnsw->pq != NULL) {
        return jw_pq_distance(hnsw->pq, a->vec, b->code, hnsw->metric);
    }
    if (b->vec != NULL && a->code != NULL && hnsw->pq != NULL) {
        return jw_pq_distance(hnsw->pq, b->vec, a->code, hnsw->metric);
    }
    return JW_FLT_MAX;
}

/*
 * 注意: PQ量化器的创建、训练、编码、距离计算等功能
 * 已在 jw_quant.h/jw_quant.c 中实现。
 * IVF/HNSW索引中的pq字段指向 jw_pq_quantizer_t 结构。
 */

/*
 * =============================================================================
 * 优先队列 (最小堆) 实现
 * =============================================================================
 *
 * 优先队列用于KNN搜索中维护top-K候选结果
 * 实现为最小堆，堆顶是距离最大的候选，当新候选距离更小时替换堆顶
 */

/**
 * 优先队列结构
 */
typedef struct {
    search_candidate_t *items;  /**< 候选数组 */
    jw_size_t size;            /**< 当前元素数量 */
    jw_size_t capacity;        /**< 队列容量 */
} priority_queue_t;

/**
 * 初始化优先队列
 *
 * @param pq        优先队列指针
 * @param capacity  队列容量 (即K值)
 * @param arena      内存池
 */
static void pq_init(priority_queue_t *pq, jw_size_t capacity, jw_arena_t *arena)
{
    pq->items = (search_candidate_t *)jw_arena_alloc(arena,
                capacity * sizeof(search_candidate_t));
    pq->size = 0;
    pq->capacity = (pq->items != NULL) ? capacity : 0;
}

/**
 * 向优先队列添加候选
 *
 * 如果队列未满，直接插入尾部并上浮
 * 如果队列已满且新候选更优，替换堆顶并下沉
 *
 * @param pq    优先队列指针
 * @param vid   向量ID
 * @param score 距离分数
 * @param level 节点层级 (HNSW用)
 */
static void pq_push(priority_queue_t *pq, jw_vid_t vid,
                    jw_score_t score, jw_uint32_t level)
{
    if (pq->items == NULL || pq->capacity == 0) return;
    if (pq->size < pq->capacity) {
        /** 队列未满，直接插入到尾部 */
        jw_size_t i = pq->size++;
        pq->items[i].vid = vid;
        pq->items[i].score = score;
        pq->items[i].level = level;

        /** 上浮操作: 将新元素移动到正确位置 */
        while (i > 0) {
            jw_size_t parent = (i - 1) / 2;
            if (pq->items[parent].score >= pq->items[i].score) {
                break;  /**< 父节点分数不小于子节点，最大堆无需继续上浮 */
            }
            /** 交换父子节点 */
            search_candidate_t tmp = pq->items[parent];
            pq->items[parent] = pq->items[i];
            pq->items[i] = tmp;
            i = parent;
        }
    } else if (score < pq->items[0].score) {
        /** 队列已满但新候选更优，替换堆顶 */
        pq->items[0].vid = vid;
        pq->items[0].score = score;
        pq->items[0].level = level;

        /** 下沉操作: 将堆顶元素下沉到正确位置 */
        jw_size_t i = 0;
        while (1) {
            jw_size_t left = 2 * i + 1;
            jw_size_t right = 2 * i + 2;
            jw_size_t smallest = i;

            /** 找到三个节点中最大的 (最大堆: 堆顶为最远候选) */
            if (left < pq->size &&
                pq->items[left].score > pq->items[smallest].score) {
                smallest = left;
            }
            if (right < pq->size &&
                pq->items[right].score > pq->items[smallest].score) {
                smallest = right;
            }

            if (smallest == i) {
                break;  /**< 已经是最大堆，无需继续下沉 */
            }

            /** 交换当前节点与最小子节点 */
            search_candidate_t tmp = pq->items[i];
            pq->items[i] = pq->items[smallest];
            pq->items[smallest] = tmp;
            i = smallest;
        }
    }
}

/**
 * 检查候选是否已在队列中
 *
 * @param pq    优先队列
 * @param vid   向量ID
 * @return JW_TRUE 如果已存在
 */
static jw_bool_t pq_contains(const priority_queue_t *pq, jw_vid_t vid)
{
    for (jw_size_t i = 0; i < pq->size; i++) {
        if (pq->items[i].vid == vid) {
            return JW_TRUE;
        }
    }
    return JW_FALSE;
}

/*
 * =============================================================================
 * 辅助比较函数
 * =============================================================================
 */

/**
 * 比较函数: 按分数升序排序搜索结果
 *
 * 用于最终结果的排序
 *
 * @param a  结果A
 * @param b  结果B
 * @return   负数表示A分数更小(更优)
 */
static int compare_results(const void *a, const void *b)
{
    const jw_search_result_t *ra = (const jw_search_result_t *)a;
    const jw_search_result_t *rb = (const jw_search_result_t *)b;

    if (ra->score < rb->score) return -1;
    if (ra->score > rb->score) return 1;
    return 0;
}

/**
 * search_candidate_t 比较 (按距离升序)
 * 用于 HNSW 搜索后对完整结果集排序再取前 k 个
 */
static int compare_candidates(const void *a, const void *b)
{
    const search_candidate_t *ca = (const search_candidate_t *)a;
    const search_candidate_t *cb = (const search_candidate_t *)b;

    if (ca->score < cb->score) return -1;
    if (ca->score > cb->score) return 1;
    return 0;
}

/*
 * =============================================================================
 * K-means聚类算法实现
 * =============================================================================
 *
 * K-means是最基础的聚类算法，用于IVF索引构建聚类中心
 * 算法步骤:
 * 1. 随机选择K个点作为初始聚类中心
 * 2. 分配每个点到最近的聚类中心
 * 3. 重新计算每个聚类的中心
 * 4. 重复2-3直到收敛或达到最大迭代次数
 */

/**
 * 计算欧氏距离的平方 (避免开方加速计算)
 *
 * @param a     向量A
 * @param b     向量B
 * @param dim   向量维度
 * @return      距离平方
 */
static jw_float32_t distance_sq(jw_cvec_t a, jw_cvec_t b, jw_dim_t dim)
{
    if (a == NULL || b == NULL || dim == 0) {
        return JW_FLT_MAX;
    }
    /** 防御：过滤不是合法堆地址的垃圾指针 */
    if (!is_valid_heap_ptr(a)) {
        JW_LOGE("distance_sq: GARBAGE VEC A! a=%p", (const void*)a);
        return JW_FLT_MAX;
    }
    if (!is_valid_heap_ptr(b)) {
        JW_LOGE("distance_sq: GARBAGE VEC B! b=%p", (const void*)b);
        return JW_FLT_MAX;
    }
    jw_float32_t dist_sq = 0.0f;
    for (jw_dim_t d = 0; d < dim; d++) {
        jw_float32_t diff = a[d] - b[d];
        dist_sq += diff * diff;
    }
    return dist_sq;
}

/**
 * K-means聚类主函数
 *
 * @param arena       内存池
 * @param vectors    输入向量数组 (连续存储)
 * @param count      向量数量
 * @param dim        向量维度
 * @param k          聚类数量 (即nlist)
 * @param max_iter   最大迭代次数
 * @param centroids  输出: 聚类中心数组
 * @return           状态码
 */
static jw_status_t kmeans_cluster(jw_arena_t *arena,
                                   jw_cvec_t vectors,
                                   jw_size_t count,
                                   jw_dim_t dim,
                                   jw_uint32_t k,
                                   jw_uint32_t max_iter,
                                   jw_metric_t metric,
                                   jw_vec_t centroids)
{
    /** 参数校验: 向量数必须大于聚类数 */
    if (count < k) {
        return JW_INVALID_PARAM;
    }

    /**
     * 第一步: 初始化聚类中心
     * 使用K-means++策略选择初始中心，比纯随机效果更好
     * 第一个中心随机选择，后续中心按概率加权选择距离现有中心更远的点
     */
    for (jw_uint32_t i = 0; i < k; i++) {
        jw_size_t idx;
        if (i == 0) {
            /** 第一个中心: 随机选择 */
            idx = (jw_uint32_t)(jw_rand() % count);
        } else {
            /** K-means++: 选择距离现有中心最远的点作为下一个中心 */
            jw_float32_t max_min_dist = 0.0f;
            for (jw_size_t j = 0; j < count; j++) {
                jw_float32_t min_dist = JW_FLT_MAX;
                for (jw_uint32_t c = 0; c <= i - 1; c++) {
                    jw_float32_t d = jw_vec_distance(vectors + j * dim,
                                                  centroids + c * dim, dim, metric);
                    if (d < min_dist) min_dist = d;
                }
                if (min_dist > max_min_dist) {
                    max_min_dist = min_dist;
                    idx = j;
                }
            }
        }
        jw_memcpy(centroids + i * dim, vectors + idx * dim,
                  dim * sizeof(jw_float32_t));
    }

    /** 分配临时数组 */
    jw_uint32_t *assignments = (jw_uint32_t *)jw_arena_alloc(arena,
                                      count * sizeof(jw_uint32_t));
    jw_uint32_t *counts = (jw_uint32_t *)jw_arena_alloc(arena,
                              k * sizeof(jw_uint32_t));
    jw_vec_t new_centroids = (jw_vec_t)jw_arena_alloc(arena,
                                k * dim * sizeof(jw_float32_t));

    if (assignments == NULL || counts == NULL || new_centroids == NULL) {
        return JW_OUT_OF_MEMORY;
    }

    /**
     * 第二步: 迭代优化
     * 每次迭代: 分配 -> 重新计算中心 -> 检查收敛
     */
    for (jw_uint32_t iter = 0; iter < max_iter; iter++) {
        /** 步骤2.1: 分配每个点到最近的聚类中心 */
        for (jw_size_t i = 0; i < count; i++) {
            jw_cvec_t vec = vectors + i * dim;
            jw_float32_t min_dist = JW_FLT_MAX;
            jw_uint32_t min_idx = 0;

            for (jw_uint32_t j = 0; j < k; j++) {
                jw_float32_t dist = jw_vec_distance(vec, centroids + j * dim, dim, metric);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_idx = j;
                }
            }

            assignments[i] = min_idx;
        }

        /** 步骤2.2: 重新计算聚类中心 */
        jw_memset(counts, 0, k * sizeof(jw_uint32_t));
        jw_memset(new_centroids, 0, k * dim * sizeof(jw_float32_t));

        for (jw_size_t i = 0; i < count; i++) {
            jw_uint32_t c = assignments[i];
            jw_cvec_t vec = vectors + i * dim;

            for (jw_dim_t d = 0; d < dim; d++) {
                new_centroids[c * dim + d] += vec[d];
            }
            counts[c]++;
        }

        /** 计算新的聚类中心 = 该簇所有点的均值 */
        for (jw_uint32_t i = 0; i < k; i++) {
            if (counts[i] > 0) {
                for (jw_dim_t d = 0; d < dim; d++) {
                    centroids[i * dim + d] = new_centroids[i * dim + d] / counts[i];
                }
            }
        }
    }

    return JW_SUCCESS;
}

/*
 * =============================================================================
 * HNSW随机层级生成
 * =============================================================================
 */

/**
 * 生成节点随机层级
 *
 * 使用指数分布生成层级，层级越高节点越少
 * 公式: level = floor(-ln(rand) * level_mult)
 * 其中level_mult默认值为1/ln(M)
 *
 * @param config  HNSW配置
 * @return        节点层级 (从0开始)
 */
static jw_uint32_t hnsw_generate_level(const jw_hnsw_config_t *config)
{
    /** 生成0-1之间的均匀随机数 */
    jw_float32_t r = (jw_rand() & 0xFFFF) / (jw_float32_t)0xFFFF;

    /**
     * 使用指数分布计算层级
     * level_mult控制层级的衰减速度
     * M=16时，level_mult≈0.23，节点出现在第0层的概率约为80%
     */
    jw_float32_t mult = config->level_mult;
    if (mult <= 0) {
        /* M < 2 时 log(M) <= 0 会除零, 兜底用 M=16 的衰减系数 */
        jw_float32_t M = (config->M >= 2) ? (jw_float32_t)config->M : 16.0f;
        mult = 1.0f / jw_math_log_f32(M);
    }

    jw_uint32_t level = (jw_uint32_t)(-jw_math_log_f32(r) * mult);

    /** 限制最大层级 */
    if (config->max_level > 0 && level > config->max_level) {
        level = config->max_level;
    }

    return level;
}

/*
 * =============================================================================
 * HNSW贪婪搜索算法
 * =============================================================================
 *
 * 从给定层级开始，贪婪地向更近的邻居移动
 * 用于HNSW查询的每层搜索
 *
 * @param hnsw       HNSW索引
 * @param query      查询向量
 * @param ep_vid     入口点向量ID
 * @param level      搜索起始层级
 * @param visit_mark 已访问标记数组 (大小=capacity, 调用方负责清零;
 *                   用 O(1) 标记替代原来的 O(n) 线性查找, 避免插入/查询
 *                   复杂度退化)
 * @param arena       内存池
 * @return           最近邻的向量ID
 */
static jw_vid_t hnsw_greedy_search(const jw_hnsw_index_t *hnsw,
                                    jw_cvec_t query,
                                    jw_vid_t ep_vid,
                                    jw_uint32_t level,
                                    jw_uint8_t *visit_mark,
                                    jw_arena_t *arena)
{
    /** 入口点合法性检查 */
    if (ep_vid >= hnsw->capacity || hnsw->nodes[ep_vid] == NULL
        || hnsw->nodes[ep_vid]->deleted) {
        return ep_vid;
    }
    jw_hnsw_node_t *gep_node = hnsw->nodes[ep_vid];
    if (!hnsw_node_usable(hnsw, gep_node)) {
        JW_LOGE("hnsw_greedy_search: BAD ENTRY POINT DATA!"
            " ep_vid=%" PRIu64 " node=%p vec=%p code=%p",
            (uint64_t)ep_vid, (void*)gep_node,
            gep_node ? (void*)gep_node->vec : NULL,
            gep_node ? (void*)gep_node->code : NULL);
        return ep_vid;
    }

    jw_vid_t current = ep_vid;
    jw_vid_t nearest = ep_vid;
    jw_float32_t nearest_dist = hnsw_node_dist(hnsw, query,
                             hnsw->nodes[ep_vid]);
    jw_float32_t current_dist;

    while (1) {
        jw_vid_t prev_nearest = nearest;
        jw_hnsw_node_t *node = hnsw->nodes[current];

        /* 防御: node 可能已被并发删除置 NULL; 且 link_counts/links 数组只
         * 分配到 node->level+1, 若 level 越界会读到相邻内存 (竞态崩溃根因) */
        if (node == NULL || level > node->level) {
            break;
        }

        /** 遍历当前节点的所有邻居 (第0层容量为 max_M0 = 2*M) */
        jw_uint32_t cap = (level == 0) ? node->max_M0 : node->max_M;
        for (jw_uint32_t i = 0; i < node->link_counts[level] && i < cap; i++) {
            jw_vid_t neighbor = node->links[level][i];

            /** 跳过已删除或无效的邻居 */
            if (neighbor >= hnsw->capacity || hnsw->nodes[neighbor] == NULL
                || hnsw->nodes[neighbor]->deleted) {
                continue;
            }

            /** 检查数据指针：仅 NULL 不够，还要过滤垃圾小值 */
            jw_hnsw_node_t *gnb_node = hnsw->nodes[neighbor];
            if (!hnsw_node_usable(hnsw, gnb_node)) {
                continue;
            }
            if (hnsw->pq == NULL && !is_valid_heap_ptr(gnb_node->vec)) {
                JW_LOGE("hnsw_greedy_search: GARBAGE VEC! neighbor=%" PRIu64
                    " node=%p vec=%p vid=%" PRIu64 " level=%u deleted=%d"
                    " links=%p link_counts=%p max_M=%u max_M0=%u",
                    (uint64_t)neighbor, (void*)gnb_node, (void*)gnb_node->vec,
                    (uint64_t)gnb_node->vid, (unsigned)gnb_node->level,
                    (int)gnb_node->deleted,
                    (void*)gnb_node->links, (void*)gnb_node->link_counts,
                    (unsigned)gnb_node->max_M, (unsigned)gnb_node->max_M0);
                continue;
            }

            /** O(1) 已访问判断 (替代原来的线性扫描) */
            if (visit_mark[neighbor]) {
                continue;
            }
            visit_mark[neighbor] = 1;

            /** 计算距离 */
            current_dist = hnsw_node_dist(hnsw, query, gnb_node);

                /** 如果找到更近的邻居，更新最近邻 */
                if (current_dist < nearest_dist) {
                    nearest_dist = current_dist;
                    nearest = neighbor;
                }
                }

                /** 如果没有找到更近的邻居，搜索结束 */
                if (nearest == prev_nearest) {
                break;
                }

        current = nearest;
    }

    return nearest;
}

/*
 * =============================================================================
 * HNSW层内搜索 (Best-First Search)
 * =============================================================================
 *
 * 使用优先队列实现的最优优先搜索
 * 在给定层级搜索ef_search个最近邻
 *
 * @param hnsw         HNSW索引
 * @param query        查询向量
 * @param ep_vid       入口点向量ID
 * @param level        搜索层级
 * @param ef_search    搜索宽度
 * @param results      输出: 搜索结果
 * @param result_count 输出: 结果数量
 * @param arena         内存池
 */
static void hnsw_layer_search(const jw_hnsw_index_t *hnsw,
                              jw_cvec_t query,
                              jw_vid_t ep_vid,
                              jw_uint32_t level,
                              jw_uint32_t ef_search,
                              search_candidate_t *results,
                              jw_size_t *result_count,
                              jw_arena_t *arena)
{
    /** 入口点合法性检查 */
    if (ep_vid >= hnsw->capacity || hnsw->nodes[ep_vid] == NULL
        || hnsw->nodes[ep_vid]->deleted) {
        *result_count = 0;
        return;
    }
    jw_hnsw_node_t *ep_node = hnsw->nodes[ep_vid];
    if (!hnsw_node_usable(hnsw, ep_node)) {
        JW_LOGE("hnsw_layer_search: BAD ENTRY POINT DATA!"
            " ep_vid=%" PRIu64 " node=%p vec=%p code=%p",
            (uint64_t)ep_vid, (void*)ep_node,
            ep_node ? (void*)ep_node->vec : NULL,
            ep_node ? (void*)ep_node->code : NULL);
        *result_count = 0;
        return;
    }

    /** 已访问标记数组 (O(1) 判断, 替代原来的线性扫描) */
    jw_uint8_t *visit_mark = (jw_uint8_t *)jw_arena_alloc(arena, hnsw->capacity);
    if (visit_mark == NULL) {
        *result_count = 0;
        return;
    }
    jw_memset(visit_mark, 0, hnsw->capacity);

    /** 结果优先队列 (最大堆: 堆顶是ef_search个结果中最远的) */
    priority_queue_t pq;
    pq_init(&pq, ef_search, arena);

    /** 待探索候选数组 (按加入顺序存储，每次选最近的来探索) */
    jw_vid_t *todo = (jw_vid_t *)jw_arena_alloc(arena,
                      hnsw->capacity * sizeof(jw_vid_t));
    jw_float32_t *todo_dist = (jw_float32_t *)jw_arena_alloc(arena,
                                hnsw->capacity * sizeof(jw_float32_t));
    jw_size_t todo_count = 0;

    /* 防御: 任一分配失败都不可继续 (否则写 NULL 崩溃) */
    if (pq.items == NULL || todo == NULL || todo_dist == NULL) {
        *result_count = 0;
        return;
    }

    /** 初始化: 入口点 */
    visit_mark[ep_vid] = 1;
    jw_float32_t ep_dist = hnsw_node_dist(hnsw, query, hnsw->nodes[ep_vid]);
    pq_push(&pq, ep_vid, ep_dist, level);
    todo[todo_count] = ep_vid;
    todo_dist[todo_count] = ep_dist;
    todo_count++;

    /**
     * Best-First Search主循环:
     * 每次从待探索列表中选择距离最近的候选探索其邻居
     */
    while (todo_count > 0) {
        /** 在 todo 中找距离最小的候选 */
        jw_size_t best_idx = 0;
        jw_float32_t best_dist = todo_dist[0];
        for (jw_size_t t = 1; t < todo_count; t++) {
            if (todo_dist[t] < best_dist) {
                best_dist = todo_dist[t];
                best_idx = t;
            }
        }

        jw_vid_t current_vid = todo[best_idx];
        jw_float32_t current_dist = best_dist;

        /** 从 todo 中移除 (用最后一个覆盖) */
        todo[best_idx] = todo[todo_count - 1];
        todo_dist[best_idx] = todo_dist[todo_count - 1];
        todo_count--;

        /**
         * 终止条件:
         * 当前候选距离 > 结果集中最远的距离 → 不可能有更近的了
         */
        /* 仅在结果集填满(达到 ef_search)后, 才用"最远结果"做提前终止。
         * 否则结果集尚小, items[0](最大堆堆顶=最远) 只是少量样本的最远值,
         * 误用作终止阈值会让 BFS 过早结束, 导致召回率随 ef_search 抖动。 */
        jw_float32_t max_result_dist = JW_FLT_MAX;
        if (pq.size >= ef_search) {
            max_result_dist = pq.items[0].score;
        }
        if (current_dist > max_result_dist) {
            break;
        }

        /** 探索当前候选的邻居 (第0层容量为 max_M0) */
        jw_hnsw_node_t *node = hnsw->nodes[current_vid];
        /* 防御: node 可能已被并发删除置 NULL; 且 link_counts/links 数组只
         * 分配到 node->level+1, 若 level 越界会读到相邻内存 (竞态崩溃根因) */
        if (node == NULL || level > node->level) {
            continue;
        }
        jw_uint32_t cap = (level == 0) ? node->max_M0 : node->max_M;
        jw_uint32_t link_end = node->link_counts[level];
        if (link_end > cap) link_end = cap;

        for (jw_uint32_t i = 0; i < link_end; i++) {
            jw_vid_t neighbor = node->links[level][i];

            /** 跳过已删除或无效的邻居 */
            if (neighbor >= hnsw->capacity || hnsw->nodes[neighbor] == NULL
                || hnsw->nodes[neighbor]->deleted) {
                continue;
            }

            /** 检查数据指针：仅 NULL 不够，还要过滤垃圾小值 */
            jw_hnsw_node_t *nb_node = hnsw->nodes[neighbor];
            if (!hnsw_node_usable(hnsw, nb_node)) {
                continue;
            }
            if (hnsw->pq == NULL && !is_valid_heap_ptr(nb_node->vec)) {
                JW_LOGE("hnsw_layer_search: GARBAGE VEC! neighbor=%" PRIu64
                    " node=%p vec=%p vid=%" PRIu64 " level=%u deleted=%d"
                    " links=%p link_counts=%p max_M=%u max_M0=%u",
                    (uint64_t)neighbor, (void*)nb_node, (void*)nb_node->vec,
                    (uint64_t)nb_node->vid, (unsigned)nb_node->level,
                    (int)nb_node->deleted,
                    (void*)nb_node->links, (void*)nb_node->link_counts,
                    (unsigned)nb_node->max_M, (unsigned)nb_node->max_M0);
                continue;
            }

            /** O(1) 已访问判断 */
            if (visit_mark[neighbor]) {
                continue;
            }
            visit_mark[neighbor] = 1;

            jw_float32_t dist = hnsw_node_dist(hnsw, query, nb_node);

            /** 加入结果集 (最大堆) */
            pq_push(&pq, neighbor, dist, level);

            /** 加入待探索列表 */
            todo[todo_count] = neighbor;
            todo_dist[todo_count] = dist;
            todo_count++;
        }
    }

    /**
     * 复制结果
     *
     * 修复 (2026-10-22): 原先直接拷贝 pq.items[] 数组, 但该数组是**最大堆的
     * 堆序存储**(items[0] 恒为最远候选), 既未按距离排序, 前 ef 个元素也不是
     * "最近的 ef 个"。上层 hnsw_select_neighbors 会在这个乱序数组上选最近 M 个,
     * 于是 ef_construction 越大, 引入的坏候选越多, 构图质量反而越差。
     *
     * 实测: 修复前 recall@5 随 ef 增大而**下降** (M=32: ef=32 时 100%,
     * ef=256 时 60%), 完全违背 HNSW 理论。
     *
     * 现改为反复取堆顶(最远)放到末尾, 即堆排序, 得到按距离**升序**的结果数组,
     * 前 ef 个恰为最近的 ef 个候选。
     */
    jw_size_t out_n = (pq.size < ef_search) ? pq.size : ef_search;
    jw_size_t heap_n = pq.size;

    /* 在 pq.items[0..heap_n) 上原地堆排序 (升序) */
    for (jw_size_t i = heap_n; i > 0; i--) {
        /* 堆顶(最远)换到末尾 */
        search_candidate_t tmp = pq.items[0];
        pq.items[0] = pq.items[i - 1];
        pq.items[i - 1] = tmp;

        /* 对剩余 [0, i-1) 重新下沉成最大堆 */
        jw_size_t idx = 0;
        while (1) {
            jw_size_t left = 2 * idx + 1;
            jw_size_t right = 2 * idx + 2;
            jw_size_t largest = idx;
            if (left < i - 1 && pq.items[left].score > pq.items[largest].score) {
                largest = left;
            }
            if (right < i - 1 && pq.items[right].score > pq.items[largest].score) {
                largest = right;
            }
            if (largest == idx) {
                break;
            }
            search_candidate_t t2 = pq.items[idx];
            pq.items[idx] = pq.items[largest];
            pq.items[largest] = t2;
            idx = largest;
        }
    }

    /* 排序后 items[0..out_n) 即最近的 out_n 个, 按距离升序 */
    *result_count = out_n;
    for (jw_size_t i = 0; i < out_n; i++) {
        results[i] = pq.items[i];
    }
}

/*
 * =============================================================================
 * HNSW邻居选择算法
 * =============================================================================
 *
 * 选择最优秀的M个邻居
 * 使用简单贪婪策略: 距离更近的优先保留
 *
 * @param neighbors   候选邻居数组
 * @param count       候选邻居数量
 * @param M           要选择的邻居数量
 * @param hnsw        HNSW索引
 * @param query       查询向量 (用于判断邻居的距离)
 * @param level       当前层级
 * @param arena        内存池
 */
static void hnsw_select_neighbors(jw_hnsw_node_t **neighbors,
                                   jw_uint32_t count,
                                   jw_uint32_t M,
                                   const jw_hnsw_index_t *hnsw,
                                   jw_cvec_t query,
                                   jw_uint32_t level,
                                   jw_arena_t *arena)
{
    /** 如果候选数小于M，全部保留 */
    if (count <= M) {
        return;
    }

    /** 使用简单的选择排序保留最近的M个 */
    for (jw_uint32_t i = 0; i < M; i++) {
        jw_uint32_t min_idx = i;
        jw_float32_t min_dist = hnsw_node_usable(hnsw, neighbors[i])
            ? hnsw_node_dist(hnsw, query, neighbors[i])
            : JW_FLT_MAX;

        for (jw_uint32_t j = i + 1; j < count; j++) {
            jw_float32_t dist = hnsw_node_usable(hnsw, neighbors[j])
                ? hnsw_node_dist(hnsw, query, neighbors[j])
                : JW_FLT_MAX;
            if (dist < min_dist) {
                min_dist = dist;
                min_idx = j;
            }
        }

        /** 交换 */
        if (min_idx != i) {
            jw_hnsw_node_t *tmp = neighbors[i];
            neighbors[i] = neighbors[min_idx];
            neighbors[min_idx] = tmp;
        }
    }
}

/*
 * =============================================================================
 * 通用索引接口实现
 * =============================================================================
 */

/**
 * 创建索引
 *
 * @param arena   内存池，如果为NULL则创建独立内存池
 * @param config 索引配置
 * @return 索引指针，失败返回NULL
 */
JW_API jw_index_t *jw_index_create(jw_arena_t *arena,
                                    const jw_index_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }

    /** 如果未传入arena，则创建独立的内存池 */
    jw_arena_t *local_arena = arena;
    jw_bool_t owns_arena = JW_FALSE;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
        owns_arena = JW_TRUE;
    }

    jw_index_t *index = (jw_index_t *)jw_arena_calloc(local_arena, 1, sizeof(jw_index_t));
    if (index == NULL) {
        if (owns_arena) jw_arena_destroy(local_arena);
        return NULL;
    }

    index->type = config->type;
    index->arena = local_arena;

    /** 根据索引类型创建对应的实现 */
    switch (config->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ: {
            jw_ivf_index_t *ivf = jw_ivf_create(index->arena, config->dim,
                                                 &config->params.ivf);
            if (ivf != NULL) {
                ivf->type = config->type;
                ivf->quant_type = config->quant;
                ivf->metric = config->metric;  /**< 修复: 传递度量类型, 否则 IVF 始终按 L2 计算 */
                if (config->type == JW_INDEX_IVF_PQ) {
                    /** 创建PQ量化器 */
                    jw_pq_quantizer_t *pq = NULL;
                    jw_pq_create(ivf->arena, config->dim, 
                                config->pq.nsub, 
                                config->pq.nbits, 
                                &pq);
                    ivf->pq = pq;
                } else if (config->type == JW_INDEX_IVF_SQ || config->quant == JW_QUANT_INT8 || config->quant == JW_QUANT_UINT8) {
                    /** 创建SQ量化器 */
                    ivf->sq = jw_sq_quantizer_create(ivf->arena, config->dim);
                }
            }
            index->impl = ivf;
            break;
        }
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ: {
            jw_hnsw_index_t *hnsw = jw_hnsw_create(index->arena, config->dim,
                                                     &config->params.hnsw);
            if (hnsw != NULL) {
                hnsw->type = config->type;
                hnsw->quant_type = config->quant;
                hnsw->metric = config->metric;  /**< 修复: 传递度量类型, 否则 HNSW 始终按 L2 计算 */
                
                if (config->type == JW_INDEX_HNSW_PQ) {
                    /** 创建PQ量化器 */
                    jw_pq_quantizer_t *pq = NULL;
                    jw_pq_create(hnsw->arena, config->dim, 
                                config->pq.nsub, 
                                config->pq.nbits, 
                                &pq);
                    hnsw->pq = pq;
                } else if (config->type == JW_INDEX_HNSW_SQ || config->quant == JW_QUANT_INT8 || config->quant == JW_QUANT_UINT8) {
                    /** 创建SQ量化器 */
                    hnsw->sq = jw_sq_quantizer_create(hnsw->arena, config->dim);
                }
            }
            index->impl = hnsw;
            break;
        }
        default:
            if (!arena) jw_arena_destroy(index->arena);
            return NULL;
    }

    if (index->impl == NULL) {
        if (!arena) jw_arena_destroy(index->arena);
        return NULL;
    }

    return index;
}

/**
 * 销毁索引
 *
 * @param index 索引指针
 */
JW_API void jw_index_destroy(jw_index_t *index)
{
    if (index == NULL) {
        return;
    }

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
            jw_ivf_destroy((jw_ivf_index_t *)index->impl);
            break;
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
            jw_hnsw_destroy((jw_hnsw_index_t *)index->impl);
            break;
        default:
            break;
    }

    /** 释放索引结构本身 (通过外部arena管理，这里只是通知) */
}

/**
 * 获取索引类型
 *
 * @param index 索引指针
 * @return 索引类型
 */
JW_API jw_index_type_t jw_index_get_type(const jw_index_t *index)
{
    return (index != NULL) ? index->type : JW_INDEX_FLAT;
}

/**
 * 获取索引维度
 *
 * @param index 索引指针
 * @return 向量维度，失败返回0
 */
JW_API jw_dim_t jw_index_get_dim(const jw_index_t *index)
{
    if (index == NULL || index->impl == NULL) {
        return 0;
    }

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ:
            return ((jw_ivf_index_t *)index->impl)->dim;
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ:
            return ((jw_hnsw_index_t *)index->impl)->dim;
        default:
            return 0;
    }
}

/**
 * 获取索引中的向量总数
 *
 * @param index 索引指针
 * @return 向量数量
 */
JW_API jw_size_t jw_index_get_ntotal(const jw_index_t *index)
{
    if (index == NULL || index->impl == NULL) {
        return 0;
    }

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ:
            return ((jw_ivf_index_t *)index->impl)->ntotal;
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ:
            return ((jw_hnsw_index_t *)index->impl)->ntotal;
        default:
            return 0;
    }
}

/*
 * =============================================================================
 * IVF索引实现
 * =============================================================================
 */

/**
 * 创建IVF索引
 *
 * @param arena   内存池
 * @param dim    向量维度
 * @param config IVF配置
 * @return IVF索引指针
 */
JW_API jw_ivf_index_t *jw_ivf_create(jw_arena_t *arena,
                                      jw_dim_t dim,
                                      const jw_ivf_config_t *config)
{
    if (dim == 0 || config == NULL || config->nlist == 0) {
        return NULL;
    }

    /** 创建本地内存池 */
    jw_arena_t *local_arena = arena;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
    }

    /** 分配IVF索引结构 */
    jw_ivf_index_t *index = (jw_ivf_index_t *)jw_arena_calloc(local_arena, 1,
                                 sizeof(jw_ivf_index_t));
    if (index == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }

    /** 初始化IVF索引字段 */
    index->type = JW_INDEX_IVF;
    index->dim = dim;
    index->metric = JW_METRIC_L2;
    index->config = *config;
    index->arena = local_arena;

    /* 防御：未显式赋值的 IVF 参数回退到安全默认 */
    if (index->config.nprobe == 0) {
        index->config.nprobe = 16;
    }
    if (index->config.max_iter == 0) {
        index->config.max_iter = 100;
    }
    if (index->config.seed == 0) {
        index->config.seed = 42;
    }
    index->nlist = config->nlist;
    index->trained = JW_FALSE;
    index->quant_type = JW_QUANT_INT8;  /* 默认使用int8量化 */
    index->pq = NULL;
    index->scales = NULL;

    /** 分配倒排列表数组 */
    index->lists = (jw_ivf_list_t *)jw_arena_calloc(local_arena,
                           config->nlist, sizeof(jw_ivf_list_t));
    if (index->lists == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }

    /** 初始化每个倒排列表 */
    for (jw_uint32_t i = 0; i < config->nlist; i++) {
        /** 分配聚类中心向量 */
        index->lists[i].centroid = (jw_vec_t)jw_arena_alloc(local_arena,
                                   dim * sizeof(jw_float32_t));
        if (index->lists[i].centroid == NULL) {
            if (!arena) jw_arena_destroy(local_arena);
            return NULL;
        }
        jw_memset(index->lists[i].centroid, 0, dim * sizeof(jw_float32_t));

        /** 初始化列表容量和条目数组 */
        index->lists[i].capacity = 1024;
        index->lists[i].entries = (jw_ivf_entry_t *)jw_arena_calloc(local_arena,
                                          index->lists[i].capacity,
                                          sizeof(jw_ivf_entry_t));
        index->lists[i].count = 0;

        /** 创建读写锁以支持并发查询 (NULL=使用malloc而非arena) */
        jw_rwlock_t *lock = NULL;
        jw_status_t lock_st = jw_rwlock_create(NULL, NULL, &lock);
        if (lock == NULL) {
            if (!arena) jw_arena_destroy(local_arena);
            return NULL;
        }
        index->lists[i].lock = lock;
    }

    /** 创建全局互斥锁 (NULL=使用malloc而非arena) */
    jw_mutex_t *mutex = NULL;
    jw_status_t mutex_st = jw_mutex_create(NULL, NULL, &mutex);
    if (mutex == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }
    index->lock = mutex;

    return index;
}

/**
 * 销毁IVF索引
 *
 * @param index IVF索引指针
 */
JW_API void jw_ivf_destroy(jw_ivf_index_t *index)
{
    if (index == NULL) {
        return;
    }

    /** 销毁每个倒排列表的锁 (由jw_malloc分配，需手动释放) */
    if (index->lists != NULL) {
        for (jw_uint32_t i = 0; i < index->nlist; i++) {
            if (index->lists[i].lock != NULL) {
                jw_rwlock_destroy(index->lists[i].lock);
                jw_free(index->lists[i].lock);
                index->lists[i].lock = NULL;
            }
        }
    }

    /** 销毁全局锁 (由jw_malloc分配，需手动释放) */
    if (index->lock != NULL) {
        jw_mutex_destroy(index->lock);
        jw_free(index->lock);
        index->lock = NULL;
    }

    /** 注意: 其余内存由外部内存池统一释放 */
}

/**
 * 训练索引 (K-means聚类)
 *
 * 对于IVF索引，需要先用训练数据执行K-means获得聚类中心
 * 训练后才能进行添加和搜索操作
 *
 * @param index   索引指针
 * @param vectors 训练向量数组
 * @param count   训练向量数量
 * @return        状态码
 */
JW_API jw_status_t jw_index_train(jw_index_t *index,
                                   jw_cvec_t vectors,
                                   jw_size_t count)
{
    if (index == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }

    /** IVF训练: 执行K-means聚类 */
    if (index->type == JW_INDEX_IVF || index->type == JW_INDEX_IVF_PQ || index->type == JW_INDEX_IVF_SQ) {
        jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;

        /** 训练数据量必须大于聚类数 */
        if (ivf == NULL || count < ivf->config.nlist) {
            return JW_INVALID_PARAM;
        }

        jw_dim_t dim = ivf->dim;
        jw_uint32_t nlist = ivf->config.nlist;

        /** 分配聚类中心数组 */
        jw_vec_t centroids = (jw_vec_t)jw_arena_alloc(index->arena,
                              nlist * dim * sizeof(jw_float32_t));
        if (centroids == NULL) {
            return JW_OUT_OF_MEMORY;
        }

        /** 执行K-means聚类 */
        jw_status_t status = kmeans_cluster(index->arena, vectors, count,
                                            dim, nlist,
                                            ivf->config.max_iter, ivf->metric,
                                            centroids);
        if (status != JW_SUCCESS) {
            return status;
        }

        /** 复制聚类中心到各个倒排列表 */
        for (jw_uint32_t i = 0; i < nlist; i++) {
            jw_memcpy(ivf->lists[i].centroid, centroids + i * dim,
                      dim * sizeof(jw_float32_t));
        }

        /** 训练PQ量化器 (传入度量, 使码本按度量训练) */
        if (index->type == JW_INDEX_IVF_PQ && ivf->pq != NULL) {
            status = jw_pq_train(ivf->pq, vectors, count, ivf->metric);
            if (status != JW_SUCCESS) {
                return status;
            }
        } else if ((index->type == JW_INDEX_IVF_SQ || ivf->quant_type == JW_QUANT_INT8 || ivf->quant_type == JW_QUANT_UINT8) && ivf->sq != NULL) {
            /** 训练SQ量化器 */
            status = jw_sq_quantizer_train(ivf->sq, vectors, count);
            if (status != JW_SUCCESS) {
                return status;
            }
        }

        ivf->trained = JW_TRUE;
        return JW_SUCCESS;
    }

    /** HNSW_PQ: 需要训练PQ码本, 否则码本全零导致量化完全失效 */
    if (index->type == JW_INDEX_HNSW_PQ) {
        jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
        if (hnsw != NULL && hnsw->pq != NULL) {
            jw_status_t st = jw_pq_train(hnsw->pq, vectors, count, hnsw->metric);
            if (st != JW_SUCCESS) {
                return st;
            }
        }
    }

    /** HNSW不需要训练 (或使用随机投影树) */
    return JW_SUCCESS;
}

/**
 * 检查索引是否已训练
 *
 * @param index 索引指针
 * @return JW_TRUE 如果已训练
 */
JW_API jw_bool_t jw_index_is_trained(const jw_index_t *index)
{
    if (index == NULL || index->impl == NULL) {
        return JW_FALSE;
    }

    if (index->type == JW_INDEX_IVF || index->type == JW_INDEX_IVF_PQ || index->type == JW_INDEX_IVF_SQ) {
        return ((jw_ivf_index_t *)index->impl)->trained;
    }

    /** HNSW认为总是已训练 */
    return JW_TRUE;
}

/**
 * 添加单个向量到索引
 *
 * @param index 索引指针
 * @param vid   向量ID
 * @param vec   向量数据
 * @return      状态码
 */
JW_API jw_status_t jw_index_add(jw_index_t *index,
                                 jw_vid_t vid,
                                 jw_cvec_t vec)
{
    return jw_index_add_batch(index, &vid, vec, 1);
}

/**
 * 批量添加向量到索引
 *
 * @param index   索引指针
 * @param vids    向量ID数组
 * @param vectors 向量数据数组 (连续存储)
 * @param count   向量数量
 * @return        状态码
 */
JW_API jw_status_t jw_index_add_batch(jw_index_t *index,
                                       const jw_vid_t *vids,
                                       jw_cvec_t vectors,
                                       jw_size_t count)
{
    if (index == NULL || vids == NULL || vectors == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }

    jw_dim_t dim = jw_index_get_dim(index);

    /** IVF添加向量 */
    if (index->type == JW_INDEX_IVF || index->type == JW_INDEX_IVF_PQ || index->type == JW_INDEX_IVF_SQ) {
        jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;

        /** 检查是否已训练 */
        if (!ivf->trained) {
            return JW_INDEX_NOT_READY;
        }

        jw_mutex_lock(ivf->lock);

        /** 为每个向量找到最近的聚类中心并添加到对应列表 */
        for (jw_size_t i = 0; i < count; i++) {
            jw_cvec_t vec = vectors + i * dim;

            /** 找到最近的聚类中心 */
            jw_float32_t min_dist = JW_FLT_MAX;
            jw_uint32_t min_idx = 0;

            for (jw_uint32_t j = 0; j < ivf->nlist; j++) {
                jw_float32_t dist = jw_vec_distance(vec, ivf->lists[j].centroid, dim, ivf->metric);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_idx = j;
                }
            }

            /** 添加到对应列表 */
            if (min_idx >= ivf->nlist) {
                jw_mutex_unlock(ivf->lock);
                return JW_INVALID_PARAM;
            }
            jw_ivf_list_t *list = &ivf->lists[min_idx];
            if (list->lock == NULL) {
                jw_mutex_unlock(ivf->lock);
                return JW_UNKNOWN_ERROR;
            }
            jw_rwlock_wrlock(list->lock);

            /** 扩容检查 */
            if (list->count >= list->capacity) {
                jw_size_t new_capacity = list->capacity * 2;
                jw_ivf_entry_t *new_entries = (jw_ivf_entry_t *)jw_arena_calloc(
                        index->arena, new_capacity, sizeof(jw_ivf_entry_t));
                if (new_entries == NULL) {
                    jw_rwlock_wrunlock(list->lock);
                    jw_mutex_unlock(ivf->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_memcpy(new_entries, list->entries,
                          list->count * sizeof(jw_ivf_entry_t));
                list->entries = new_entries;
                list->capacity = new_capacity;
            }
            
            /** 复制向量数据或量化编码 */
            list->entries[list->count].vid = vids[i];
            
            if (ivf->type == JW_INDEX_IVF_PQ && ivf->pq != NULL) {
                /** 使用PQ量化 */
                list->entries[list->count].vec = NULL;
                list->entries[list->count].code = (jw_uint8_t *)jw_arena_alloc(
                        index->arena, ivf->pq->nsub * sizeof(jw_uint8_t));
                if (list->entries[list->count].code == NULL) {
                    jw_rwlock_wrunlock(list->lock);
                    jw_mutex_unlock(ivf->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_pq_encode(ivf->pq, vec, list->entries[list->count].code);
            } else if (ivf->type == JW_INDEX_IVF_SQ && ivf->sq != NULL) {
                /** 使用SQ量化 */
                list->entries[list->count].vec = NULL;
                list->entries[list->count].code = (jw_uint8_t *)jw_arena_alloc(
                        index->arena, dim * sizeof(jw_uint8_t));
                if (list->entries[list->count].code == NULL) {
                    jw_rwlock_wrunlock(list->lock);
                    jw_mutex_unlock(ivf->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_sq_quantize(ivf->sq, vec, list->entries[list->count].code);
            } else {
                /** 存储原始向量 */
                list->entries[list->count].vec = (jw_vec_t)jw_arena_alloc(
                        index->arena, dim * sizeof(jw_float32_t));
                if (list->entries[list->count].vec == NULL) {
                    jw_rwlock_wrunlock(list->lock);
                    jw_mutex_unlock(ivf->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_memcpy(list->entries[list->count].vec, vec,
                          dim * sizeof(jw_float32_t));
                list->entries[list->count].code = NULL;
            }
            list->count++;

            jw_rwlock_wrunlock(list->lock);
        }

        ivf->ntotal += count;
        jw_mutex_unlock(ivf->lock);

        return JW_SUCCESS;
    }

    /** HNSW添加向量 */
    if (index->type == JW_INDEX_HNSW || index->type == JW_INDEX_HNSW_PQ) {
        jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;

        jw_rwlock_wrlock(hnsw->lock);

        /**
         * 预扫描本批次的最大 vid, 一次性分配足够大的标记数组。
         * 必须覆盖扩容后的 capacity (扩容公式为 (vid+1)*2), 否则
         * visit_mark[neighbor] 会在扩容后越界写。
         */
        jw_vid_t batch_max_vid = 0;
        for (jw_size_t i = 0; i < count; i++) {
            if (vids[i] > batch_max_vid) {
                batch_max_vid = vids[i];
            }
        }
        jw_size_t mark_cap = ((jw_size_t)batch_max_vid + 1) * 2;
        if (mark_cap < hnsw->capacity) {
            mark_cap = hnsw->capacity;
        }

        /** 预分配批量插入所需的临时数组（只需分配一次） */
        jw_uint8_t *insert_visit_mark = (jw_uint8_t *)jw_arena_alloc(index->arena,
                                     mark_cap);
        jw_uint32_t ef_constr = hnsw->config.ef_construction;
        search_candidate_t *insert_layer_results = (search_candidate_t *)jw_arena_alloc(
                index->arena, ef_constr * sizeof(search_candidate_t));

        if (insert_visit_mark == NULL || insert_layer_results == NULL) {
            jw_rwlock_wrunlock(hnsw->lock);
            return JW_OUT_OF_MEMORY;
        }

        for (jw_size_t i = 0; i < count; i++) {
            jw_vid_t vid = vids[i];
            jw_cvec_t vec = vectors + i * dim;

            /** 扩容检查 */
            if (vid >= hnsw->capacity) {
                jw_size_t new_capacity = (vid + 1) * 2;
                jw_hnsw_node_t **new_nodes = (jw_hnsw_node_t **)jw_arena_calloc(
                        index->arena, new_capacity, sizeof(jw_hnsw_node_t *));
                if (new_nodes == NULL) {
                    jw_rwlock_wrunlock(hnsw->lock);
                    return JW_OUT_OF_MEMORY;
                }
                if (hnsw->nodes != NULL) {
                    jw_memcpy(new_nodes, hnsw->nodes,
                              hnsw->capacity * sizeof(jw_hnsw_node_t *));
                }
                hnsw->nodes = new_nodes;
                hnsw->capacity = new_capacity;
            }

            /** 生成随机层级 */
            jw_uint32_t node_level = hnsw_generate_level(&hnsw->config);

            /** 创建节点 */
            jw_hnsw_node_t *node = (jw_hnsw_node_t *)jw_arena_calloc(
                    index->arena, 1, sizeof(jw_hnsw_node_t));
            if (node == NULL) {
                jw_rwlock_wrunlock(hnsw->lock);
                return JW_OUT_OF_MEMORY;
            }

            node->vid = vid;
            if (hnsw->pq != NULL) {
                /** HNSW_PQ: 只存 PQ 编码, 不存原始向量 (与 IVF_PQ 一致, 省内存) */
                node->code = (jw_uint8_t *)jw_arena_alloc(index->arena,
                                hnsw->pq->nsub * sizeof(jw_uint8_t));
                if (node->code == NULL) {
                    jw_rwlock_wrunlock(hnsw->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_pq_encode(hnsw->pq, vec, node->code);
            } else {
                node->vec = (jw_vec_t)jw_arena_alloc(index->arena,
                               dim * sizeof(jw_float32_t));
                if (node->vec == NULL) {
                    jw_rwlock_wrunlock(hnsw->lock);
                    return JW_OUT_OF_MEMORY;
                }
                jw_memcpy(node->vec, vec, dim * sizeof(jw_float32_t));
            }
            node->level = node_level;
            node->max_level = node_level;
            node->max_M = hnsw->config.M;
            node->max_M0 = hnsw->config.M * 2;  /** 第0层允许更多连接 */

            /** 分配连接表内存 (任一失败都必须解锁返回, 否则后续写 NULL 崩溃) */
            node->links = (jw_vid_t **)jw_arena_alloc(index->arena,
                          (node_level + 1) * sizeof(jw_vid_t *));
            node->link_counts = (jw_uint32_t *)jw_arena_alloc(index->arena,
                                (node_level + 1) * sizeof(jw_uint32_t));
            if (node->links == NULL || node->link_counts == NULL) {
                jw_rwlock_wrunlock(hnsw->lock);
                return JW_OUT_OF_MEMORY;
            }
            int links_alloc_ok = 1;
            for (jw_uint32_t l = 0; l <= node_level; l++) {
                /* 第0层用 max_M0，其他层用 max_M */
                jw_uint32_t alloc_M = (l == 0) ? node->max_M0 : node->max_M;
                node->links[l] = (jw_vid_t *)jw_arena_alloc(index->arena,
                            alloc_M * sizeof(jw_vid_t));
                if (node->links[l] == NULL) {
                    links_alloc_ok = 0;
                    /* 已分配的层保持有效, 未分配的置 NULL, 供统一释放路径判断 */
                    for (jw_uint32_t l2 = l + 1; l2 <= node_level; l2++) {
                        node->links[l2] = NULL;
                    }
                    break;
                }
                node->link_counts[l] = 0;
            }
            if (!links_alloc_ok) {
                jw_rwlock_wrunlock(hnsw->lock);
                return JW_OUT_OF_MEMORY;
            }

            hnsw->nodes[vid] = node;
            hnsw->ntotal++;

            /** 创建后验证：确认数据指针和 links 都是合法堆地址 */
            if (hnsw->pq == NULL && !is_valid_heap_ptr(node->vec)) {
                JW_LOGE("add_batch: NODE CREATED WITH GARBAGE VEC!"
                    " vid=%" PRIu64 " node=%p vec=%p level=%u max_M=%u",
                    (uint64_t)vid, (void*)node, (void*)node->vec,
                    (unsigned)node->level, (unsigned)node->max_M);
            }
            if (hnsw->pq != NULL && node->code == NULL) {
                JW_LOGE("add_batch: NODE CREATED WITH NULL PQ CODE!"
                    " vid=%" PRIu64 " node=%p code=%p level=%u max_M=%u",
                    (uint64_t)vid, (void*)node, (void*)node->code,
                    (unsigned)node->level, (unsigned)node->max_M);
            }
            if (!is_valid_heap_ptr((const void*)node->links)) {
                JW_LOGE("add_batch: NODE CREATED WITH GARBAGE LINKS!"
                    " vid=%" PRIu64 " node=%p links=%p",
                    (uint64_t)vid, (void*)node, (void*)node->links);
            }

            /**
             * HNSW插入算法:
             * 1. 从当前最大层级开始搜索，找到每层的最近邻
             * 2. 在每层添加双向连接边
             * 3. 更新入口点和最大层级
             */
            if (hnsw->entry_point == ((jw_vid_t)-1) && hnsw->ntotal == 1) {
                /** 第一个节点：直接设为入口点就好了 */
                hnsw->entry_point = vid;
                hnsw->max_level = node_level;
            } else {
                /** 非首个节点：执行完整的 HNSW 插入 */
                jw_vid_t current_ep = hnsw->entry_point;
                jw_uint32_t current_max_level = hnsw->max_level;

                /**
                 * 每个向量都要重置已访问标记。
                 * 原实现是 visited_count = 0 (每向量重置), 语义必须保持一致,
                 * 否则后插入的向量会跳过前面的向量访问过的节点, 随规模累积劣化图质量。
                 */
                jw_memset(insert_visit_mark, 0, mark_cap);

                /**
                 * 第一阶段: 从顶层向下贪婪搜索
                 * 对于高于新节点层级的层，只需找到向下传递的入口点
                 */
                for (jw_uint32_t l = current_max_level; l > node_level; l--) {
                    current_ep = hnsw_greedy_search(hnsw, vec, current_ep, l,
                                                    insert_visit_mark,
                                                    index->arena);
                }

                /**
                 * 第二阶段: 在新节点所在层级及以下建立连接
                 * 从 min(node_level, current_max_level) 向下搜索到第0层
                 */
                jw_uint32_t conn_start = (node_level < current_max_level) ?
                                          node_level : current_max_level;

                for (jw_int32_t l = (jw_int32_t)conn_start; l >= 0; l--) {
                    jw_uint32_t level = (jw_uint32_t)l;
                    jw_size_t result_count = 0;

                    /** 在第level层搜索 ef_construction 个候选 */
                    hnsw_layer_search(hnsw, vec, current_ep, level,
                                     ef_constr, insert_layer_results,
                                     &result_count, index->arena);

                    /**
                     * 若层搜索没找到足够候选（图还太小或太稀疏），
                     * 暴力扫描所有已有节点，选出最近的M个建立连接
                     */
                    if (result_count == 0) {
                        jw_uint32_t max_links = (level == 0) ?
                                               node->max_M0 : node->max_M;

                        /** 收集该层可达的所有已有节点 (跳过已删除的) */
                        jw_size_t existing_count = 0;
                        for (jw_size_t n = 0; n < hnsw->capacity; n++) {
                            if (hnsw->nodes[n] != NULL
                                && !hnsw->nodes[n]->deleted
                                && hnsw_node_usable(hnsw, hnsw->nodes[n])
                                && hnsw->nodes[n]->vid != vid
                                && level <= hnsw->nodes[n]->level) {
                                existing_count++;
                            }
                        }

                        if (existing_count == 0) {
                            continue;
                        }

                        jw_hnsw_node_t **all_candidates = (jw_hnsw_node_t **)jw_arena_alloc(
                                index->arena,
                                existing_count * sizeof(jw_hnsw_node_t *));
                        jw_size_t ac = 0;
                        for (jw_size_t n = 0; n < hnsw->capacity; n++) {
                            if (hnsw->nodes[n] != NULL
                                && !hnsw->nodes[n]->deleted
                                && hnsw_node_usable(hnsw, hnsw->nodes[n])
                                && hnsw->nodes[n]->vid != vid
                                && level <= hnsw->nodes[n]->level) {
                                all_candidates[ac++] = hnsw->nodes[n];
                            }
                        }

                        hnsw_select_neighbors(all_candidates,
                                             (jw_uint32_t)existing_count,
                                             max_links,
                                             hnsw, vec, level, index->arena);

                        jw_uint32_t link_count = (existing_count < (jw_size_t)max_links) ?
                                                (jw_uint32_t)existing_count : max_links;
                        for (jw_uint32_t c = 0; c < link_count; c++) {
                            jw_hnsw_node_t *nb = all_candidates[c];
                            /* 防御: 候选节点层级可能低于当前 level, 其 link_counts
                             * 数组只分配到 nb->level+1, 越界写会破坏相邻内存 */
                            if (level > nb->level) {
                                continue;
                            }
                            jw_uint32_t nb_max = (level == 0) ?
                                                nb->max_M0 : nb->max_M;

                            if (node->link_counts[level] < max_links) {
                                node->links[level][node->link_counts[level]++] = nb->vid;
                            }
                            if (nb->link_counts[level] < nb_max) {
                                nb->links[level][nb->link_counts[level]++] = vid;
                            }
                        }
                        continue;
                    }

                    /** 把搜索结果转成节点指针数组 */
                    jw_hnsw_node_t **candidates = (jw_hnsw_node_t **)jw_arena_alloc(
                            index->arena,
                            result_count * sizeof(jw_hnsw_node_t *));
                    jw_uint32_t valid_count = 0;
                    for (jw_size_t c = 0; c < result_count; c++) {
                        jw_vid_t cvid = insert_layer_results[c].vid;
                        if (cvid < hnsw->capacity && hnsw->nodes[cvid] != NULL
                            && hnsw_node_usable(hnsw, hnsw->nodes[cvid])) {
                            candidates[valid_count++] = hnsw->nodes[cvid];
                        }
                    }

                    if (valid_count == 0) {
                        continue;
                    }

                    /** 选择最近的M个邻居（第0层用M0=2*M） */
                    jw_uint32_t max_links = (level == 0) ?
                                           node->max_M0 : node->max_M;
                    hnsw_select_neighbors(candidates, valid_count, max_links,
                                         hnsw, vec, level, index->arena);

                    /** 建立双向连接 */
                    jw_uint32_t link_count = (valid_count < max_links) ?
                                            valid_count : max_links;
                    for (jw_size_t c = 0; c < link_count; c++) {
                        jw_hnsw_node_t *neighbor = candidates[c];
                        jw_vid_t nvid = neighbor->vid;

                        /* 防御: 候选节点层级可能低于当前 level, 其 link_counts
                         * 数组只分配到 neighbor->level+1, 越界写会破坏相邻内存 */
                        if (level > neighbor->level) {
                            continue;
                        }

                        /** node -> neighbor */
                        if (node->link_counts[level] < max_links) {
                            node->links[level][node->link_counts[level]++] = nvid;
                        }

                        /** neighbor -> node */
                        jw_uint32_t neigh_max = (level == 0) ?
                                               neighbor->max_M0 : neighbor->max_M;
                        if (neighbor->link_counts[level] < neigh_max) {
                            neighbor->links[level][neighbor->link_counts[level]++] = vid;
                        } else {
                            /** 邻居的连接已满，尝试替换最远的连接 */
                            jw_float32_t max_dist = -1.0f;
                            jw_uint32_t max_idx = 0;
                            int found = 0;
                            for (jw_uint32_t k = 0; k < neighbor->link_counts[level]; k++) {
                                jw_vid_t nv = neighbor->links[level][k];
                                if (nv < hnsw->capacity && hnsw->nodes[nv] != NULL
                                    && hnsw_node_usable(hnsw, hnsw->nodes[nv])) {
                                    /**
                                     * 站在"邻居自身"视角, 比较它的各条现有连接
                                     * 中谁离它最远, 以便被新连接替换。
                                     *
                                     * 修复 (2026-10-22): 原先此处用新插入向量 vec
                                     * 作查询 (等于问"新节点离 nv 近、还是 nv 离
                                     * 新节点近"), 判据错乱使替换决策随机化, 破坏
                                     * 图结构。实测 ef_construction 越大越容易误
                                     * 替换, 表现为 recall 随 ef 增大而**下降**
                                     * (M=32: ef=32 时 recall@5=100%, ef=256 时 52%)。
                                     */
                                    jw_float32_t d = hnsw_node_pair_dist(hnsw, neighbor,
                                                    hnsw->nodes[nv]);
                                    if (!found || d > max_dist) {
                                        max_dist = d;
                                        max_idx = k;
                                        found = 1;
                                    }
                                }
                            }
                            jw_float32_t new_dist = hnsw_node_pair_dist(hnsw, neighbor, node);
                            if (found && new_dist < max_dist) {
                                neighbor->links[level][max_idx] = vid;
                            }
                        }
                    }

                    /** 为下一层搜索更新入口点 */
                    if (result_count > 0 && result_count <= ef_constr) {
                        current_ep = insert_layer_results[0].vid;
                    }
                }

                /** 第三阶段: 如果新节点层级更高，更新全局入口点 */
                if (node_level > hnsw->max_level) {
                    hnsw->entry_point = vid;
                    hnsw->max_level = node_level;
                }
            }

            /**
             * 性能修复 (2026-10-21): 原先此处调用 watch_all_nodes_vec() 逐节点
             * 扫描全部 capacity, 使插入复杂度退化为 O(N^2) (实测 N=20000 仅
             * 143 vec/s)。该函数是早期排查 vec 被破坏问题时的 GDB watchpoint
             * 模拟, 问题早已定位, 且它对 PQ 模式误报 (所有节点 vec==NULL 会被
             * 判为"损坏")。故从热路径移除, 函数一并删除。
             */
        }

        jw_rwlock_wrunlock(hnsw->lock);
        return JW_SUCCESS;
    }

    return JW_NOT_SUPPORTED;
}

/**
 * 从索引中删除向量
 *
 * @param index 索引指针
 * @param vid   向量ID
 * @return      状态码
 */
JW_API jw_status_t jw_index_remove(jw_index_t *index, jw_vid_t vid)
{
    if (index == NULL) {
        return JW_INVALID_PARAM;
    }

    if (index->type == JW_INDEX_HNSW) {
        jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
        jw_rwlock_wrlock(hnsw->lock);

        /* 未找到时返回 JW_NOT_FOUND (与 IVF 分支一致)。
         * 修复 (2026-10-23): 原先无条件 return JW_SUCCESS, 删除不存在的
         * vid 也报成功, 调用方无法区分"删掉了"和"本来就没有"。 */
        jw_status_t ret = JW_NOT_FOUND;

        if (vid < hnsw->capacity && hnsw->nodes[vid] != NULL) {
            /** 直接置空节点的指针, 搜索时会跳过 NULL 节点 */
            hnsw->nodes[vid] = NULL;
            /** 同步递减计数, 否则 jw_index_get_count 会虚高 */
            if (hnsw->ntotal > 0) {
                hnsw->ntotal--;
            }

            /** 如果删除的是入口点, 找新的 */
            if (hnsw->entry_point == vid) {
                hnsw->entry_point = ((jw_vid_t)-1);
                hnsw->max_level = 0;
                for (jw_size_t n = 0; n < hnsw->capacity; n++) {
                    if (hnsw->nodes[n] != NULL) {
                        hnsw->entry_point = hnsw->nodes[n]->vid;
                        hnsw->max_level = hnsw->nodes[n]->level;
                        break;
                    }
                }
            }
            ret = JW_SUCCESS;
        }

        jw_rwlock_wrunlock(hnsw->lock);
        return ret;
    }

    if (index->type == JW_INDEX_IVF || index->type == JW_INDEX_IVF_PQ || index->type == JW_INDEX_IVF_SQ) {
        /**
         * IVF 删除 (2026-10-23 实现, 修复空实现缺陷)
         *
         * 原先是空实现 (直接 return JW_SUCCESS), 导致:
         *   - ntotal 不递减, jw_index_get_count 虚高
         *   - 已删向量仍留在倒排列表里, 搜索继续返回它
         *   - jw_collection_search 无兜底过滤, 上层直接拿到已删数据
         *
         * 实现: 遍历各倒排列表, 找到 vid 后用 memmove 前移移除,
         * 同步递减 list->count 与 ivf->ntotal。
         * 持 list->lock 保证与并发搜索互斥。
         */
        jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
        if (ivf == NULL) {
            return JW_INVALID_PARAM;
        }

        jw_mutex_lock(ivf->lock);

        jw_bool_t found = JW_FALSE;
        for (jw_uint32_t li = 0; li < ivf->nlist && !found; li++) {
            jw_ivf_list_t *list = &ivf->lists[li];
            if (list->entries == NULL || list->count == 0) {
                continue;
            }

            jw_rwlock_wrlock(list->lock);
            for (jw_size_t j = 0; j < list->count; j++) {
                if (list->entries[j].vid == vid) {
                    /* 前移移除该条目 (保持数组紧凑) */
                    if (j + 1 < list->count) {
                        jw_memmove(&list->entries[j],
                                   &list->entries[j + 1],
                                   (list->count - j - 1) * sizeof(jw_ivf_entry_t));
                    }
                    list->count--;
                    if (ivf->ntotal > 0) {
                        ivf->ntotal--;
                    }
                    found = JW_TRUE;
                    break;
                }
            }
            jw_rwlock_wrunlock(list->lock);
        }

        jw_mutex_unlock(ivf->lock);
        return found ? JW_SUCCESS : JW_NOT_FOUND;
    }

    return JW_SUCCESS;
}

/**
 * 批量删除向量
 *
 * @param index 索引指针
 * @param vids  向量ID数组
 * @param count 向量数量
 * @return      状态码
 */
JW_API jw_status_t jw_index_remove_batch(jw_index_t *index,
                                          const jw_vid_t *vids,
                                          jw_size_t count)
{
    if (index == NULL || vids == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }

    for (jw_size_t i = 0; i < count; i++) {
        jw_status_t status = jw_index_remove(index, vids[i]);
        if (status != JW_SUCCESS) {
            return status;
        }
    }

    return JW_SUCCESS;
}

/*
 * =============================================================================
 * KNN搜索接口实现
 * =============================================================================
 */

/**
 * 搜索最近邻
 *
 * @param index   索引指针
 * @param query   查询向量
 * @param k       返回结果数量
 * @param results 输出: 搜索结果数组
 * @return        实际返回的结果数量
 */
JW_API jw_size_t jw_index_search(const jw_index_t *index,
                                  jw_cvec_t query,
                                  jw_size_t k,
                                  jw_search_result_t *results)
{
    if (index == NULL || query == NULL || results == NULL || k == 0) {
        return 0;
    }

    jw_dim_t dim = jw_index_get_dim(index);
    jw_size_t ntotal = jw_index_get_ntotal(index);

    if (ntotal == 0) {
        return 0;
    }

    /**
     * IVF搜索算法:
     * 1. 计算查询向量到所有聚类中心的距离
     * 2. 选择最近的nprobe个聚类中心
     * 3. 在这些聚类中心对应的列表中搜索最近邻
     */
    if (index->type == JW_INDEX_IVF || index->type == JW_INDEX_IVF_PQ || index->type == JW_INDEX_IVF_SQ) {
        jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;

        if (!ivf->trained) {
            return 0;
        }

        jw_uint32_t nprobe = ivf->config.nprobe;
        if (nprobe > ivf->nlist) {
            nprobe = ivf->nlist;
        }

        /** 计算到各聚类中心的距离 */
        typedef struct {
            jw_uint32_t idx;
            jw_float32_t dist;
        } centroid_dist_t;

        centroid_dist_t *centroids = (centroid_dist_t *)jw_malloc(
                ivf->nlist * sizeof(centroid_dist_t));
        if (centroids == NULL) {
            return 0;
        }

        for (jw_uint32_t i = 0; i < ivf->nlist; i++) {
            centroids[i].idx = i;
            centroids[i].dist = jw_vec_distance(query, ivf->lists[i].centroid, dim, ivf->metric);
        }

        /** 选择最近的nprobe个聚类中心 (简单选择排序) */
        for (jw_uint32_t i = 0; i < nprobe; i++) {
            jw_uint32_t min_j = i;
            for (jw_uint32_t j = i + 1; j < ivf->nlist; j++) {
                if (centroids[j].dist < centroids[min_j].dist) {
                    min_j = j;
                }
            }
            centroid_dist_t tmp = centroids[i];
            centroids[i] = centroids[min_j];
            centroids[min_j] = tmp;
        }

        /** 在最近的列表中搜索 */
        priority_queue_t pq;
        pq_init(&pq, k, index->arena);
        if (pq.items == NULL) {
            jw_free(centroids);
            return 0;
        }

        for (jw_uint32_t i = 0; i < nprobe; i++) {
            jw_ivf_list_t *list = &ivf->lists[centroids[i].idx];

            jw_rwlock_rdlock(list->lock);

            /** 遍历列表中的所有向量 */
            for (jw_size_t j = 0; j < list->count; j++) {
                jw_float32_t dist;
                jw_ivf_entry_t *entry = &list->entries[j];
                if (entry->code != NULL && ivf->pq != NULL
                    && ivf->type == JW_INDEX_IVF_PQ) {
                /** 使用PQ距离计算 */
                dist = jw_pq_distance(ivf->pq, query, entry->code, ivf->metric);
                } else if (entry->code != NULL && ivf->sq != NULL
                           && ivf->type == JW_INDEX_IVF_SQ) {
                    /**
                     * 使用SQ量化距离计算 (int8/uint8 标量量化)
                     * 修复 (2026-10-22): 原先此分支缺失, IVF_SQ 的 entry
                     * 只有 code 量化字节、vec 为 NULL, 于是落入下面的 else
                     * 把 NULL 传给 jw_vec_distance -> 段错误。
                     * 与 IVF_PQ 的同类缺陷同源, 实测复现:
                     * test_quantization_serialization_mmap 段错误。
                     *
                     * 注意 jw_sq_distance(quant, qvec=量化码, fvec=原始查询, metric)
                     */
                    dist = jw_sq_distance(ivf->sq, entry->code, query, ivf->metric);
                } else if (entry->vec != NULL) {
                    /** 使用原始距离计算 */
                    dist = jw_vec_distance(query, entry->vec, dim, ivf->metric);
                } else {
                    /** 数据缺失 (如反序列化失败), 跳过而非解引用 NULL */
                    continue;
                }
                pq_push(&pq, list->entries[j].vid, dist, 0);
            }

            jw_rwlock_rdunlock(list->lock);
        }

        /** 复制结果 */
        jw_size_t result_count = (pq.size < k) ? pq.size : k;
        for (jw_size_t i = 0; i < result_count; i++) {
            results[i].id = pq.items[i].vid;
            results[i].score = pq.items[i].score;
        }

        jw_free(centroids);

        /** 按距离排序 */
        jw_qsort_simple(results, result_count,
                        sizeof(jw_search_result_t), compare_results);

        /** 将内部\"距离\"转换为与 HNSW 一致的相似度语义 (单调变换, 不改变排序):
         *   COSINE -> 余弦相似度 (1 - 余弦距离)
         *   IP     -> 内积相似度 (距离 = -内积)
         *   L2     -> 保持原有距离 (内部已是平方/开方距离, 与 HNSW 一致) */
        for (jw_size_t i = 0; i < result_count; i++) {
            jw_float32_t sc = results[i].score;
            if (ivf->metric == JW_METRIC_COSINE) {
                sc = 1.0f - sc;
            } else if (ivf->metric == JW_METRIC_IP) {
                sc = -sc;
            }
            results[i].score = sc;
        }

        return result_count;
    }

    /**
     * HNSW搜索算法:
     * 1. 从顶层入口点开始
     * 2. 每层使用贪婪搜索找到该层最近邻
     * 3. 到达第0层后使用Best-First搜索收集候选
     * 4. 返回最近的k个结果
     */
    if (index->type == JW_INDEX_HNSW || index->type == JW_INDEX_HNSW_PQ) {
        jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;

        if (hnsw->entry_point == ((jw_vid_t)-1) || hnsw->ntotal == 0) {
            return 0;
        }

        jw_rwlock_rdlock(hnsw->lock);

        /** 获取配置参数 */
        jw_uint32_t ef_search = hnsw->config.ef_search;
        if (ef_search < k) {
            ef_search = (jw_uint32_t)k;
        }

        /** 分配临时数组 */
        search_candidate_t *layer_results = (search_candidate_t *)jw_malloc(
                hnsw->capacity * sizeof(search_candidate_t));
        jw_uint8_t *visit_mark = NULL;

        if (layer_results != NULL) {
            visit_mark = (jw_uint8_t *)jw_malloc(hnsw->capacity);
        }

        if (layer_results == NULL || visit_mark == NULL) {
            if (layer_results != NULL) {
                jw_free(layer_results);
            }
            if (visit_mark != NULL) {
                jw_free(visit_mark);
            }
            jw_rwlock_rdunlock(hnsw->lock);
            return 0;
        }
        jw_memset(visit_mark, 0, hnsw->capacity);

        jw_vid_t current_ep = hnsw->entry_point;
        jw_uint32_t max_level = (hnsw->max_level > 0) ? hnsw->max_level : 0;

        /** 如果入口点无效或已删除，寻找新的有效入口点 */
        if (current_ep == ((jw_vid_t)-1)
            || current_ep >= hnsw->capacity
            || hnsw->nodes[current_ep] == NULL
            || hnsw->nodes[current_ep]->deleted
            || !hnsw_node_usable(hnsw, hnsw->nodes[current_ep])) {
            jw_vid_t new_ep = ((jw_vid_t)-1);
            for (jw_size_t n = 0; n < hnsw->capacity; n++) {
                if (hnsw->nodes[n] != NULL && !hnsw->nodes[n]->deleted
                    && hnsw_node_usable(hnsw, hnsw->nodes[n])) {
                    new_ep = hnsw->nodes[n]->vid;
                    hnsw->max_level = hnsw->nodes[n]->level;
                    break;
                }
            }
            if (new_ep == ((jw_vid_t)-1)) {
                /** 没有有效节点，返回空 */
                jw_free(layer_results);
                jw_free(visit_mark);
                jw_rwlock_rdunlock(hnsw->lock);
                return 0;
            }
            current_ep = new_ep;
            max_level = hnsw->max_level;
        }

        /**
         * 第一阶段: 从顶层向下进行贪婪搜索
         * 每层找到该层的最近邻作为下一层的入口点
         */
        for (jw_uint32_t level = max_level; level > 0; level--) {
            current_ep = hnsw_greedy_search(hnsw, query, current_ep,
                                            level, visit_mark,
                                            index->arena);
        }

        /**
         * 第二阶段: 在第0层使用Best-First搜索
         * 收集ef_search个最近邻
         */
        jw_size_t result_count = 0;
        hnsw_layer_search(hnsw, query, current_ep, 0, ef_search,
                          layer_results, &result_count, index->arena);

        /** 对完整结果集排序后取前 k 个。
         *  layer_results 是堆序(非有序), 不能直接取前缀; 否则 k>1 时结果错误。 */
        jw_qsort_simple(layer_results, result_count,
                        sizeof(search_candidate_t), compare_candidates);
        jw_size_t output_count = (result_count < k) ? result_count : k;
        for (jw_size_t i = 0; i < output_count; i++) {
            /* 暂存内部距离, 相似度转换延后到最终排序之后 */
            /* 将内部\"距离\"转换回\"相似度\"语义:
             *   COSINE -> 余弦相似度 (1 - 余弦距离)
             *   IP     -> 内积相似度 (距离 = -内积)
             *   L2     -> 保持平方欧氏距离 (距离越小越近) */
            results[i].id = layer_results[i].vid;
            results[i].score = layer_results[i].score;  /** 暂存内部距离 */
        }

        jw_free(layer_results);
        jw_free(visit_mark);

        jw_rwlock_rdunlock(hnsw->lock);

        /** 按内部距离升序排最终结果 (compare_results 按 score 升序, 对所有度量一致正确) */
        jw_qsort_simple(results, output_count,
                        sizeof(jw_search_result_t), compare_results);

        /** 将内部"距离"转换回"相似度"语义 (单调变换, 不改变已排顺序):
         *   COSINE -> 余弦相似度 (1 - 余弦距离)
         *   IP     -> 内积相似度 (距离 = -内积)
         *   L2     -> 保持平方欧氏距离 (距离越小越近) */
        for (jw_size_t i = 0; i < output_count; i++) {
            jw_float32_t sc = results[i].score;
            if (hnsw->metric == JW_METRIC_COSINE) {
                sc = 1.0f - sc;  /**< 余弦相似度 */
            } else if (hnsw->metric == JW_METRIC_IP) {
                sc = -sc;         /**< 内积相似度 */
            }
            /* L2: 内部距离 jw_vec_distance 已是平方距离, 直接作为 score, 保持兼容 */
            results[i].score = sc;
        }

        return output_count;
    }

    return 0;
}

/**
 * 范围搜索 (半径检索)
 *
 * 返回所有与查询距离在 radius 以内的向量, 结果按距离升序排列。
 *
 * 实现策略: 按索引类型分派
 *   - IVF/IVF_PQ/IVF_SQ: 先按 nprobe 选簇心, 再在候选簇内逐条算精确距离
 *   - HNSW/HNSW_PQ/HNSW_SQ: 复用 jw_index_search 取足够大的 topK
 *     (HNSW 无法直接做半径检索, 用 k = capacity 的上限再过滤),
 *     这样保证结果按距离有序且不遗漏已建图的节点
 *
 * @param index    索引指针
 * @param query   查询向量
 * @param radius  检索半径 (度量相关: L2 为平方距离, COSINE 为余弦距离, IP 为 -内积)
 * @param results 输出: 搜索结果数组
 * @param capacity 结果数组容量
 * @return 实际返回的结果数量, 参数非法返回 0
 */
JW_API jw_size_t jw_index_range_search(const jw_index_t *index,
                                       jw_cvec_t query,
                                       jw_float32_t radius,
                                       jw_search_result_t *results,
                                       jw_size_t capacity)
{
    if (index == NULL || query == NULL || results == NULL || capacity == 0) {
        return 0;
    }
    if (radius < 0.0f) {
        return 0;
    }

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ: {
            jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
            if (ivf == NULL || ivf->trained != 1) {
                return 0;
            }

            /* 选 nprobe 个最近簇心 */
            jw_uint32_t nprobe = ivf->config.nprobe;
            if (nprobe > ivf->nlist) {
                nprobe = ivf->nlist;
            }
            if (nprobe == 0) {
                return 0;
            }

            jw_uint32_t *probe = (jw_uint32_t *)jw_malloc(nprobe * sizeof(jw_uint32_t));
            if (probe == NULL) {
                return 0;
            }
            jw_uint32_t actual = jw_ivf_get_nearest_centroids(ivf, query, nprobe, probe);

            /* 在候选簇内收集半径内的向量 */
            jw_size_t found = 0;
            for (jw_uint32_t p = 0; p < actual && found < capacity; p++) {
                jw_ivf_list_t *list = &ivf->lists[probe[p]];
                for (jw_size_t j = 0; j < list->count && found < capacity; j++) {
                    jw_ivf_entry_t *entry = &list->entries[j];
                    jw_float32_t dist;
                    if (entry->code != NULL && ivf->pq != NULL
                        && ivf->type == JW_INDEX_IVF_PQ) {
                        dist = jw_pq_distance(ivf->pq, query, entry->code, ivf->metric);
                    } else if (entry->code != NULL && ivf->sq != NULL
                               && ivf->type == JW_INDEX_IVF_SQ) {
                        /* SQ 量化距离: jw_sq_distance(quant, qvec=码, fvec=原始查询, metric) */
                        dist = jw_sq_distance(ivf->sq, entry->code, query, ivf->metric);
                    } else if (entry->vec != NULL) {
                        dist = jw_vec_distance(query, entry->vec, ivf->dim, ivf->metric);
                    } else {
                        /* 数据缺失 (反序列化失败等), 跳过而非解引用 NULL */
                        continue;
                    }
                    if (dist <= radius) {
                        results[found].id = entry->vid;
                        results[found].score = dist;
                        found++;
                    }
                }
            }

            /* 按距离升序排序 */
            for (jw_size_t i = 0; i + 1 < found; i++) {
                jw_size_t m = i;
                for (jw_size_t j = i + 1; j < found; j++) {
                    if (results[j].score < results[m].score) {
                        m = j;
                    }
                }
                if (m != i) {
                    jw_search_result_t tmp = results[i];
                    results[i] = results[m];
                    results[m] = tmp;
                }
            }

            jw_free(probe);
            return found;
        }

        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ: {
            jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
            if (hnsw == NULL) {
                return 0;
            }

            /* HNSW 不支持原生半径检索: 取一个足够大的 topK 再按半径过滤。
               k 上限取 capacity 与 ntotal 的较小值, 保证不会越界。 */
            jw_size_t k = capacity;
            if (k > hnsw->ntotal) {
                k = hnsw->ntotal;
            }
            if (k == 0) {
                return 0;
            }

            jw_search_result_t *tmp = (jw_search_result_t *)jw_malloc(
                    k * sizeof(jw_search_result_t));
            if (tmp == NULL) {
                return 0;
            }

            jw_size_t got = jw_index_search(index, query, k, tmp);

            /* 过滤 + 转换: 内部 score 对 COSINE/IP 已是相似度, 需转回距离比较 */
            jw_size_t found = 0;
            for (jw_size_t i = 0; i < got && found < capacity; i++) {
                jw_float32_t dist;
                if (hnsw->metric == JW_METRIC_COSINE) {
                    dist = 1.0f - tmp[i].score;   /* 相似度 -> 余弦距离 */
                } else if (hnsw->metric == JW_METRIC_IP) {
                    dist = -tmp[i].score;          /* 内积相似度 -> -内积 */
                } else {
                    dist = tmp[i].score;           /* L2 平方距离 */
                }
                if (dist <= radius) {
                    results[found].id = tmp[i].id;
                    results[found].score = tmp[i].score;
                    found++;
                }
            }

            jw_free(tmp);
            return found;
        }

        default:
            return 0;
    }
}


/**
 * 批量搜索
 *
 * @param index   索引指针
 * @param queries 查询向量数组
 * @param nquery  查询数量
 * @param k       每个查询返回的结果数
 * @param results 输出: 搜索结果数组 (长度为nquery * k)
 * @return        状态码
 */
JW_API jw_status_t jw_index_search_batch(const jw_index_t *index,
                                          jw_cvec_t queries,
                                          jw_size_t nquery,
                                          jw_size_t k,
                                          jw_search_result_t *results)
{
    if (index == NULL || queries == NULL || results == NULL) {
        return JW_INVALID_PARAM;
    }

    jw_dim_t dim = jw_index_get_dim(index);

    for (jw_size_t i = 0; i < nquery; i++) {
        jw_cvec_t query = queries + i * dim;
        jw_search_result_t *result = results + i * k;
        jw_size_t count = jw_index_search(index, query, k, result);
        if (count == 0 && k > 0) {
            result[0].id = ((jw_vid_t)-1);
            result[0].score = JW_FLT_MAX;
        }
    }

    return JW_SUCCESS;
}

/*
 * =============================================================================
 * IVF索引特定接口
 * =============================================================================
 */

/**
 * 获取IVF索引的聚类数
 *
 * @param index IVF索引
 * @return     聚类数 (nlist)
 */
JW_API jw_uint32_t jw_ivf_get_nlist(const jw_ivf_index_t *index)
{
    return (index != NULL) ? index->nlist : 0;
}

/**
 * 获取IVF索引某个列表的向量数
 *
 * @param index IVF索引
 * @param list_idx 列表索引
 * @return         该列表的向量数
 */
JW_API jw_size_t jw_ivf_get_list_size(const jw_ivf_index_t *index,
                                       jw_uint32_t list_idx)
{
    if (index == NULL || list_idx >= index->nlist) {
        return 0;
    }
    return index->lists[list_idx].count;
}

/**
 * 获取与查询最近的 n 个聚类中心 (按距离升序)
 *
 * 修复 (2026-10-22): 该 API 在 jw_index.h 声明但从未实现, 导致任何链接
 * 它的代码都失败 (本次由 jw_index_range_search 引入调用时暴露)。
 * 实现复用 IVF 搜索的选簇心逻辑。
 *
 * @param index IVF索引指针
 * @param query 查询向量
 * @param n     需要的聚类中心数量
 * @param ids   输出: 聚类中心索引数组 (长度 >= n)
 * @return 实际返回的数量 (0 表示参数非法或未训练)
 */
JW_API jw_uint32_t jw_ivf_get_nearest_centroids(const jw_ivf_index_t *index,
                                                jw_cvec_t query,
                                                jw_uint32_t n,
                                                jw_uint32_t *ids)
{
    if (index == NULL || query == NULL || ids == NULL || n == 0) {
        return 0;
    }
    if (index->trained != 1 || index->nlist == 0) {
        return 0;
    }

    jw_uint32_t cnt = (n < index->nlist) ? n : index->nlist;
    jw_dim_t dim = index->dim;

    /* 计算所有簇心距离后做选择排序取前 cnt 个 */
    jw_float32_t *dists = (jw_float32_t *)jw_malloc(index->nlist * sizeof(jw_float32_t));
    if (dists == NULL) {
        return 0;
    }
    for (jw_uint32_t i = 0; i < index->nlist; i++) {
        dists[i] = jw_vec_distance(query, index->lists[i].centroid, dim, index->metric);
    }

    for (jw_uint32_t i = 0; i < cnt; i++) {
        jw_uint32_t min_idx = i;
        for (jw_uint32_t j = i + 1; j < index->nlist; j++) {
            if (dists[j] < dists[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            jw_float32_t td = dists[i]; dists[i] = dists[min_idx]; dists[min_idx] = td;
        }
        ids[i] = min_idx;
    }

    jw_free(dists);
    return cnt;
}

/**
 * 设置 IVF 的 nprobe 参数
 *
 * @param index  IVF索引指针
 * @param nprobe 新的 nprobe 值 (0 表示不限制, 会被钳到 nlist)
 */
JW_API void jw_ivf_set_nprobe(jw_ivf_index_t *index, jw_uint32_t nprobe)
{
    if (index == NULL) {
        return;
    }
    index->config.nprobe = nprobe;
}

/*
 * =============================================================================
 * HNSW索引特定接口
 * =============================================================================
 */

/**
 * 创建HNSW索引
 *
 * @param arena   内存池
 * @param dim    向量维度
 * @param config HNSW配置
 * @return       HNSW索引指针
 */
JW_API jw_hnsw_index_t *jw_hnsw_create(jw_arena_t *arena,
                                        jw_dim_t dim,
                                        const jw_hnsw_config_t *config)
{
    if (dim == 0 || config == NULL) {
        return NULL;
    }

    /** 创建本地内存池 */
    jw_arena_t *local_arena = arena;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
    }

    /** 分配HNSW索引结构 */
    jw_hnsw_index_t *index = (jw_hnsw_index_t *)jw_arena_calloc(local_arena, 1,
                               sizeof(jw_hnsw_index_t));
    if (index == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }

    /** 初始化HNSW索引字段 */
    index->type = JW_INDEX_HNSW;
    index->dim = dim;
    index->metric = JW_METRIC_L2;
    index->config = *config;
    index->arena = local_arena;

    /* 防御：未显式赋值/非法的 HNSW 参数回退到安全默认，避免野值导致内存越界。
       M < 2 时 log(M) <= 0, 1/log(M) 会除零产生 inf, 进而使层级计算的
       (uint32_t)inf 成为未定义行为, 故一并兜底为 16。 */
    if (index->config.M < 2) {
        index->config.M = 16;
    }
    if (index->config.ef_construction == 0) {
        index->config.ef_construction = 200;
    }
    if (index->config.ef_search == 0) {
        index->config.ef_search = 100;
    }
    if (index->config.seed == 0) {
        index->config.seed = 42;
    }
    index->ntotal = 0;
    index->capacity = 4096;  /** 初始容量 */
    index->max_level = 0;
    index->entry_point = ((jw_vid_t)-1);

    /** 分配节点数组 */
    index->nodes = (jw_hnsw_node_t **)jw_arena_calloc(local_arena,
                        index->capacity, sizeof(jw_hnsw_node_t *));
    if (index->nodes == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }

    /** 创建读写锁 */
    jw_rwlock_t *lock;
    jw_rwlock_create(local_arena, NULL, &lock);
    index->lock = lock;

    /** 初始化随机数生成器 */
    index->rng_state = config->seed ? config->seed : 12345;

    /** 如果未设置level_mult，使用默认值 1/ln(M) */
    if (index->config.level_mult <= 0) {
        index->config.level_mult = 1.0f / jw_math_log_f32((jw_float32_t)config->M);
    }

    return index;
}

/**
 * 销毁HNSW索引
 *
 * @param index HNSW索引指针
 */
JW_API void jw_hnsw_destroy(jw_hnsw_index_t *index)
{
    if (index == NULL) {
        return;
    }

    /** 销毁读写锁 */
    if (index->lock != NULL) {
        jw_rwlock_destroy(index->lock);
    }

    /** 节点内存由外部内存池统一释放 */
}

/**
 * 获取HNSW索引的最大层级
 *
 * @param index HNSW索引
 * @return      最大层级
 */
JW_API jw_uint32_t jw_hnsw_get_max_level(const jw_hnsw_index_t *index)
{
    return (index != NULL) ? index->max_level : 0;
}

/**
 * 获取HNSW索引的入口点
 *
 * @param index HNSW索引
 * @return      入口点向量ID
 */
JW_API jw_vid_t jw_hnsw_get_entry_point(const jw_hnsw_index_t *index)
{
    return (index != NULL) ? index->entry_point : ((jw_vid_t)-1);
}

/*
 * =============================================================================
 * 索引序列化和反序列化
 * =============================================================================
 */

/**
 * 保存索引到文件
 *
 * @param index 索引指针
 * @param filename 文件路径
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_save(const jw_index_t *index,
                                  const jw_str_t *filename)
{
    if (index == NULL || filename == NULL || filename->ptr == NULL) {
        return JW_INVALID_PARAM;
    }

    /* 直接打开文件 */
    int fd = open(filename->ptr, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return JW_IO_ERROR;
    }

    jw_status_t status = JW_SUCCESS;

    /* 写入索引头部 */
    if (write(fd, &index->type, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) {
        status = JW_IO_ERROR;
        goto cleanup;
    }

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ: {
            jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
            
            /* 写入IVF索引数据 */
            if (write(fd, &ivf->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &ivf->nlist, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            jw_uint32_t metric = (jw_uint32_t)ivf->metric;
            if (write(fd, &metric, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &ivf->trained, sizeof(jw_uint8_t)) != sizeof(jw_uint8_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &ivf->config.nlist, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &ivf->config.nprobe, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &ivf->config.max_iter, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            
            /* 写入量化器 (必须在聚类中心/倒排列表之前, 与加载侧顺序一致)
             * 修复 2026-10-22: 原实现在倒排列表之后才写, 而加载侧读 entry
             * 时就需要 ivf->pq/sq 非空才能正确解析, 顺序不一致导致
             * IVF_PQ 存盘后重载即段错误。 */
            if (ivf->type == JW_INDEX_IVF_PQ && ivf->pq != NULL) {
                if (write(fd, &ivf->pq->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &ivf->pq->nsub, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &ivf->pq->k, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &ivf->pq->sub_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                for (jw_uint32_t qi = 0; qi < ivf->pq->nsub; qi++) {
                    if (write(fd, ivf->pq->centroids[qi], ivf->pq->k * ivf->pq->sub_dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->pq->k * ivf->pq->sub_dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                }
            } else if (ivf->type == JW_INDEX_IVF_SQ && ivf->sq != NULL) {
                if (write(fd, &ivf->sq->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, ivf->sq->mins, ivf->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, ivf->sq->maxs, ivf->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, ivf->sq->scales, ivf->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
            }

            /* 写入聚类中心 */
            for (jw_uint32_t i = 0; i < ivf->nlist; i++) {
                if (write(fd, ivf->lists[i].centroid, ivf->dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->dim * sizeof(jw_float32_t))) {
                    status = JW_IO_ERROR;
                    goto cleanup;
                }
            }
            
            /* 写入倒排列表 */
            for (jw_uint32_t i = 0; i < ivf->nlist; i++) {
                jw_ivf_list_t *list = &ivf->lists[i];
                if (write(fd, &list->count, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                
                for (jw_size_t j = 0; j < list->count; j++) {
                    jw_ivf_entry_t *entry = &list->entries[j];
                    if (write(fd, &entry->vid, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
                    
                    if (ivf->type == JW_INDEX_IVF_PQ && ivf->pq != NULL && entry->code != NULL) {
                        if (write(fd, entry->code, ivf->pq->nsub) != (ssize_t)ivf->pq->nsub) { status = JW_IO_ERROR; goto cleanup; }
                    } else if (ivf->type == JW_INDEX_IVF_SQ && ivf->sq != NULL && entry->code != NULL) {
                        if (write(fd, entry->code, ivf->dim) != (ssize_t)ivf->dim) { status = JW_IO_ERROR; goto cleanup; }
                    } else if (entry->vec != NULL) {
                        if (write(fd, entry->vec, ivf->dim * sizeof(jw_float32_t)) != (ssize_t)(ivf->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                    }
                }
            }
            
            /* 写入量化器 (已移至聚类中心之前写入, 此处不再重复) */
            break;
        }
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ: {
            jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
            
            /* 写入HNSW索引数据 */
            if (write(fd, &hnsw->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->metric, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->ntotal, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->capacity, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->max_level, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->entry_point, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->config.M, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->config.ef_construction, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->config.ef_search, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->config.level_mult, sizeof(jw_float32_t)) != sizeof(jw_float32_t)) { status = JW_IO_ERROR; goto cleanup; }
            if (write(fd, &hnsw->rng_state, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
            
            /* 先写量化器 (必须在节点数据之前: 加载时需要 nsub 才能解析节点的 PQ 编码) */
            if (hnsw->type == JW_INDEX_HNSW_PQ && hnsw->pq != NULL) {
                if (write(fd, &hnsw->pq->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &hnsw->pq->nsub, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &hnsw->pq->k, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, &hnsw->pq->sub_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }

                for (jw_uint32_t i = 0; i < hnsw->pq->nsub; i++) {
                    if (write(fd, hnsw->pq->centroids[i], hnsw->pq->k * hnsw->pq->sub_dim * sizeof(jw_float32_t)) !=
                        (ssize_t)(hnsw->pq->k * hnsw->pq->sub_dim * sizeof(jw_float32_t))) {
                        status = JW_IO_ERROR;
                        goto cleanup;
                    }
                }
            } else if (hnsw->type == JW_INDEX_HNSW_SQ && hnsw->sq != NULL) {
                if (write(fd, &hnsw->sq->dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, hnsw->sq->mins, hnsw->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(hnsw->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, hnsw->sq->maxs, hnsw->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(hnsw->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                if (write(fd, hnsw->sq->scales, hnsw->sq->dim * sizeof(jw_float32_t)) != (ssize_t)(hnsw->sq->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
            }

            /* 写入节点数据 */
            for (jw_size_t i = 0; i < hnsw->ntotal; i++) {
                jw_hnsw_node_t *node = hnsw->nodes[i];
                if (node != NULL) {
                    if (write(fd, &node->vid, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
                    if (write(fd, &node->level, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                    if (hnsw->pq != NULL) {
                        /** HNSW_PQ: 写 PQ 编码 (nsub 字节) 而非原始向量 */
                        if (write(fd, node->code, hnsw->pq->nsub) != (ssize_t)hnsw->pq->nsub) { status = JW_IO_ERROR; goto cleanup; }
                    } else {
                        if (write(fd, node->vec, hnsw->dim * sizeof(jw_float32_t)) != (ssize_t)(hnsw->dim * sizeof(jw_float32_t))) { status = JW_IO_ERROR; goto cleanup; }
                    }

                    /* 写入连接 */
                    for (jw_uint32_t l = 0; l <= node->level; l++) {
                        if (write(fd, &node->link_counts[l], sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) { status = JW_IO_ERROR; goto cleanup; }
                        for (jw_uint32_t j = 0; j < node->link_counts[l]; j++) {
                            if (write(fd, &node->links[l][j], sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) { status = JW_IO_ERROR; goto cleanup; }
                        }
                    }
                }
            }
            break;
        }
        default:
            status = JW_NOT_SUPPORTED;
            break;
    }

cleanup:
    close(fd);
    return status;
}

/**
 * 从文件加载索引
 *
 * @param arena 内存池
 * @param filename 文件路径
 * @return 索引指针，失败返回NULL
 */
JW_API jw_index_t *jw_index_load(jw_arena_t *arena,
                                  const jw_str_t *filename)
{
    if (arena == NULL || filename == NULL || filename->ptr == NULL) {
        return NULL;
    }

    /* 直接打开文件 */
    int fd = open(filename->ptr, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    jw_index_t *index = NULL;

    /* 读取索引类型 */
    jw_uint32_t index_type;
    if (read(fd, &index_type, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) {
        goto cleanup;
    }

    switch (index_type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ:
        case JW_INDEX_IVF_SQ: {
            /* 读取IVF索引数据 */
            jw_uint32_t dim, nlist, metric, trained, nlist_config, nprobe, max_iter;
            
            if (read(fd, &dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &nlist, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &metric, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &trained, sizeof(jw_uint8_t)) != sizeof(jw_uint8_t)) goto cleanup;
            if (read(fd, &nlist_config, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &nprobe, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &max_iter, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

            /* 创建IVF索引 */
            jw_index_config_t config = {
                .type = (jw_index_type_t)index_type,
                .dim = dim,
                .params.ivf = {
                    .nlist = nlist_config,
                    .nprobe = nprobe,
                    .max_iter = max_iter
                }
            };
            
            index = jw_index_create(arena, &config);
            if (index == NULL) goto cleanup;
            
            jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
            ivf->metric = metric;
            ivf->trained = trained;
            ivf->ntotal = 0;
            
            /* 读取量化器
             *
             * 修复 (2026-10-22): 必须在"读取倒排列表"之前完成。
             * 原实现把量化器放在倒排列表之后, 导致读 entry 时
             * ivf->pq / ivf->sq 仍为 NULL, 于是落入 else 分支把
             * PQ 编码/SQ 量化字节当作 dim*4 的浮点向量读入:
             *   - 字节数错位, 后续全部数据解析错乱
             *   - entry->code 保持 NULL, 而搜索路径按 IVF_PQ 走
             *     jw_pq_distance(pq, query, entry->code=NULL) -> SIGSEGV
             * 实测: IVF_PQ 存盘后重载, 一搜索即段错误 (普通 IVF 正常)。
             * 这与 HNSW_PQ 的同类顺序 bug 是同一个模式, 当时只修了 HNSW。
             */
            if (ivf->type == JW_INDEX_IVF_PQ) {
                jw_uint32_t pq_dim, pq_nsub, pq_k, pq_subdim;

                if (read(fd, &pq_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_nsub, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_k, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_subdim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

                jw_pq_quantizer_t *pq = NULL;
                jw_pq_create(arena, pq_dim, pq_nsub, 8, &pq);  /* 8 bits per subvector */
                if (pq != NULL) {
                    ivf->pq = pq;
                    pq->metric = metric;  /* 恢复训练度量, 供 jw_pq_distance 归一化判断 */

                    for (jw_uint32_t i = 0; i < pq_nsub; i++) {
                        if (read(fd, pq->centroids[i], pq_k * pq_subdim * sizeof(jw_float32_t)) !=
                            (ssize_t)(pq_k * pq_subdim * sizeof(jw_float32_t))) {
                            goto cleanup;
                        }
                    }
                }
            } else if (ivf->type == JW_INDEX_IVF_SQ) {
                jw_uint32_t sq_dim;

                if (read(fd, &sq_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

                jw_sq_quantizer_t *sq = jw_sq_quantizer_create(arena, sq_dim);
                if (sq != NULL) {
                    ivf->sq = sq;

                    if (read(fd, sq->mins, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }

                    if (read(fd, sq->maxs, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }

                    if (read(fd, sq->scales, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }
                }
            }

            /* 读取聚类中心 */
            for (jw_uint32_t i = 0; i < nlist; i++) {
                if (read(fd, ivf->lists[i].centroid, dim * sizeof(jw_float32_t)) != (ssize_t)(dim * sizeof(jw_float32_t))) {
                    goto cleanup;
                }
            }

            /* 读取倒排列表 */
            for (jw_uint32_t i = 0; i < nlist; i++) {
                jw_ivf_list_t *list = &ivf->lists[i];
                jw_uint32_t count;
                if (read(fd, &count, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

                list->count = count;
                ivf->ntotal += count;
                if (count > 0) {
                    /* 扩容列表 */
                    if (list->capacity < count) {
                        list->capacity = count * 2;
                        list->entries = (jw_ivf_entry_t *)jw_arena_calloc(arena,
                                         list->capacity, sizeof(jw_ivf_entry_t));
                        if (list->entries == NULL) goto cleanup;
                    }

                    for (jw_size_t j = 0; j < count; j++) {
                        jw_ivf_entry_t *entry = &list->entries[j];

                        if (read(fd, &entry->vid, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;

                        if (ivf->type == JW_INDEX_IVF_PQ && ivf->pq != NULL) {
                            entry->code = (jw_uint8_t *)jw_arena_alloc(arena, ivf->pq->nsub);
                            if (entry->code == NULL) goto cleanup;
                            if (read(fd, entry->code, ivf->pq->nsub) != (ssize_t)ivf->pq->nsub) {
                                goto cleanup;
                            }
                            entry->vec = NULL;
                        } else if (ivf->type == JW_INDEX_IVF_SQ && ivf->sq != NULL) {
                            entry->code = (jw_uint8_t *)jw_arena_alloc(arena, dim);
                            if (entry->code == NULL) goto cleanup;
                            if (read(fd, entry->code, dim) != (ssize_t)dim) {
                                goto cleanup;
                            }
                            entry->vec = NULL;
                        } else {
                            entry->vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
                            if (entry->vec == NULL) goto cleanup;
                            if (read(fd, entry->vec, dim * sizeof(jw_float32_t)) != (ssize_t)(dim * sizeof(jw_float32_t))) {
                                goto cleanup;
                            }
                            entry->code = NULL;
                        }
                    }
                }
            }
            break;
        }
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ:
        case JW_INDEX_HNSW_SQ: {
            /* 读取HNSW索引数据 */
            jw_uint32_t dim, metric, max_level, M, ef_construction, ef_search;
            jw_float32_t level_mult;
            jw_uint64_t ntotal, capacity, entry_point, rng_state;
            
            if (read(fd, &dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &metric, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &ntotal, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;
            if (read(fd, &capacity, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;
            if (read(fd, &max_level, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &entry_point, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;
            if (read(fd, &M, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &ef_construction, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &ef_search, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
            if (read(fd, &level_mult, sizeof(jw_float32_t)) != sizeof(jw_float32_t)) goto cleanup;
            if (read(fd, &rng_state, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;

            /* 创建HNSW索引 */
            jw_index_config_t config = {
                .type = (jw_index_type_t)index_type,
                .dim = dim,
                .params.hnsw = {
                    .M = M,
                    .ef_construction = ef_construction,
                    .ef_search = ef_search,
                    .level_mult = level_mult,
                    .seed = (jw_uint32_t)rng_state
                }
            };
            
            index = jw_index_create(arena, &config);
            if (index == NULL) goto cleanup;
            
            jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
            hnsw->metric = metric;
            hnsw->ntotal = ntotal;
            hnsw->capacity = capacity;
            hnsw->max_level = max_level;
            hnsw->entry_point = entry_point;
            hnsw->rng_state = rng_state;

            /* 先读量化器 (与保存顺序一致: 节点数据之前; 且需要 nsub 才能解析节点的 PQ 编码) */
            if (hnsw->type == JW_INDEX_HNSW_PQ) {
                jw_uint32_t pq_dim, pq_nsub, pq_k, pq_subdim;

                if (read(fd, &pq_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_nsub, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_k, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                if (read(fd, &pq_subdim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

                jw_pq_quantizer_t *pq = NULL;
                jw_pq_create(arena, pq_dim, pq_nsub, 8, &pq);  /* 8 bits per subvector */
                if (pq != NULL) {
                    pq->metric = metric;  /* 恢复训练时的度量, 供 jw_pq_distance 归一化判断 */
                    hnsw->pq = pq;

                    for (jw_uint32_t i = 0; i < pq_nsub; i++) {
                        if (read(fd, pq->centroids[i], pq_k * pq_subdim * sizeof(jw_float32_t)) !=
                            (ssize_t)(pq_k * pq_subdim * sizeof(jw_float32_t))) {
                            goto cleanup;
                        }
                    }
                }
            } else if (hnsw->type == JW_INDEX_HNSW_SQ) {
                jw_uint32_t sq_dim;

                if (read(fd, &sq_dim, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;

                jw_sq_quantizer_t *sq = jw_sq_quantizer_create(arena, sq_dim);
                if (sq != NULL) {
                    hnsw->sq = sq;

                    if (read(fd, sq->mins, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }

                    if (read(fd, sq->maxs, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }

                    if (read(fd, sq->scales, sq_dim * sizeof(jw_float32_t)) != (ssize_t)(sq_dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }
                }
            }

            /* 读取节点数据 */
            for (jw_size_t i = 0; i < ntotal; i++) {
                jw_uint64_t vid;
                jw_uint32_t node_level;
                
                if (read(fd, &vid, sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) goto cleanup;
                if (read(fd, &node_level, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) goto cleanup;
                
                /* 扩容节点数组 */
                if (vid >= hnsw->capacity) {
                    jw_size_t new_capacity = (vid + 1) * 2;
                    jw_hnsw_node_t **new_nodes = (jw_hnsw_node_t **)jw_arena_calloc(
                        arena, new_capacity, sizeof(jw_hnsw_node_t *));
                    if (new_nodes == NULL) goto cleanup;
                    if (hnsw->nodes != NULL) {
                        jw_memcpy(new_nodes, hnsw->nodes, hnsw->capacity * sizeof(jw_hnsw_node_t *));
                    }
                    hnsw->nodes = new_nodes;
                    hnsw->capacity = new_capacity;
                }
                
                /* 创建节点 */
                jw_hnsw_node_t *node = (jw_hnsw_node_t *)jw_arena_calloc(arena, 1, sizeof(jw_hnsw_node_t));
                if (node == NULL) goto cleanup;
                
                node->vid = vid;
                node->level = node_level;
                node->max_level = node_level;
                node->max_M = M;
                node->max_M0 = M * 2;
                
                /* 读取数据: PQ 模式读编码, 否则读原始向量 */
                if (hnsw->pq != NULL) {
                    node->code = (jw_uint8_t *)jw_arena_alloc(arena, hnsw->pq->nsub);
                    if (node->code == NULL) {
                        goto cleanup;
                    }
                    if (read(fd, node->code, hnsw->pq->nsub) != (ssize_t)hnsw->pq->nsub) {
                        goto cleanup;
                    }
                } else {
                    node->vec = (jw_vec_t)jw_arena_alloc(arena, dim * sizeof(jw_float32_t));
                    if (node->vec == NULL) {
                        goto cleanup;
                    }
                    if (read(fd, node->vec, dim * sizeof(jw_float32_t)) != (ssize_t)(dim * sizeof(jw_float32_t))) {
                        goto cleanup;
                    }
                }
                
                /* 读取连接 */
                node->links = (jw_vid_t **)jw_arena_alloc(arena, (node_level + 1) * sizeof(jw_vid_t *));
                node->link_counts = (jw_uint32_t *)jw_arena_alloc(arena, (node_level + 1) * sizeof(jw_uint32_t));
                if (node->links == NULL || node->link_counts == NULL) {
                    goto cleanup;
                }
                
                for (jw_uint32_t l = 0; l <= node_level; l++) {
                    jw_uint32_t link_count;
                    if (read(fd, &link_count, sizeof(jw_uint32_t)) != sizeof(jw_uint32_t)) {
                        goto cleanup;
                    }
                    
                    node->link_counts[l] = link_count;
                    /* 第0层用 max_M0，其他层用 max_M */
                    jw_uint32_t alloc_M_load = (l == 0) ? (node->max_M0) : (node->max_M);
                    node->links[l] = (jw_vid_t *)jw_arena_alloc(arena, alloc_M_load * sizeof(jw_vid_t));
                    if (node->links[l] == NULL) {
                        goto cleanup;
                    }
                    
                    for (jw_uint32_t j = 0; j < link_count; j++) {
                        if (read(fd, &node->links[l][j], sizeof(jw_uint64_t)) != sizeof(jw_uint64_t)) {
                            goto cleanup;
                        }
                    }
                }
                
                hnsw->nodes[vid] = node;
            }
            break;
        }
        default:
            goto cleanup;
    }

cleanup:
    close(fd);
    return index;
}

/**
 * 获取索引内存使用量
 *
 * @param index 索引指针
 * @return 内存使用量 (字节)
 */
JW_API jw_size_t jw_index_get_memory_usage(const jw_index_t *index)
{
    if (index == NULL || index->impl == NULL) {
        return 0;
    }

    jw_size_t usage = sizeof(jw_index_t);

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ: {
            jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
            usage += sizeof(jw_ivf_index_t);
            usage += ivf->nlist * sizeof(jw_ivf_list_t);
            for (jw_uint32_t i = 0; i < ivf->nlist; i++) {
                usage += ivf->lists[i].count * sizeof(jw_ivf_entry_t);
                usage += ivf->lists[i].count * ivf->dim * sizeof(jw_float32_t);
            }
            break;
        }
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ: {
            jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
            usage += sizeof(jw_hnsw_index_t);
            usage += hnsw->capacity * sizeof(jw_hnsw_node_t *);
            for (jw_size_t i = 0; i < hnsw->ntotal; i++) {
                if (hnsw->nodes[i] != NULL) {
                    usage += sizeof(jw_hnsw_node_t);
                    if (hnsw->pq != NULL) {
                        usage += hnsw->pq->nsub * sizeof(jw_uint8_t);
                    } else {
                        usage += hnsw->dim * sizeof(jw_float32_t);
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    return usage;
}

/**
 * 获取索引统计信息
 *
 * @param index 索引指针
 * @param stats 统计信息输出
 * @return JW_SUCCESS 成功，其他值失败
 */
JW_API jw_status_t jw_index_get_stats(const jw_index_t *index,
                                       jw_index_stats_t *stats)
{
    if (index == NULL || stats == NULL) {
        return JW_INVALID_PARAM;
    }

    jw_memset(stats, 0, sizeof(jw_index_stats_t));

    stats->ntotal = jw_index_get_ntotal(index);

    switch (index->type) {
        case JW_INDEX_IVF:
        case JW_INDEX_IVF_PQ: {
            jw_ivf_index_t *ivf = (jw_ivf_index_t *)index->impl;
            stats->nlist = ivf->nlist;
            break;
        }
        case JW_INDEX_HNSW:
        case JW_INDEX_HNSW_PQ: {
            jw_hnsw_index_t *hnsw = (jw_hnsw_index_t *)index->impl;
            stats->max_level = hnsw->max_level;
            break;
        }
        default:
            break;
    }

    stats->memory_used = jw_index_get_memory_usage(index);

    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 宏定义展开函数 (解决Windows编译问题)
 * =============================================================================
 */

/** Windows平台下解决JW_UNUSED编译警告 */
#ifndef JW_UNUSED
#define JW_UNUSED(x) (void)(x)
#endif

JW_END_DECL
