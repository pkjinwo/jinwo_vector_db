// JinWo VecDB Node.js 演示（使用 npm 包 jinwo-vecdb，WASM 实现，高层 API）
// 运行前： npm install jinwo-vecdb
// 运行：   node nodejs_demo.js

import { open } from 'jinwo-vecdb';

function randomVector(dim) {
  return Array.from({ length: dim }, () => Math.random() * 2 - 1);
}

async function main() {
  console.log('========================================');
  console.log('  JinWo VecDB Node.js Demo');
  console.log('========================================');

  // 打开/创建数据库（路径为空字符串表示内存模式）
  const db = await open('./vecdb_nodejs');
  console.log('version:', db.version());

  // 创建 Collection（128 维）
  const coll = db.createCollection('test', 128);

  // 插入向量
  for (let i = 0; i < 10; i++) {
    const id = coll.insert(randomVector(128));
    console.log(`inserted vector ${i}, id=${id}`);
  }

  // 构建索引，加速检索
  coll.buildIndex();

  // 相似度搜索 Top-5
  const query = randomVector(128);
  const results = db.search('test', query, 5);
  console.log('search results (top 5):');
  results.forEach(r => console.log(`  id=${r.id}, score=${r.score}`));

  // 删除一条向量
  if (results.length > 0) coll.delete(results[0].id);

  db.close();
  console.log('Demo completed successfully!');
}

main().catch(err => {
  console.error('Error:', err.message);
  process.exit(1);
});
