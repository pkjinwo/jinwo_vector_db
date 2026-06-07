/**
 * JinWo VecDB - JavaScript/TypeScript API
 *
 * WASM 驱动的嵌入式向量数据库
 *
 * 用法:
 *   import { open } from 'jinwo-vecdb';
 *   const db = await open('');              // 内存模式
 *   const db = await open('/data/my_db');   // 文件持久化 (仅 Node.js)
 *   const coll = db.createCollection('docs', 384);
 *   coll.insert([0.1, 0.2, ...]);
 *   const results = coll.search(query, 5);
 *   db.close();
 */
/** 搜索结果条目 */
export interface SearchResult {
    /** 向量唯一 ID */
    id: number;
    /** 相似度分数 */
    score: number;
}
export declare class Collection {
    readonly name: string;
    readonly dim: number;
    private handle;
    constructor(name: string, dim: number, handle: number);
    /**
     * 插入一条向量
     * @returns 分配的向量 ID
     */
    insert(vector: number[]): number;
    /**
     * 删除一个向量
     * @param vid 向量 ID
     */
    delete(vid: number): void;
    /**
     * 构建索引，加速搜索
     */
    buildIndex(): void;
}
export declare class JinWoDB {
    private handle;
    private collections;
    constructor(handle: number);
    /**
     * 创建 Collection（类似 SQLite 的 CREATE TABLE）
     * @param name 集合名称
     * @param dim  向量维度
     */
    createCollection(name: string, dim: number): Collection;
    /**
     * 获取已创建的 Collection
     */
    getCollection(name: string): Collection | null;
    /**
     * 插入向量（自动处理 Collection 创建）
     * @param collName Collection 名称
     * @param vector   向量数据
     * @returns 分配的向量 ID
     */
    insert(collName: string, vector: number[]): number;
    /**
     * 向量相似度搜索
     * @param collName Collection 名称
     * @param query    查询向量
     * @param k        返回结果数
     */
    search(collName: string, query: number[], k?: number): SearchResult[];
    /**
     * 关闭数据库（文件模式会自动落盘）
     */
    close(): void;
    /**
     * 数据库是否已打开
     */
    get isOpen(): boolean;
    /**
     * 获取版本信息
     */
    version(): string;
}
/**
 * 打开或创建数据库
 *
 * - 传 '' : 内存模式，浏览器和 Node.js 均可用
 * - 传路径: 文件持久化模式，仅 Node.js（NODEFS）
 *
 * @param path 空字符串为内存模式，非空为文件路径
 * @returns JinWoDB 实例
 *
 * @example
 * ```ts
 * const db = await open('');            // 内存
 * const db = await open('/data/my_db'); // 文件持久化
 * ```
 */
export declare function open(path?: string): Promise<JinWoDB>;
declare const _default: {
    open: typeof open;
    JinWoDB: typeof JinWoDB;
    Collection: typeof Collection;
};
export default _default;
//# sourceMappingURL=index.d.ts.map