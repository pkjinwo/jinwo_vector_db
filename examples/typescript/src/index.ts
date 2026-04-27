import * as ffi from 'ffi-napi';
import * as ref from 'ref-napi';
import * as Struct from 'ref-struct-napi';

// 定义搜索结果结构体
const SearchResult = Struct({
  id: ref.types.uint64,
  distance: ref.types.float
});

// 定义类型
const SearchResultPtr = ref.refType(SearchResult);
const SearchResultPtrPtr = ref.refType(SearchResultPtr);
const CharPtr = ref.types.CString;
const CharPtrPtr = ref.refType(CharPtr);

// 加载库
let libraryPath: string;
switch (process.platform) {
  case 'linux':
    libraryPath = '../../build/libjw_vecdb.so';
    break;
  case 'win32':
    libraryPath = '../../build/jw_vecdb.dll';
    break;
  case 'darwin':
    libraryPath = '../../build/libjw_vecdb.dylib';
    break;
  default:
    throw new Error('Unsupported platform');
}

console.log(`Using library: ${libraryPath}`);

// 定义FFI接口
const jwVecDB = ffi.Library(libraryPath, {
  jw_vecdb_open: ['int', ['pointer', 'string', 'int']],
  jw_vecdb_close: ['int', ['pointer']],
  jw_version: ['string', []],
  jw_collection_create: ['int', ['pointer', 'pointer', 'string', 'int']],
  jw_collection_close: ['int', ['pointer']],
  jw_collection_list: ['int', ['pointer', 'pointer', 'pointer']],
  jw_collection_insert: ['int', ['pointer', 'pointer', 'pointer']],
  jw_collection_search: ['int', ['pointer', 'pointer', 'size_t', 'pointer', 'pointer']]
});

function main() {
  console.log('JinWo VecDB TypeScript Demo');
  console.log('=========================');

  try {
    // 1. 测试版本信息
    const version = jwVecDB.jw_version();
    console.log(`Version: ${version}`);

    // 2. 打开数据库
    const dbPtr = ref.alloc(ref.refType(ref.types.void));
    const dbPath = 'test_db_typescript';
    const create = 1; // 创建新数据库

    const result = jwVecDB.jw_vecdb_open(dbPtr, dbPath, create);
    if (result !== 0) {
      console.log(`Failed to open database, error: ${result}`);
      return;
    }

    const db = ref.readPointer(dbPtr);
    console.log('Database opened successfully');

    // 3. 创建集合
    const collectionPtr = ref.alloc(ref.refType(ref.types.void));
    const collectionName = 'test_collection';
    const dimension = 128;

    const result2 = jwVecDB.jw_collection_create(collectionPtr, db, collectionName, dimension);
    if (result2 !== 0) {
      console.log(`Failed to create collection, error: ${result2}`);
      jwVecDB.jw_vecdb_close(db);
      return;
    }

    const collection = ref.readPointer(collectionPtr);
    console.log('Collection created successfully');

    // 4. 插入向量
    const vectors: number[][] = [];
    for (let i = 0; i < 5; i++) {
      const vector: number[] = [];
      for (let j = 0; j < dimension; j++) {
        vector.push(i * dimension + j);
      }
      vectors.push(vector);
    }

    for (let i = 0; i < vectors.length; i++) {
      const vector = vectors[i];
      const vectorBuffer = Buffer.from(new Float32Array(vector).buffer);
      const idPtr = ref.alloc('uint64');

      const result3 = jwVecDB.jw_collection_insert(collection, vectorBuffer, idPtr);
      if (result3 !== 0) {
        console.log(`Failed to insert vector ${i}, error: ${result3}`);
      } else {
        const id = ref.readUInt64BE(idPtr, 0);
        console.log(`Inserted vector ${i} with ID: ${id}`);
      }
    }

    // 5. 搜索向量
    const queryVector = vectors[0];
    const queryBuffer = Buffer.from(new Float32Array(queryVector).buffer);
    const k = 3;
    const resultsPtr = ref.alloc(SearchResultPtrPtr);
    const countPtr = ref.alloc('size_t');

    const result4 = jwVecDB.jw_collection_search(collection, queryBuffer, k, resultsPtr, countPtr);
    if (result4 !== 0) {
      console.log(`Failed to search, error: ${result4}`);
    } else {
      const count = ref.readUInt64BE(countPtr, 0);
      console.log(`Search results (top ${count}):`);
      const results = ref.readPointer(resultsPtr);
      for (let i = 0; i < count; i++) {
        const resultPtr = ref.readPointer(results, i * ref.sizeof.pointer);
        const searchResult = ref.readStruct(resultPtr, 0, SearchResult);
        console.log(`  ID: ${searchResult.id}, Distance: ${searchResult.distance.toFixed(4)}`);
      }
    }

    // 6. 列出集合
    const namesPtr = ref.alloc(CharPtrPtr);
    const countPtr2 = ref.alloc('size_t');

    const result5 = jwVecDB.jw_collection_list(db, namesPtr, countPtr2);
    if (result5 !== 0) {
      console.log(`Failed to list collections, error: ${result5}`);
    } else {
      const count = ref.readUInt64BE(countPtr2, 0);
      console.log(`Collections (${count}):`);
      const names = ref.readPointer(namesPtr);
      for (let i = 0; i < count; i++) {
        const namePtr = ref.readPointer(names, i * ref.sizeof.pointer);
        const name = ref.readCString(namePtr);
        console.log(`  ${name}`);
      }
    }

    // 7. 清理资源
    jwVecDB.jw_collection_close(collection);
    jwVecDB.jw_vecdb_close(db);

    console.log('Demo completed successfully');

  } catch (error) {
    console.error(`Error: ${error}`);
  }
}

main();
