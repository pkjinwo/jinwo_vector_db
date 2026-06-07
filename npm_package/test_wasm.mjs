/**
 * 快速验证 JinWo VecDB WASM 模块是否正常加载
 *
 * 运行: node test_wasm.js
 */
import { open } from './dist/index.js';

async function test() {
  console.log('=== JinWo VecDB WASM 测试 ===\n');

  // 1. 版本信息
  const db = await open('');  // 内存数据库
  console.log('1. 数据库已打开 (内存模式)');
  console.log('   版本:', db.version());
  console.log('   isOpen:', db.isOpen);

  // 2. 创建 Collection
  const coll = db.createCollection('test_docs', 384);
  console.log('\n2. 创建 Collection: test_docs (dim=384)');

  // 3. 插入向量
  for (let i = 0; i < 20; i++) {
    const vec = new Array(384).fill(0).map(() => Math.random());
    coll.insert(vec);
  }
  console.log('\n3. 插入 20 个向量');

  // 4. 构建索引
  coll.buildIndex();
  console.log('\n4. 构建索引完成');

  // 5. 搜索
  const queryVec = new Array(384).fill(0).map(() => Math.random());
  const results = db.search('test_docs', queryVec, 5);
  console.log('\n5. 搜索返回', results.length, '条结果 (预期 5 条)');
  assert(results.length === 5, `搜索应返回 5 条, 实际 ${results.length}`);

  for (const r of results) {
    console.log(`   id=${r.id}, score=${r.score.toFixed(6)}`);
  }

  // 6. 删除向量（删除第一个搜索结果）
  const vidToDelete = results[0].id;
  coll.delete(vidToDelete);
  console.log('\n6. 删除向量: vid =', vidToDelete);

  // 7. 关闭
  db.close();
  console.log('\n7. 数据库已关闭');
  console.log('\n=== 所有测试通过! ===');
}

test().catch(err => {
  console.error('测试失败:', err);
  process.exit(1);
});
