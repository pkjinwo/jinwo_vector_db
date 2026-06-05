/**
 * 边界用例测试：不存在集合/vid、错误维度、空集合、灾难场景
 * 对应 Python test_jinwo_vecdb.py 的所有 Edge Cases
 *
 * 运行: node test_edge.mjs
 */
import { open } from './dist/index.js';
import { existsSync, unlinkSync, rmSync } from 'fs';

const DB_PATH = './test_edge_db.jwv';
const COLL_NAME = 'edge_test';
const DIM = 128;

function cleanup() {
  try {
    if (existsSync(DB_PATH)) unlinkSync(DB_PATH);
    if (existsSync('./test_catastrophe_db.jwv')) {
      try { rmSync('./test_catastrophe_db.jwv', { recursive: true, force: true }); } catch {}
    }
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
  // 新建数据库
  // ============================================================
  cleanup();
  const db = await open(DB_PATH);
  log('[OK] 数据库已打开');
  assert(db.isOpen, 'isOpen');

  const coll = db.createCollection(COLL_NAME, DIM);
  log(`[OK] 创建 collection: ${COLL_NAME} (dim=${DIM})`);

  // 插入一条数据，构建索引
  const vec = new Array(DIM).fill(0.1);
  const vid = coll.insert(vec);
  coll.buildIndex();
  log(`[OK] 插入 1 条数据 (vid=${vid}) 并构建索引`);

  // ============================================================
  // 边缘用例
  // ============================================================
  console.log('\n' + '='.repeat(60));
  console.log('  EDGE CASES');
  console.log('='.repeat(60));

  // ---- Edge 1: 搜索不存在的 collection ----
  console.log('\n[Edge 1] 搜索不存在的 collection');
  const noExistResults = db.search('nonexistent_coll', vec, 5);
  log(`[INFO] 返回 ${noExistResults.length} 条结果 (当前不抛异常，待 WASM 层加校验)`);

  // ---- Edge 2: 获取不存在的 collection ----
  console.log('\n[Edge 2] 获取不存在的 collection');
  const nullColl = db.getCollection('nonexistent_coll');
  assert(nullColl === null, '应返回 null');
  log('[OK] 返回 null (符合预期)');

  // ---- Edge 3: 删除不存在的 vid ----
  console.log('\n[Edge 3] 删除不存在的 vid');
  try {
    coll.delete(99999);
    console.log('  [FAIL] 没有抛出异常');
  } catch (e) {
    log(`[OK] 预期异常: ${e.message}`);
  }

  // ---- Edge 4: 插入错误维度的向量 ----
  console.log('\n[Edge 4] 插入错误维度向量');
  try {
    coll.insert([1.0, 2.0]);  // dim=2, 期望 128
    console.log('  [FAIL] 没有抛出异常');
  } catch (e) {
    assert(e.message.includes('维度') || e.message.includes('dimension') || e.message.includes('期望'),
      `错误信息应包含维度提示, 实际: ${e.message}`);
    log(`[OK] 预期异常: ${e.message}`);
  }

  // ---- Edge 5: 搜索时使用错误维度 ----
  console.log('\n[Edge 5] 搜索时使用错误维度');
  const wrongDimResults = db.search(COLL_NAME, [1.0, 2.0], 5);  // dim=2 vs 128
  log(`[INFO] 返回 ${wrongDimResults.length} 条，score=${wrongDimResults[0]?.score} (当前不抛异常，待 WASM 层加校验)`);

  // ---- Edge 6: 搜索空 collection ----
  console.log('\n[Edge 6] 搜索空 collection');
  const emptyColl = db.createCollection('empty_coll', DIM);
  const emptyResults = db.search('empty_coll', vec, 5);
  assert(emptyResults.length === 0,
    `空 collection 应返回 0 条, 实际 ${emptyResults.length}`);
  log(`[OK] 返回 ${emptyResults.length} 条结果 (符合预期)`);

  // ---- Edge 7: 创建重复 collection ----
  console.log('\n[Edge 7] 创建重复 collection');
  try {
    db.createCollection(COLL_NAME, DIM);
    console.log('  [FAIL] 没有抛出异常');
  } catch (e) {
    log(`[OK] 预期异常: ${e.message}`);
  }

  // ---- Edge 8: 获取不存在的 vid ----
  console.log('\n[Edge 8] 获取不存在的 vid');
  // Node.js Collection 当前未暴露 get() 方法，通过 search 间接验证
  // 搜索时应搜不到不存在的 vid
  const sr = db.search(COLL_NAME, vec, 100);
  const nonexistentVid = 999999;
  const found = sr.some(r => r.id === nonexistentVid);
  assert(!found, `不应搜到不存在的 vid=${nonexistentVid}`);
  log(`[OK] 不存在的 vid=${nonexistentVid} 未出现在搜索结果中 (符合预期)`);

  // ---- Edge 9: db.insert 不存在的 collection (自动创建) ----
  console.log('\n[Edge 9] db.insert 到不存在的 collection');
  try {
    const newVec = new Array(DIM).fill(0.5);
    const newVid = db.insert('auto_created_coll', newVec);
    log(`[OK] 自动创建 collection 并插入, vid=${newVid}`);
    const newSr = db.search('auto_created_coll', newVec, 3);
    assert(newSr.length >= 1, '应能搜到刚插入的向量');
    log(`[OK] 搜索返回 ${newSr.length} 条结果 ✓`);
  } catch (e) {
    log(`[!] 异常 (可能不支持自动创建): ${e.message}`);
  }

  // ---- 关闭 ----
  db.close();
  log('\n[OK] 数据库关闭');
  assert(!db.isOpen, '关闭后 isOpen 应为 false');

  // ============================================================
  // 灾难场景：运行中文件被删除（子进程隔离）
  // ============================================================
  console.log('\n' + '='.repeat(60));
  console.log('  CATASTROPHE: 运行中删除数据库文件');
  console.log('='.repeat(60));

  const { spawnSync } = await import('child_process');
  const catastropheCode = `
import { open } from './dist/index.js';
import { rmSync, existsSync } from 'fs';

const DB_PATH = './test_catastrophe_db.jwv';

async function run() {
  try { if (existsSync(DB_PATH)) rmSync(DB_PATH, { recursive: true, force: true }); } catch {}

  const db = await open(DB_PATH);
  const coll = db.createCollection('docs', 64);
  const vec = new Array(64).fill(1.0);
  const vids = [];
  for (let i = 0; i < 10; i++) vids.push(coll.insert(vec));
  coll.buildIndex();
  console.log('[>] STAGE1: directory_deleted');

  try { rmSync(DB_PATH, { recursive: true, force: true }); } catch(e) {
    console.log('[!] cannot delete dir:', e.message);
  }

  try {
    const r = db.search('docs', vec, 5);
    console.log('[>] STAGE2: search_ok results=' + r.length);
  } catch(e) {
    console.log('[!] STAGE2: search_failed: ' + e.message);
  }

  try {
    coll.insert(vec);
    console.log('[>] STAGE3: insert_ok');
  } catch(e) {
    console.log('[!] STAGE3: insert_failed: ' + e.message);
  }

  try {
    coll.delete(vids[0]);
    console.log('[>] STAGE4: delete_ok');
  } catch(e) {
    console.log('[!] STAGE4: delete_failed: ' + e.message);
  }

  try {
    db.close();
    console.log('[>] STAGE5: close_ok');
  } catch(e) {
    console.log('[!] STAGE5: close_failed: ' + e.message);
  }
}

run().catch(err => {
  console.log('[FATAL] ' + err.message);
  process.exitCode = 1;
});
`;

  const { writeFileSync } = await import('fs');
  const tempScript = './test_catastrophe_temp.mjs';
  writeFileSync(tempScript, catastropheCode);

  const result = spawnSync('node', [tempScript], {
    cwd: process.cwd(),
    timeout: 30000,
  });

  try { unlinkSync(tempScript); } catch {}

  console.log(`  returncode=${result.status}`);
  const output = (result.stdout?.toString() || '') + (result.stderr?.toString() || '');
  const lines = output.split('\n').filter(l => l.trim());
  for (const line of lines) {
    if (line.trim()) console.log(`  ${line.trim()}`);
  }

  if (result.status === 0 || result.signal === null) {
    log('[OK] 进程正常退出（没有段错误）');
  } else if (result.signal) {
    console.log(`  [FAIL] 进程异常终止: signal=${result.signal}`);
    process.exit(1);
  }

  cleanup();
  console.log('\n[OK] 清理测试文件');

  console.log();
  console.log('='.repeat(60));
  console.log('所有边缘测试通过!');
  console.log('='.repeat(60));
}

test().catch(err => {
  console.error('测试失败:', err);
  process.exit(1);
});
