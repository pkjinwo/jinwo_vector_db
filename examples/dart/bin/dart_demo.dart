import 'dart:ffi';
import 'dart:io';

// 定义搜索结果结构体
class SearchResult extends Struct {
  @Uint64()
  external int id;
  @Float()
  external double distance;
}

// 定义FFI接口
typedef JwVecdbOpen = Int32 Function(
    Pointer<Pointer<Void>> db, Pointer<Utf8> path, Int32 create);
typedef JwVecdbOpenDart = int Function(
    Pointer<Pointer<Void>> db, Pointer<Utf8> path, int create);

typedef JwVecdbClose = Int32 Function(Pointer<Void> db);
typedef JwVecdbCloseDart = int Function(Pointer<Void> db);

typedef JwVersion = Pointer<Utf8> Function();
typedef JwVersionDart = Pointer<Utf8> Function();

typedef JwCollectionCreate = Int32 Function(
    Pointer<Pointer<Void>> collection, Pointer<Void> db, Pointer<Utf8> name, Int32 dimension);
typedef JwCollectionCreateDart = int Function(
    Pointer<Pointer<Void>> collection, Pointer<Void> db, Pointer<Utf8> name, int dimension);

typedef JwCollectionClose = Int32 Function(Pointer<Void> collection);
typedef JwCollectionCloseDart = int Function(Pointer<Void> collection);

typedef JwCollectionList = Int32 Function(
    Pointer<Void> db, Pointer<Pointer<Pointer<Utf8>>> names, Pointer<Size> count);
typedef JwCollectionListDart = int Function(
    Pointer<Void> db, Pointer<Pointer<Pointer<Utf8>>> names, Pointer<Size> count);

typedef JwCollectionInsert = Int32 Function(
    Pointer<Void> collection, Pointer<Float> vector, Pointer<Uint64> id);
typedef JwCollectionInsertDart = int Function(
    Pointer<Void> collection, Pointer<Float> vector, Pointer<Uint64> id);
typedef JwCollectionSearch = Int32 Function(
    Pointer<Void> collection, Pointer<Float> query, Size k, Pointer<Pointer<Pointer<SearchResult>>> results, Pointer<Size> count);
typedef JwCollectionSearchDart = int Function(
    Pointer<Void> collection, Pointer<Float> query, int k, Pointer<Pointer<Pointer<SearchResult>>> results, Pointer<Size> count);

