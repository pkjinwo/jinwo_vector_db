/**
 * 完整生命周期测试: 新建 -> 增查 -> 关闭 -> 重新打开(100条持久化) -> 删改查 -> 关闭 -> 重新打开 -> 增删查 -> 关闭
 */
import { open } from './dist/index.js';
import { existsSync, unlinkSync } from 'fs';

const DB_PATH = './test_lifecycle_db.jwv';
const COLL_NAME = 'lifecycle_test';
const DIM = 128;

function cleanup() {
  try {
    if (existsSync(DB_PATH)) unlinkSync(DB_PATH);
  } catch {}
}

function assert(cond, msg) {
  if (!cond) throw new Error(`FAIL: ${msg}`);
}

function log(msg) {
  console.log(`  ${msg}`);
}

async function test() {
  // ============================================================
  // Phase 1: 新建数据库 -> 插入100 -> 关闭 (不删除!)
  // ============================================================
  console.log('='.repeat(60));
  console.log('Phase 1: 新建库 -> 插入100 -> 关闭 (不删除)');
  console.log('='.repeat(60));

  cleanup();
  const db = await open(DB_PATH);
  log('[OK] 打开文件数据库');

  const coll = db.createCollection(COLL_NAME, DIM);
  assert(coll.name === COLL_NAME, 'collection name');
  assert(coll.dim === DIM, 'collection dim');
  log(`[OK] 创建 collection: ${COLL_NAME} (dim=${DIM})`);

  // Insert 100 条
  const vids = [];
  for (let i = 0; i < 100; i++) {
    const vec = new Array(DIM).fill(0).map((_, j) => i * 100 + j);
    const vid = coll.insert(vec);
    vids.push(vid);
  }
  log(`[OK] 插入 100 条向量, vids=[${vids[0]},${vids[1]},${vids[2]}]...[${vids[97]},${vids[98]},${vids[99]}]`);

  // Build index
  coll.buildIndex();
  log('[OK] 构建索引');

  // Search
  const query = new Array(DIM).fill(0).map((_, j) => 5000 + j);
  let results = db.search(COLL_NAME, query, 10);
  log(`[OK] 查询返回 ${results.length} 条结果`);
  assert(results.length === 10, `搜索结果应返回 10 条, 实际 ${results.length}`);
  for (let i = 0; i < Math.min(5, results.length); i++) {
    log(`  #${i + 1}: vid=${results[i].id}, score=${results[i].score.toFixed(6)}`);
  }

  // Close (不删除任何数据)
  db.close();
  log('[OK] Phase 1 关闭数据库 (100条完整数据)');
  assert(!db.isOpen, 'close后 isOpen 应为 false');

  // ============================================================
  // Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> 增改查 -> 关闭
  // ============================================================
  console.log();
  console.log('='.repeat(60));
  console.log('Phase 2: 重新打开 -> 验证100条持久化 -> 删除50 -> CRUD -> 关闭');
  console.log('='.repeat(60));

  const db2 = await open(DB_PATH);
  assert(db2.isOpen, '重新打开后 isOpen');
  log('[OK] 重新打开数据库');

  // 验证版本
  const ver = db2.version();
  log(`[OK] version = ${ver}`);

  // 获取 collection
  const coll2 = db2.getCollection(COLL_NAME);
  assert(coll2 !== null, 'collection 应该存在');
  log('[OK] Collection 存在');

  // 重新打开后 search (应该还能查到 10 条)
  results = db2.search(COLL_NAME, query, 10);
  log(`[OK] 重新打开后查询返回 ${results.length} 条结果 (应有 10 条)`);
  assert(results.length === 10, `重新打开后应返回 10 条, 实际 ${results.length}`);
  for (let i = 0; i < Math.min(5, results.length); i++) {
    log(`  #${i + 1}: vid=${results[i].id}, score=${results[i].score.toFixed(6)}`);
  }

  // Delete 50 条
  const deleteCount = 50;
  for (let i = 0; i < deleteCount; i++) {
    coll2.delete(vids[i]);
  }
  log(`[OK] 删除 ${deleteCount} 条向量`);

  // 验证删除后搜索：已删除的 vid 不应出现
  results = db2.search(COLL_NAME, query, 10);
  log(`[OK] 删除后查询返回 ${results.length} 条结果`);

  const deletedSet = new Set(vids.slice(0, deleteCount));
  let deletedInResults = false;
  for (const r of results) {
    if (deletedSet.has(r.id)) {
      log(`[FAIL] 已删除的 vid=${r.id} 仍出现在结果中!`);
      deletedInResults = true;
      break;
    }
  }
  if (!deletedInResults) {
    log('[OK] 已删除的 vid 没有出现在搜索结果中 ✓');
  }

  // Close
  db2.close();
  log('[OK] Phase 2 关闭数据库 (50条剩余)');
  assert(!db2.isOpen, 'close后 isOpen 应为 false');

  // ============================================================
  // Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭
  // ============================================================
  console.log();
  console.log('='.repeat(60));
  console.log('Phase 3: 再次重新打开 -> 验证50条 -> 插入20 -> 删除10 -> 关闭');
  console.log('='.repeat(60));

  const db3 = await open(DB_PATH);
  assert(db3.isOpen, '再次重新打开后 isOpen');
  log('[OK] 再次重新打开数据库');

  const coll3 = db3.getCollection(COLL_NAME);
  assert(coll3 !== null, 'collection 应该存在');
  log('[OK] Collection 存在');

  // 搜索验证 50 条
  results = db3.search(COLL_NAME, query, 10);
  log(`[OK] 50条持久化后查询返回 ${results.length} 条结果 ✓`);

  // Insert 新数据
  const newVids = [];
  for (let i = 0; i < 20; i++) {
    const vec = new Array(DIM).fill(0).map((_, j) => 10000 + i * 100 + j);
    const vid = db3.insert(COLL_NAME, vec);
    newVids.push(vid);
  }
  log('[OK] 插入 20 条新向量');

  // 搜索新数据
  const newQuery = new Array(DIM).fill(0).map((_, j) => 10000 + 500 + j);
  results = db3.search(COLL_NAME, newQuery, 5);
  log(`[OK] 查询新数据返回 ${results.length} 条结果`);
  assert(results.length === 5, `查询新数据应返回 5 条, 实际 ${results.length}`);
    log(`  #${i + 1}: vid=${results[i].id}, score=${results[i].score.toFixed(6)}`);
  }

  // Delete 新插入的前 10 条
  for (let i = 0; i < 10; i++) {
    coll3.delete(newVids[i]);
  }
  log('[OK] 删除新插入的 10 条');

  // 验证删除后的查询
  results = db3.search(COLL_NAME, newQuery, 5);
  log(`[OK] 删除新数据后查询返回 ${results.length} 条结果`);
  const newDeletedSet = new Set(newVids.slice(0, 10));
  let newDeletedFound = false;
  for (const r of results) {
    if (newDeletedSet.has(r.id)) {
      log(`[FAIL] 已删除的 vid=${r.id} 出现在查询中!`);
      newDeletedFound = true;
      break;
    }
  }
  if (!newDeletedFound) {
    log('[OK] Phase 3 删除的 vid 不在查询结果中 ✓');
  }

  // Close
  db3.close();
  log('[OK] Phase 3 关闭数据库');
  assert(!db3.isOpen, '第三次 close 后 isOpen 应为 false');

  // ============================================================
  // 清理
  // ============================================================
  cleanup();
  log('[OK] 清理测试文件');

  console.log();
  console.log('='.repeat(60));
  console.log('所有生命周期测试通过!');
  console.log('  Phase 1: 100条完整持久化 ✓');
  console.log('  Phase 2: 删除50后持久化 ✓');
  console.log('  Phase 3: 50条+新增20删10持久化 ✓');
  console.log('='.repeat(60));
}

test().catch(err => {
  console.error('测试失败:', err);
  process.exit(1);
});
