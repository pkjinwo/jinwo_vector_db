# JinWo VecDB iOS 平台集成指南

**版本**：v1.0.0
**生成时间**：2026-04-26
**文档类型**：对外发布

---

## 1. 平台概述

### 1.1 支持的iOS版本

| iOS版本 | 支持状态 | 最低SDK版本 |
|---------|---------|------------|
| iOS 13.0+ | ✅ 支持 | iOS 13.0 |
| iOS 14.0+ | ✅ 支持 | iOS 14.0 |
| iOS 15.0+ | ✅ 支持 | iOS 15.0 |
| iOS 16.0+ | ✅ 支持 | iOS 16.0 |
| iOS 17.0+ | ✅ 支持 | iOS 17.0 |

### 1.2 系统要求

| 组件 | 要求 |
|------|------|
| 设备 | iPhone 6s及以上，iPad Air 2及以上 |
| CPU | 64位处理器 |
| 内存 | 至少 2GB RAM |
| 存储 | 至少 100MB 可用空间 |
| Xcode | Xcode 15.0+ |
| Swift | Swift 5.0+ |
| Objective-C | Objective-C 2.0+ |

---

## 2. 集成方式

### 2.1 CocoaPods 集成

1. **安装 CocoaPods**
   ```bash
   sudo gem install cocoapods
   ```

2. **创建 Podfile**
   ```ruby
   # Podfile
   platform :ios, '13.0'
   use_frameworks!
   
   target 'YourApp' do
     pod 'JinWoVecDB', '~> 1.0.0'
   end
   ```

3. **安装依赖**
   ```bash
   pod install
   ```

4. **打开项目**
   - 使用 `.xcworkspace` 文件打开项目

### 2.2 手动集成

1. **下载框架**
   - 从 GitHub Release 页面下载 `JinWoVecDB.framework`

2. **添加到项目**
   - 将 `JinWoVecDB.framework` 拖放到 Xcode 项目的 "Frameworks, Libraries, and Embedded Content" 部分
   - 选择 "Copy items if needed"
   - 设置 "Embed" 为 "Embed & Sign"

3. **添加系统依赖**
   - 在 "Link Binary With Libraries" 中添加 `libc++.tbd`

---

## 3. API 封装

### 3.1 Swift 封装类