void main() {
  print('JinWo VecDB Dart Demo');
  print('====================');

  // 加载库
  String libraryPath;
  if (Platform.isLinux) {
    libraryPath = '../../build/libjw_vecdb.so';
  } else if (Platform.isWindows) {
    libraryPath = '../../build/jw_vecdb.dll';
  } else if (Platform.isMacOS) {
    libraryPath = '../../build/libjw_vecdb.dylib';
  } else {
    throw UnsupportedError('Unsupported platform');
  }

  print('Using library: $libraryPath');

  try {
    final dylib = DynamicLibrary.open(libraryPath);

    // 1. 测试版本信息
    final jwVersion = dylib.lookupFunction<JwVersion, JwVersionDart>('jw_version');
    final versionPtr = jwVersion();
    final version = versionPtr.toDartString();
    print('Version: $version');

    // 2. 打开数据库
    final jwVecdbOpen = dylib.lookupFunction<JwVecdbOpen, JwVecdbOpenDart>('jw_vecdb_open');
    final dbPtr = calloc<Pointer<Void>>();
    final dbPath = 'test_db_dart'.toNativeUtf8();
    final create = 1; // 创建新数据库

    final result = jwVecdbOpen(dbPtr, dbPath, create);
    if (result != 0) {
      print('Failed to open database, error: $result');
      calloc.free(dbPath);
      calloc.free(dbPtr);
      return;
    }

    final db = dbPtr.value;
    print('Database opened successfully');

    // 3. 创建集合
    final jwCollectionCreate = dylib.lookupFunction<JwCollectionCreate, JwCollectionCreateDart>('jw_collection_create');
    final collectionPtr = calloc<Pointer<Void>>();
    final collectionName = 'test_collection'.toNativeUtf8();
    final dimension = 128;

    final result2 = jwCollectionCreate(collectionPtr, db, collectionName, dimension);
    if (result2 != 0) {
      print('Failed to create collection, error: $result2');
      dylib.lookupFunction<JwVecdbClose, JwVecdbCloseDart>('jw_vecdb_close')(db);
      calloc.free(dbPath);
      calloc.free(dbPtr);
      calloc.free(collectionName);
      calloc.free(collectionPtr);
      return;
    }

    final collection = collectionPtr.value;
    print('Collection created successfully');

    // 4. 插入向量
    final jwCollectionInsert = dylib.lookupFunction<JwCollectionInsert, JwCollectionInsertDart>('jw_collection_insert');
    final vectors = <List<double>>[];
    for (var i = 0; i < 5; i++) {
      final vector = List<double>.generate(dimension, (j) => i * dimension + j.toDouble());
      vectors.add(vector);
    }

    for (var i = 0; i < vectors.length; i++) {
      final vector = vectors[i];
      final vectorPtr = calloc<Float>(dimension);
      for (var j = 0; j < dimension; j++) {
        vectorPtr[j] = vector[j];
      }
      final idPtr = calloc<Uint64>();

      final result3 = jwCollectionInsert(collection, vectorPtr, idPtr);
      if (result3 != 0) {
        print('Failed to insert vector $i, error: $result3');
      } else {
        final id = idPtr.value;
        print('Inserted vector $i with ID: $id');
      }

      calloc.free(vectorPtr);
      calloc.free(idPtr);
    }

    // 5. 搜索向量
    final jwCollectionSearch = dylib.lookupFunction<JwCollectionSearch, JwCollectionSearchDart>('jw_collection_search');
    final queryVector = vectors[0];
    final queryPtr = calloc<Float>(dimension);
    for (var j = 0; j < dimension; j++) {
      queryPtr[j] = queryVector[j];
    }
    final k = 3;
    final resultsPtr = calloc<Pointer<Pointer<SearchResult>>>();
    final countPtr = calloc<Size>();

    final result4 = jwCollectionSearch(collection, queryPtr, k, resultsPtr, countPtr);
    if (result4 != 0) {
      print('Failed to search, error: $result4');
    } else {
      final count = countPtr.value;
      print('Search results (top $count):');
      final results = resultsPtr.value;
      for (var i = 0; i < count; i++) {
        final result = results[i];
        print('  ID: ${result.ref.id}, Distance: ${result.ref.distance.toStringAsFixed(4)}');
      }
    }

    // 6. 列出集合
    final jwCollectionList = dylib.lookupFunction<JwCollectionList, JwCollectionListDart>('jw_collection_list');
    final namesPtr = calloc<Pointer<Pointer<Utf8>>>();
    final countPtr2 = calloc<Size>();

    final result5 = jwCollectionList(db, namesPtr, countPtr2);
    if (result5 != 0) {
      print('Failed to list collections, error: $result5');
    } else {
      final count = countPtr2.value;
      print('Collections ($count):');
      final names = namesPtr.value;
      for (var i = 0; i < count; i++) {
        final name = names[i].toDartString();
        print('  $name');
      }
    }

    // 7. 清理资源
    final jwCollectionClose = dylib.lookupFunction<JwCollectionClose, JwCollectionCloseDart>('jw_collection_close');
    final jwVecdbClose = dylib.lookupFunction<JwVecdbClose, JwVecdbCloseDart>('jw_vecdb_close');
    jwCollectionClose(collection);
    jwVecdbClose(db);

    // 释放内存
    calloc.free(dbPath);
    calloc.free(dbPtr);
    calloc.free(collectionName);
    calloc.free(collectionPtr);
    calloc.free(queryPtr);
    calloc.free(resultsPtr);
    calloc.free(countPtr);
    calloc.free(namesPtr);
    calloc.free(countPtr2);

    print('Demo completed successfully');

  } catch (e) {
    print('Error: $e');
  }
}
