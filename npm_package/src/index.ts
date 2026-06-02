/**
 * JinWo VecDB - JavaScript/TypeScript API
 *
 * WASM 驱动的嵌入式向量数据库
 *
 * 用法:
 *   import { open } from 'jinwo-vecdb';
 *   const db = await open('my_vecs.jwv');
 *   const coll = db.createCollection('docs', 384);
 *   coll.insert([0.1, 0.2, ...]);
 *   const results = coll.search(query, 5);
 *   db.close();
 */

import initModule from '../wasm/jinwo.js';

// ============================================================================
// WASM 模块 (懒加载)
// ============================================================================

let M: any = null;

async function ensureWasm(): Promise<any> {
  if (M) return M;
  M = await initModule();
  return M;
}

// ============================================================================
// C 常量
// ============================================================================

const JW_SUCCESS = 0;
const JW_VECDB_CREATE = 0x04;
const JW_VECDB_READWRITE = 0x02;
const JW_VECDB_MEMORY = 0x10;

// ============================================================================
// WASM 类型大小 (wasm32)
// ============================================================================

const SZ_JW_STR_T = 8;    // jw_str_t: { ptr(4), slen(4) }
const SZ_JW_VID_T = 8;    // jw_vid_t: u64
// jw_search_result: id(u64@0)+score(f32@8)+vector(ptr@12)+dim(u32@16)+metadata(ptr@20)+meta_size(u32@24)+pad(8)
// wasm32 按 8 字节对齐补齐到 32
const SZ_JW_SEARCH_RESULT = 32;

// ============================================================================
// 工具函数
// ============================================================================

/** 将 JS 字符串编码为 UTF-8 字节数组 */
function utf8Encode(s: string): Uint8Array {
  const bytes: number[] = [];
  for (let i = 0; i < s.length; i++) {
    let c = s.charCodeAt(i);
    if (c < 0x80) {
      bytes.push(c);
    } else if (c < 0x800) {
      bytes.push(0xc0 | (c >> 6), 0x80 | (c & 0x3f));
    } else if (c < 0xd800 || c >= 0xe000) {
      bytes.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f));
    } else {
      i++;
      c = 0x10000 + (((c & 0x3ff) << 10) | (s.charCodeAt(i) & 0x3ff));
      bytes.push(
        0xf0 | (c >> 18),
        0x80 | ((c >> 12) & 0x3f),
        0x80 | ((c >> 6) & 0x3f),
        0x80 | (c & 0x3f)
      );
    }
  }
  return new Uint8Array(bytes);
}

/** 在 WASM 内存中创建 jw_str_t 结构 */
function writeJwStr(s: string): number {
  const utf8 = utf8Encode(s);
  const len = utf8.length;
  // 分配: struct(8) + string data + null terminator (for internal strdup)
  const structPtr = M._malloc(SZ_JW_STR_T + len + 1);
  const strPtr = structPtr + SZ_JW_STR_T;
  M.HEAPU8.set(utf8, strPtr);
  M.HEAPU8[strPtr + len] = 0; // null terminator
  M.setValue(structPtr, strPtr, '*');
  M.setValue(structPtr + 4, len, 'i32');
  return structPtr;
}

/** 释放 jw_str_t */
function freeJwStr(ptr: number): void {
  M._free(ptr);
}

/** 将 JS float 数组写入 WASM 内存，返回指针 */
function writeFloatArray(arr: number[]): number {
  const byteLen = arr.length * 4;
  const ptr = M._malloc(byteLen);
  M.HEAPF32.set(arr, ptr >> 2);
  return ptr;
}

/** 从 WASM 内存读取 float 数组 */
function readFloatArray(ptr: number, len: number): number[] {
  const result: number[] = [];
  const base = ptr >> 2;
  for (let i = 0; i < len; i++) {
    result.push(M.HEAPF32[base + i]);
  }
  return result;
}

// ============================================================================
// 类型定义
// ============================================================================