```swift
// JinWoVecDB.swift
import Foundation

public class JinWoVecDB {
    private let dbPointer: UnsafeMutableRawPointer
    
    // 初始化
    public init(path: String, create: Bool = true) throws {
        var ptr: UnsafeMutableRawPointer?
        let result = jw_vecdb_open(&ptr, path, create)
        guard result == 0, let pointer = ptr else {
            throw VecDBError(code: result)
        }
        dbPointer = pointer
    }
    
    // 关闭
    deinit {
        jw_vecdb_close(dbPointer)
    }
    
    // 创建集合
    public func createCollection(name: String, dimension: Int) throws -> Collection {
        var colPtr: UnsafeMutableRawPointer?
        let result = jw_collection_create(&colPtr, dbPointer, name, dimension)
        guard result == 0, let pointer = colPtr else {
            throw VecDBError(code: result)
        }
        return Collection(pointer: pointer)
    }
    
    // 打开集合
    public func openCollection(name: String) throws -> Collection {
        var colPtr: UnsafeMutableRawPointer?
        let result = jw_collection_open(&colPtr, dbPointer, name)
        guard result == 0, let pointer = colPtr else {
            throw VecDBError(code: result)
        }
        return Collection(pointer: pointer)
    }
    
    // 删除集合
    public func dropCollection(name: String) throws {
        let result = jw_collection_drop(dbPointer, name)
        guard result == 0 else {
            throw VecDBError(code: result)
        }
    }
    
    // 列出集合
    public func listCollections() throws -> [String] {
        var names: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>?
        var count: size_t = 0
        let result = jw_collection_list(dbPointer, &names, &count)
        guard result == 0, let collectionNames = names else {
            throw VecDBError(code: result)
        }
        
        defer {
            for i in 0..<count {
                if let name = collectionNames[Int(i)] {
                    free(name)
                }
            }
            free(collectionNames)
        }
        
        var resultArray: [String] = []
        for i in 0..<count {
            if let name = collectionNames[Int(i)] {
                let string = String(cString: name)
                resultArray.append(string)
            }
        }
        return resultArray
    }
}

// 集合类
public class Collection {
    private let collectionPointer: UnsafeMutableRawPointer
    
    init(pointer: UnsafeMutableRawPointer) {
        collectionPointer = pointer
    }
    
    deinit {
        jw_collection_close(collectionPointer)
    }
    
    // 插入向量
    public func insert(vector: [Float]) throws -> UInt64 {
        var id: UInt64 = 0
        let result = vector.withUnsafeBufferPointer { buffer in
            jw_collection_insert(collectionPointer, buffer.baseAddress, &id)
        }
        guard result == 0 else {
            throw VecDBError(code: result)
        }
        return id
    }
    
    // 搜索向量
    public func search(query: [Float], k: Int) throws -> [(id: UInt64, distance: Float)] {
        var results: UnsafeMutablePointer<jw_search_result_t>?
        var count: size_t = 0
        let result = query.withUnsafeBufferPointer { buffer in
            jw_collection_search(collectionPointer, buffer.baseAddress, size_t(k), &results, &count)
        }
        guard result == 0, let searchResults = results else {
            throw VecDBError(code: result)
        }
        
        defer {
            free(searchResults)
        }
        
        var resultArray: [(id: UInt64, distance: Float)] = []
        for i in 0..<count {
            let item = searchResults[Int(i)]
            resultArray.append((id: item.id, distance: item.distance))
        }
        return resultArray
    }
    
    // 删除向量
    public func delete(id: UInt64) throws {
        let result = jw_collection_delete(collectionPointer, id)
        guard result == 0 else {
            throw VecDBError(code: result)
        }
    }
    
    // 创建索引
    public func createIndex(type: String, params: String) throws {
        let result = jw_collection_create_index(collectionPointer, type, params)
        guard result == 0 else {
            throw VecDBError(code: result)
        }
    }
}

// 错误类
public struct VecDBError: Error {
    public let code: Int
    public var localizedDescription: String {
        switch code {
        case 0: return "成功"
        case -1: return "无效参数"
        case -2: return "内存不足"
        case -3: return "文件系统错误"
        case -4: return "集合已存在"
        case -5: return "集合不存在"
        case -6: return "向量不存在"
        case -7: return "索引创建失败"
        case -8: return "维度不匹配"
        case -9: return "内部错误"
        default: return "未知错误"
        }
    }
}

// C API 导入
private extension JinWoVecDB {
    @_silgen_name("jw_vecdb_open")
    static func jw_vecdb_open(_ db: inout UnsafeMutableRawPointer?, _ path: String, _ create: Bool) -> Int
    
    @_silgen_name("jw_vecdb_close")
    static func jw_vecdb_close(_ db: UnsafeMutableRawPointer) -> Int
    
    @_silgen_name("jw_collection_create")
    static func jw_collection_create(_ collection: inout UnsafeMutableRawPointer?, _ db: UnsafeMutableRawPointer, _ name: String, _ dimension: Int) -> Int
    
    @_silgen_name("jw_collection_open")
    static func jw_collection_open(_ collection: inout UnsafeMutableRawPointer?, _ db: UnsafeMutableRawPointer, _ name: String) -> Int
    
    @_silgen_name("jw_collection_close")
    static func jw_collection_close(_ collection: UnsafeMutableRawPointer) -> Int
    
    @_silgen_name("jw_collection_drop")
    static func jw_collection_drop(_ db: UnsafeMutableRawPointer, _ name: String) -> Int
    
    @_silgen_name("jw_collection_list")
    static func jw_collection_list(_ db: UnsafeMutableRawPointer, _ names: inout UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>, _ count: inout size_t) -> Int
    
    @_silgen_name("jw_collection_insert")
    static func jw_collection_insert(_ collection: UnsafeMutableRawPointer, _ vector: UnsafePointer<Float>, _ id: inout UInt64) -> Int
    
    @_silgen_name("jw_collection_search")
    static func jw_collection_search(_ collection: UnsafeMutableRawPointer, _ query: UnsafePointer<Float>, _ k: size_t, _ results: inout UnsafeMutablePointer<jw_search_result_t>?, _ count: inout size_t) -> Int
    
    @_silgen_name("jw_collection_delete")
    static func jw_collection_delete(_ collection: UnsafeMutableRawPointer, _ id: UInt64) -> Int
    
    @_silgen_name("jw_collection_create_index")
    static func jw_collection_create_index(_ collection: UnsafeMutableRawPointer, _ type: String, _ params: String) -> Int
}

// 搜索结果结构
private struct jw_search_result_t {
    var id: UInt64
    var distance: Float
}
```

### 3.2 Objective-C 封装

