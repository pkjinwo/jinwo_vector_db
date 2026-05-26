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
  const vec = new Array(384).fill(0.1);
  const vid = coll.insert(vec);
  console.log('\n3. 插入向量: vid =', vid);

  // 4. 构建索引
  coll.buildIndex();
  console.log('\n4. 构建索引完成');

  // 5. 搜索
  const results = db.search('test_docs', vec, 5);
  console.log('\n5. 搜索结果:');
  for (const r of results) {
    console.log(`   id=${r.id}, score=${r.score.toFixed(6)}`);
  }

  // 6. 删除向量
  coll.delete(vid);
  console.log('\n6. 删除向量: vid =', vid);

  // 7. 关闭
  db.close();
  console.log('\n7. 数据库已关闭');
  console.log('\n=== 所有测试通过! ===');
}

test().catch(err => {
  console.error('测试失败:', err);
  process.exit(1);
});