/** 搜索结果条目 */
export interface SearchResult {
  /** 向量唯一 ID */
  id: number;
  /** 相似度分数 */
  score: number;
}

// ============================================================================
// Collection 类
// ============================================================================

export class Collection {
  constructor(
    public readonly name: string,
    public readonly dim: number,
    private handle: number,  // jw_collection_t *
  ) {}

  /**
   * 插入一条向量
   * @returns 分配的向量 ID
   */
  insert(vector: number[]): number {
    if (vector.length !== this.dim) {
      throw new Error(
        `向量维度不匹配: 期望 ${this.dim}, 实际 ${vector.length}`
      );
    }
    const vecPtr = writeFloatArray(vector);
    const vidPtr = M._malloc(SZ_JW_VID_T);
    M.setValue(vidPtr, BigInt(0), 'i64');

    const status = M._jw_collection_insert(
      this.handle, vecPtr, vidPtr
    );
    const vid = M.getValue(vidPtr, 'i64');
    M._free(vidPtr);
    M._free(vecPtr);

    if (status !== JW_SUCCESS) {
      throw new Error(`插入向量失败: status ${status}`);
    }
    return Number(vid);
  }

  /**
   * 删除一个向量
   * @param vid 向量 ID
   */
  delete(vid: number): void {
    // jw_vid_t 是 u64，wasm32 需传 BigInt
    const status = M._jw_collection_delete(this.handle, BigInt(vid));
    if (status !== JW_SUCCESS) {
      throw new Error(`删除向量失败: status ${status}`);
    }
  }

  /**
   * 构建索引，加速搜索
   */
  buildIndex(): void {
    const status = M._jw_collection_build_index(this.handle);
    if (status !== JW_SUCCESS) {
      throw new Error(`构建索引失败: status ${status}`);
    }
  }
}

// ============================================================================
// JinWoDB 类
// ============================================================================

export class JinWoDB {
  private handle: number = 0;  // jw_vecdb_t *
  private collections: Map<string, Collection> = new Map();

  constructor(handle: number) {
    this.handle = handle;
  }

  /**
   * 创建 Collection（类似 SQLite 的 CREATE TABLE）
   * @param name 集合名称
   * @param dim  向量维度
   */
  createCollection(name: string, dim: number): Collection {
    if (this.collections.has(name)) {
      throw new Error(`Collection '${name}' 已存在`);
    }
    const nameStr = writeJwStr(name);
    const collPtrPtr = M._malloc(4); // jw_collection_t **
    M.setValue(collPtrPtr, 0, '*');

    const status = M._jw_vecdb_create_collection(
      this.handle, nameStr, dim, collPtrPtr
    );
    const collHandle = M.getValue(collPtrPtr, '*');
    M._free(collPtrPtr);
    freeJwStr(nameStr);

    if (status !== JW_SUCCESS) {
      throw new Error(`创建 Collection '${name}' 失败: status ${status}`);
    }
    const coll = new Collection(name, dim, collHandle);
    this.collections.set(name, coll);
    return coll;
  }

  /**
   * 获取已创建的 Collection
   */
  getCollection(name: string): Collection | null {
    if (this.collections.has(name)) return this.collections.get(name)!;
    // 尝试用 WASM 查找（之前创建过但不在当前实例缓存中）
    const nameStr = writeJwStr(name);
    const collPtr = M._jw_vecdb_get_collection(this.handle, nameStr);
    freeJwStr(nameStr);
    if (collPtr === 0) return null;
    // 返回一个受限的 Collection（无法获取 dim，但基本操作可用）
    const coll = new Collection(name, 0, collPtr);
    this.collections.set(name, coll);
    return coll;
  }