```objective-c
// JinWoVecDB.h
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VecDBError : NSError
+ (instancetype)errorWithCode:(int)code;
@end

@interface Collection : NSObject
- (instancetype)initWithPointer:(void *)pointer;
- (UInt64)insertVector:(NSArray<NSNumber *> *)vector error:(NSError **)error;
- (NSArray<NSDictionary<NSString *, id> *> *)searchWithQuery:(NSArray<NSNumber *> *)query k:(NSInteger)k error:(NSError **)error;
- (BOOL)deleteVectorWithId:(UInt64)id error:(NSError **)error;
- (BOOL)createIndexWithType:(NSString *)type params:(NSString *)params error:(NSError **)error;
@end

@interface JinWoVecDB : NSObject
- (instancetype)initWithPath:(NSString *)path create:(BOOL)create error:(NSError **)error;
- (Collection *)createCollectionWithName:(NSString *)name dimension:(NSInteger)dimension error:(NSError **)error;
- (Collection *)openCollectionWithName:(NSString *)name error:(NSError **)error;
- (BOOL)dropCollectionWithName:(NSString *)name error:(NSError **)error;
- (NSArray<NSString *> *)listCollectionsWithError:(NSError **)error;
@end

NS_ASSUME_NONNULL_END

// JinWoVecDB.m
#import "JinWoVecDB.h"
#include "jw_vecdb.h"

@implementation VecDBError
+ (instancetype)errorWithCode:(int)code {
    NSString *description;
    switch (code) {
        case 0: description = @"成功";
            break;
        case -1: description = @"无效参数";
            break;
        case -2: description = @"内存不足";
            break;
        case -3: description = @"文件系统错误";
            break;
        case -4: description = @"集合已存在";
            break;
        case -5: description = @"集合不存在";
            break;
        case -6: description = @"向量不存在";
            break;
        case -7: description = @"索引创建失败";
            break;
        case -8: description = @"维度不匹配";
            break;
        case -9: description = @"内部错误";
            break;
        default: description = @"未知错误";
    }
    return [NSError errorWithDomain:@"VecDBErrorDomain" code:code userInfo:@{NSLocalizedDescriptionKey: description}];
}
@end

@implementation Collection {
    void *_collectionPointer;
}
- (instancetype)initWithPointer:(void *)pointer {
    if (self = [super init]) {
        _collectionPointer = pointer;
    }
    return self;
}
- (void)dealloc {
    jw_collection_close(_collectionPointer);
}
- (UInt64)insertVector:(NSArray<NSNumber *> *)vector error:(NSError **)error {
    NSMutableData *data = [NSMutableData dataWithCapacity:vector.count * sizeof(float)];
    float *floatPtr = (float *)data.mutableBytes;
    for (NSInteger i = 0; i < vector.count; i++) {
        floatPtr[i] = [vector[i] floatValue];
    }
    uint64_t id;
    int result = jw_collection_insert(_collectionPointer, floatPtr, &id);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return 0;
    }
    return id;
}
- (NSArray<NSDictionary<NSString *, id> *> *)searchWithQuery:(NSArray<NSNumber *> *)query k:(NSInteger)k error:(NSError **)error {
    NSMutableData *data = [NSMutableData dataWithCapacity:query.count * sizeof(float)];
    float *floatPtr = (float *)data.mutableBytes;
    for (NSInteger i = 0; i < query.count; i++) {
        floatPtr[i] = [query[i] floatValue];
    }
    jw_search_result_t *results;
    size_t count;
    int result = jw_collection_search(_collectionPointer, floatPtr, (size_t)k, &results, &count);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return nil;
    }
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:count];
    for (size_t i = 0; i < count; i++) {
        jw_search_result_t item = results[i];
        [resultArray addObject:@{@"id": @(item.id), @"distance": @(item.distance)}];
    }
    free(results);
    return resultArray;
}
- (BOOL)deleteVectorWithId:(UInt64)id error:(NSError **)error {
    int result = jw_collection_delete(_collectionPointer, id);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return NO;
    }
    return YES;
}
- (BOOL)createIndexWithType:(NSString *)type params:(NSString *)params error:(NSError **)error {
    int result = jw_collection_create_index(_collectionPointer, [type UTF8String], [params UTF8String]);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return NO;
    }
    return YES;
}
@end

@implementation JinWoVecDB {
    void *_dbPointer;
}
- (instancetype)initWithPath:(NSString *)path create:(BOOL)create error:(NSError **)error {
    if (self = [super init]) {
        void *ptr;
        int result = jw_vecdb_open(&ptr, [path UTF8String], create);
        if (result != 0) {
            if (error) {
                *error = [VecDBError errorWithCode:result];
            }
            return nil;
        }
        _dbPointer = ptr;
    }
    return self;
}
- (void)dealloc {
    jw_vecdb_close(_dbPointer);
}
- (Collection *)createCollectionWithName:(NSString *)name dimension:(NSInteger)dimension error:(NSError **)error {
    void *ptr;
    int result = jw_collection_create(&ptr, _dbPointer, [name UTF8String], (int)dimension);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return nil;
    }
    return [[Collection alloc] initWithPointer:ptr];
}
- (Collection *)openCollectionWithName:(NSString *)name error:(NSError **)error {
    void *ptr;
    int result = jw_collection_open(&ptr, _dbPointer, [name UTF8String]);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return nil;
    }
    return [[Collection alloc] initWithPointer:ptr];
}
- (BOOL)dropCollectionWithName:(NSString *)name error:(NSError **)error {
    int result = jw_collection_drop(_dbPointer, [name UTF8String]);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return NO;
    }
    return YES;
}
- (NSArray<NSString *> *)listCollectionsWithError:(NSError **)error {
    char **names;
    size_t count;
    int result = jw_collection_list(_dbPointer, &names, &count);
    if (result != 0) {
        if (error) {
            *error = [VecDBError errorWithCode:result];
        }
        return nil;
    }
    NSMutableArray *resultArray = [NSMutableArray arrayWithCapacity:count];
    for (size_t i = 0; i < count; i++) {
        [resultArray addObject:[NSString stringWithUTF8String:names[i]]];
        free(names[i]);
    }
    free(names);
    return resultArray;
}
@end
```

