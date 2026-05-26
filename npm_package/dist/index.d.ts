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
     * 关闭数据库，保存数据到磁盘
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
export declare function open(path?: string): Promise<JinWoDB>;
declare const _default: {
    open: typeof open;
    JinWoDB: typeof JinWoDB;
    Collection: typeof Collection;
};
export default _default;
//# sourceMappingURL=index.d.ts.map