  /**
   * 插入向量（自动处理 Collection 创建）
   * @param collName Collection 名称
   * @param vector   向量数据
   * @returns 分配的向量 ID
   */
  insert(collName: string, vector: number[]): number {
    const nameStr = writeJwStr(collName);
    const vecPtr = writeFloatArray(vector);
    const vidPtr = M._malloc(SZ_JW_VID_T);
    M.setValue(vidPtr, BigInt(0), 'i64');

    const status = M._jw_vecdb_insert(
      this.handle, nameStr, vecPtr, vector.length, vidPtr
    );
    const vid = M.getValue(vidPtr, 'i64');
    M._free(vidPtr);
    M._free(vecPtr);
    freeJwStr(nameStr);

    if (status !== JW_SUCCESS) {
      throw new Error(`插入向量失败: status ${status}`);
    }
    return Number(vid);
  }

  /**
   * 向量相似度搜索
   * @param collName Collection 名称
   * @param query    查询向量
   * @param k        返回结果数
   */
  search(collName: string, query: number[], k: number = 10): SearchResult[] {
    const nameStr = writeJwStr(collName);
    const queryPtr = writeFloatArray(query);
    const resultsPtr = M._malloc(k * SZ_JW_SEARCH_RESULT);

    const count = M._jw_vecdb_search(
      this.handle, nameStr, queryPtr, query.length, k, resultsPtr
    );

    const results: SearchResult[] = [];
    for (let i = 0; i < count && i < k; i++) {
      const base = resultsPtr + i * SZ_JW_SEARCH_RESULT;
      results.push({
        id: Number(M.getValue(base, 'i64')),
        score: M.getValue(base + 8, 'float'),
      });
    }

    M._free(resultsPtr);
    M._free(queryPtr);
    freeJwStr(nameStr);
    return results;
  }

  /**
   * 关闭数据库，保存数据到磁盘
   */
  close(): void {
    if (this.handle === 0) return;
    M._jw_vecdb_close(this.handle);
    this.handle = 0;
  }

  /**
   * 数据库是否已打开
   */
  get isOpen(): boolean {
    return this.handle !== 0;
  }

  /**
   * 获取版本信息
   */
  version(): string {
    // jw_vecdb_version 返回 jw_str_t 结构体
    // 在 wasm ABI 中，结构体返回值通过隐藏的第一个指针参数传递
    const resultPtr = M._malloc(SZ_JW_STR_T);
    M._jw_vecdb_version(resultPtr);
    const strPtr = M.getValue(resultPtr, '*');
    const strLen = M.getValue(resultPtr + 4, 'i32');
    const version = M.UTF8ToString(strPtr, strLen);
    M._free(resultPtr);
    return version;
  }
}

// ============================================================================
// 公开 API
// ============================================================================

/**
 * 打开或创建数据库
 *
 * @param path 文件路径，空字符串 '' 表示内存数据库
 * @returns JinWoDB 实例
 *
 * @example
 * ```ts
 * // 文件数据库
 * const db = await open('my_vecs.jwv');
 *
 * // 内存数据库
 * const db = await open('');
 * ```
 */
export async function open(path: string = ''): Promise<JinWoDB> {
  await ensureWasm();

  const pathStr = writeJwStr(path);
  const dbPtrPtr = M._malloc(4); // jw_vecdb_t **
  M.setValue(dbPtrPtr, 0, '*');

  let flags = JW_VECDB_READWRITE;
  if (path === '') {
    flags |= JW_VECDB_MEMORY;
  } else {
    // 自动检测：若目录已存在则不传 CREATE，避免重新初始化覆盖已有数据
    try {
      const { statSync } = await import('fs');
      if (!statSync(path).isDirectory()) {
        flags |= JW_VECDB_CREATE;
      }
    } catch {
      flags |= JW_VECDB_CREATE;
    }
  }

  const status = M._jw_vecdb_open(pathStr, flags, dbPtrPtr);
  const dbHandle = M.getValue(dbPtrPtr, '*');
  M._free(dbPtrPtr);
  freeJwStr(pathStr);

  if (status !== JW_SUCCESS) {
    throw new Error(
      `打开数据库失败 '${path || ':memory:'}': status ${status}`
    );
  }
  return new JinWoDB(dbHandle);
}

export default { open, JinWoDB, Collection };