---

## 4. 示例代码

### 4.1 Swift 示例

```swift
// ViewController.swift
import UIKit

class ViewController: UIViewController {
    private var vecdb: JinWoVecDB?
    private var collection: Collection?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupVectorDB()
    }
    
    private func setupVectorDB() {
        do {
            // 获取文档目录路径
            let documentsDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
            let dbPath = documentsDirectory + "/vecdb"
            
            // 打开数据库
            vecdb = try JinWoVecDB(path: dbPath, create: true)
            
            // 创建集合
            collection = try vecdb?.createCollection(name: "test", dimension: 128)
            
            // 插入向量
            let vector = (0..<128).map { Float($0) / 128.0 }
            let id = try collection?.insert(vector: vector)
            print("Inserted vector with id: \(id!)")
            
            // 搜索向量
            let results = try collection?.search(query: vector, k: 10)
            print("Found \(results!.count) results:")
            for (index, result) in results!.enumerated() {
                print("  \(index): id=\(result.id), distance=\(result.distance)")
            }
            
        } catch let error as VecDBError {
            print("VecDB error: \(error.localizedDescription)")
        } catch {
            print("Unexpected error: \(error)")
        }
    }
}
```

### 4.2 Objective-C 示例

```objective-c
// ViewController.m
#import "ViewController.h"
#import "JinWoVecDB.h"

@interface ViewController ()
@property (nonatomic, strong) JinWoVecDB *vecdb;
@property (nonatomic, strong) Collection *collection;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setupVectorDB];
}

- (void)setupVectorDB {
    // 获取文档目录路径
    NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString *dbPath = [documentsDirectory stringByAppendingPathComponent:@"vecdb"];
    
    // 打开数据库
    NSError *error;
    self.vecdb = [[JinWoVecDB alloc] initWithPath:dbPath create:YES error:&error];
    if (error) {
        NSLog(@"Error opening database: %@", error.localizedDescription);
        return;
    }
    
    // 创建集合
    self.collection = [self.vecdb createCollectionWithName:@"test" dimension:128 error:&error];
    if (error) {
        NSLog(@"Error creating collection: %@", error.localizedDescription);
        return;
    }
    
    // 插入向量
    NSMutableArray *vector = [NSMutableArray arrayWithCapacity:128];
    for (NSInteger i = 0; i < 128; i++) {
        [vector addObject:@( (float)i / 128.0 )];
    }
    UInt64 id = [self.collection insertVector:vector error:&error];
    if (error) {
        NSLog(@"Error inserting vector: %@", error.localizedDescription);
        return;
    }
    NSLog(@"Inserted vector with id: %llu", id);
    
    // 搜索向量
    NSArray *results = [self.collection searchWithQuery:vector k:10 error:&error];
    if (error) {
        NSLog(@"Error searching: %@", error.localizedDescription);
        return;
    }
    NSLog(@"Found %lu results:", (unsigned long)results.count);
    for (NSInteger i = 0; i < results.count; i++) {
        NSDictionary *result = results[i];
        NSLog(@"  %ld: id=%llu, distance=%f", (long)i, [result[@"id"] unsignedLongLongValue], [result[@"distance"] floatValue]);
    }
}

@end
```

---

## 5. 性能优化

### 5.1 内存管理

| 优化项 | 建议 |
|-------|------|
| 向量批处理 | 使用批处理插入，减少API调用次数 |
| 内存分配 | 重用向量数组，避免频繁创建 |
| 缓存策略 | 缓存常用查询结果 |
| 索引优化 | 根据数据量选择合适的索引类型 |

### 5.2 并发处理

| 优化项 | 建议 |
|-------|------|
| 线程管理 | 使用 GCD 处理数据库操作 |
| 异步操作 | 使用 completion handler 处理长时间操作 |
| 批量操作 | 合并多个操作减少线程切换 |
| 超时处理 | 设置合理的操作超时时间 |

### 5.3 存储优化

| 优化项 | 建议 |
|-------|------|
| 存储路径 | 使用应用沙盒目录 |
| 数据压缩 | 启用向量压缩减少存储 |
| 定期清理 | 定期清理不需要的数据 |
| 备份策略 | 实现数据备份机制 |

---

## 6. 权限配置

### 6.1 iOS 权限

| 权限 | 用途 | 是否必需 |
|------|------|----------|
| 无 | 存储使用应用沙盒 | 否 |

### 6.2 沙盒路径

| 路径类型 | 用途 | 示例路径 |
|---------|------|----------|
| Document Directory | 持久化数据 | `~/Documents/vecdb` |
| Cache Directory | 缓存数据 | `~/Library/Caches/vecdb` |
| Temporary Directory | 临时数据 | `~/tmp/vecdb` |

---

## 7. 常见问题

### 7.1 编译问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 框架未签名 | 框架签名问题 | 在 "Signing & Capabilities" 中设置签名 |
| 编译错误 | 缺少依赖 | 检查 `libc++.tbd` 是否添加 |
| 链接错误 | 架构不匹配 | 确保使用正确的架构 (arm64) |

### 7.2 运行时问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 找不到框架 | 框架未正确嵌入 | 设置 "Embed" 为 "Embed & Sign" |
| 内存不足 | 向量数据过大 | 减少批处理大小 |
| 权限错误 | 沙盒权限问题 | 使用正确的沙盒路径 |
| 崩溃 | 空指针或内存错误 | 检查参数和错误处理 |

### 7.3 性能问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 插入慢 | 单次插入次数过多 | 使用批处理 |
| 搜索慢 | 索引未优化 | 调整索引参数 |
| 内存占用高 | 向量未压缩 | 启用 PQ 压缩 |
| 响应慢 | 主线程操作 | 移至后台线程 |

---

## 8. 调试技巧

### 8.1 日志调试

```swift
// 启用详细日志
UserDefaults.standard.set("debug", forKey: "jinwo.vecdb.log.level")

// 自定义日志处理
class VecDBLogger {
    static func log(_ message: String) {
        print("[VecDB] \(message)")
    }
    
    static func logError(_ message: String, error: Error? = nil) {
        if let error = error {
            print("[VecDB ERROR] \(message): \(error)")
        } else {
            print("[VecDB ERROR] \(message)")
        }
    }
}
```

### 8.2 性能分析

| 工具 | 用途 | 使用方法 |
|------|------|----------|
| Instruments | 内存和CPU分析 | Xcode → Product → Profile |
| Time Profiler | 方法执行时间 | Instruments → Time Profiler |
| Allocations | 内存分配 | Instruments → Allocations |

### 8.3 错误处理

```swift
do {
    let id = try collection.insert(vector: vector)
    // 处理成功
} catch let error as VecDBError {
    VecDBLogger.logError("Insert failed", error: error)
    // 处理错误
}
```

---

## 9. 发布注意事项

### 9.1 代码签名

| 配置项 | 要求 |
|--------|------|
| 框架签名 | 必须签名 |
| 应用签名 | 必须签名 |
| 证书 | 有效的开发者证书 |

### 9.2 架构支持

| 架构 | 支持状态 |
|------|----------|
| arm64 (iPhone) | ✅ 支持 |
| arm64 (iPad) | ✅ 支持 |
| x86_64 (模拟器) | ✅ 支持 |

### 9.3 App Store 合规

| 合规项 | 要求 |
|--------|------|
| 隐私政策 | 符合 GDPR/CCPA 要求 |
| 性能要求 | 启动时间 < 5秒 |
| 内存使用 | 合理使用内存 |
| 电池使用 | 高效使用电池 |

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
