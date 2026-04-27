# JinWo VecDB iOS Platform Integration Guide

**Version**: 1.0.0
**Last Updated**: 2026-04-26
**Platform**: iOS

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Integration Methods](#integration-methods)
4. [CMake Build Configuration](#cmake-build-configuration)
5. [Xcode Project Setup](#xcode-project-setup)
6. [CocoaPods Integration](#cocoapods-integration)
7. [Swift Usage](#swift-usage)
8. [Objective-C Usage](#objective-c-usage)
9. [Memory Management](#memory-management)
10. [Performance Optimization](#performance-optimization)
11. [Troubleshooting](#troubleshooting)

---

## Overview

JinWo VecDB provides native iOS support through a C library that can be integrated via:
- CMake with Xcode generator
- CocoaPods (recommended for Swift projects)
- Manual integration

### Supported iOS Versions

- **Minimum**: iOS 12.0
- **Recommended**: iOS 14.0+
- **Architectures**: arm64, arm64e, x86_64 (Simulator)

---

## Prerequisites

### Required Tools

1. **Xcode** 12.0 or later
2. **CMake** 3.18 or later
3. **Python** 3.6+ (for build scripts)
4. **Apple Developer Account** (for device deployment)

### Optional Tools

1. **CocoaPods** 1.10+ (for Swift projects)
2. **Swift Package Manager** (future support)

---

## Integration Methods

### Method 1: CMake + Xcode (Recommended for Objective-C/C++)

This method provides the most control and is recommended for projects that already use CMake.

#### Step 1: Create CMakeLists.txt

Create or modify your project's CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.18)
project(MyVecDBApp)

# Set iOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")

# Specify the toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ios.toolchain.cmake")

# JinWo VecDB source directory
set(JW_VECDB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jinwo_vector_db")

# Create executable
add_executable(my_vecdb_app
    main.m
    AppDelegate.m
    ViewController.m
)

# Include JinWo VecDB headers
target_include_directories(my_vecdb_app PRIVATE
    "${JW_VECDB_SOURCE_DIR}/include"
)

# Link JinWo VecDB (after building)
target_link_libraries(my_vecdb_app
    jw_vecdb
    "-framework Foundation"
    "-framework UIKit"
)
```

#### Step 2: iOS Toolchain File

Create `cmake/ios.toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 12.0)
set(CMAKE_XCODE_GENERATOR Xcode)

# Specify architectures
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# Enable death code stripping
set(CMAKE_XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING YES)

# Code signing
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic)

# Bitcode (deprecated, but kept for compatibility)
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE NO)
```

#### Step 3: Build

```bash
# Create build directory
mkdir build && cd build

# Configure for iOS Simulator
cmake .. -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATOR64

# Build
xcodebuild -scheme my_vecdb_app -configuration Release
```

---

## CMake Build Configuration

### Build JinWo VecDB as iOS Library

Create a separate build script for JinWo VecDB:

```bash
#!/bin/bash
# build_ios.sh

set -e

JW_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${JW_DIR}/build_ios"

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure for iOS Simulator
cmake "${JW_DIR}" \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="${JW_DIR}/cmake/ios.toolchain.cmake" \
    -DPLATFORM=SIMULATOR64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_SHARED=OFF \
    -DJW_BUILD_TESTS=OFF \
    -DJW_BUILD_EXAMPLES=OFF

# Build library
xcodebuild -scheme jw_vecdb -configuration Release -parallelizeTarget

# Copy library to lib directory
mkdir -p "${JW_DIR}/lib/ios"
cp -R Build/Products/Release-*.sdk/*.a "${JW_DIR}/lib/ios/"
```

---

## Xcode Project Setup

### Manual Setup Steps

1. **Open Xcode** and create a new project (or open existing)

2. **Add JinWo VecDB sources**:
   - Drag `jinwo_vector_db/` folder into your Xcode project
   - Select "Create groups" (not "Create folder references")
   - Check "Copy items if needed"

3. **Configure Build Settings**:
   - Set **Header Search Paths**: `$(SRCROOT)/jinwo_vector_db/include`
   - Set **Library Search Paths**: `$(SRCROOT)/lib/ios`
   - Set **Other Linker Flags**: `-ljw_vecdb`

4. **Configure General Settings**:
   - Set **iOS Deployment Target**: 12.0
   - Set **Enable Bitcode**: NO

---

## CocoaPods Integration

### Podfile Configuration

Create a `Podfile` in your project root:

```ruby
platform :ios, '12.0'
use_frameworks!

target 'MyVecDBApp' do
  # JinWo VecDB - build from local source
  pod 'JinWoVecDB', :path => './vendor/jinwo_vector_db'
end

post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '12.0'
      config.build_settings['ENABLE_BITCODE'] = 'NO'
    end
  end
end
```

### Podspec Configuration

Create `JinWoVecDB.podspec`:

```ruby
Pod::Spec.new do |s|
  s.name             = 'JinWoVecDB'
  s.version          = '1.0.0'
  s.summary          = 'JinWo VecDB - Embedded Vector Database for iOS'
  s.description      = <<-DESC
    JinWo VecDB is an embedded vector database library for iOS,
    optimized for mobile and embedded devices.
  DESC
  s.homepage         = 'https://jinwo.site'
  s.license          = { :type => 'Apache 2.0', :file => 'LICENSE' }
  s.author           = { 'Jinwo' => 'contact@jinwo.site' }
  s.source           = { :git => 'https://gitee.com/pkjinwo/jinwo_vector_db.git', :tag => s.version.to_s }

  s.ios.deployment_target = '12.0'
  s.source_files = ['include/*.h', 'src/*.c']
  s.public_header_files = ['include/*.h']
  s.library = 'jw_vecdb'
  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => '"$(PODS_ROOT)/JinWoVecDB/include"',
    'OTHER_CFLAGS' => '-DJW_IOS -DWITH_SIMD'
  }
end
```

---

## Swift Usage

### Bridging Header

Create a bridging header `JinWoVecDB-Bridging-Header.h`:

```objc
#ifndef JinWoVecDB_Bridging_Header_h
#define JinWoVecDB_Bridging_Header_h

#import "jw_vecdb.h"
#import "jw_collection.h"
#import "jw_vector.h"
#import "jw_types.h"

#endif /* JinWoVecDB_Bridging_Header_h */
```

### Swift Wrapper Class

```swift
import Foundation

/// JinWo VecDB Swift Wrapper
public class JinWoVecDB {

    public typealias Vector = [Float]
    public typealias SearchResult = (id: UInt64, score: Float)

    private var db: OpaquePointer?

    /// Initialize JinWo VecDB
    public init() {
        jw_init()
    }

    deinit {
        if let db = db {
            jw_vecdb_close(db)
        }
        jw_cleanup()
    }

    /// Open database
    public func open(path: String?, create: Bool = true) throws {
        var flags: UInt32 = 0
        if create {
            flags |= UInt32(JW_VECDB_CREATE)
        }
        flags |= UInt32(JW_VECDB_READWRITE)

        var dbPtr: OpaquePointer?
        let pathStr = path ?? ""
        let status = path?.withCString { cPath in
            jw_vecdb_open(jw_str_new(cPath), flags, &dbPtr)
        } ?? jw_vecdb_open(nil, flags | UInt32(JW_VECDB_MEMORY), &dbPtr)

        guard status == JW_SUCCESS else {
            throw VecDBError(code: status, message: "Failed to open database")
        }
        self.db = dbPtr
    }

    /// Create collection
    public func createCollection(name: String, dimension: Int) throws -> Collection {
        guard let db = db else {
            throw VecDBError.notOpen
        }

        var collPtr: OpaquePointer?
        let status = name.withCString { cName in
            jw_vecdb_create_collection(db, jw_str_new(cName), UInt32(dimension), &collPtr)
        }

        guard status == JW_SUCCESS, let coll = collPtr else {
            throw VecDBError(code: status, message: "Failed to create collection")
        }

        return Collection(coll)
    }

    /// Get collection
    public func getCollection(name: String) -> Collection? {
        guard let db = db else { return nil }
        return name.withCString { cName in
            guard let coll = jw_vecdb_get_collection(db, jw_str_new(cName)) else {
                return nil
            }
            return Collection(coll)
        }
    }

    /// Close database
    public func close() throws {
        guard let db = db else { return }
        let status = jw_vecdb_close(db)
        self.db = nil
        if status != JW_SUCCESS {
            throw VecDBError(code: status, message: "Failed to close database")
        }
    }
}

/// Collection wrapper
public class Collection {
    let coll: OpaquePointer

    init(_ coll: OpaquePointer) {
        self.coll = coll
    }

    /// Insert vector
    public func insert(_ vector: Vector) throws -> UInt64 {
        var vid: UInt64 = 0
        let status = vector.withUnsafeBufferPointer { buffer in
            jw_collection_insert(coll, buffer.baseAddress, &vid)
        }
        guard status == JW_SUCCESS else {
            throw VecDBError(code: status, message: "Failed to insert vector")
        }
        return vid
    }

    /// Search similar vectors
    public func search(query: Vector, k: Int) throws -> [SearchResult] {
        var results = [jw_search_result_t](repeating: jw_search_result_t(), count: k)
        let count = query.withUnsafeBufferPointer { buffer in
            jw_collection_search(coll, buffer.baseAddress, k, &results)
        }

        guard count > 0 else { return [] }

        return results.prefix(Int(count)).map { result in
            SearchResult(id: result.vid, score: result.score)
        }
    }
}

/// Error type
public struct VecDBError: Error {
    public let code: Int32
    public let message: String

    public static let notOpen = VecDBError(code: -1, message: "Database not open")

    public var description: String {
        return "VecDBError(\(code)): \(message)"
    }
}
```

### Usage Example

```swift
import Foundation

// Initialize
let db = JinWoVecDB()

// Open in-memory database
try db.open(path: nil, create: true)

// Create collection
let coll = try db.createCollection(name: "documents", dimension: 128)

// Insert vectors
let embedding = (0..<128).map { _ in Float.random(in: 0...1) }
let vid = try coll.insert(embedding)

// Search
let results = try coll.search(query: embedding, k: 5)
for result in results {
    print("ID: \(result.id), Score: \(result.score)")
}

// Close
try db.close()
```

---

## Objective-C Usage

### Header File

```objc
// VecDBManager.h
#import <Foundation/Foundation.h>
#import "jw_vecdb.h"
#import "jw_collection.h"

NS_ASSUME_NONNULL_BEGIN

@interface VecDBManager : NSObject

@property (nonatomic, readonly) BOOL isOpen;

- (instancetype)init;
- (BOOL)openWithPath:(nullable NSString *)path
              create:(BOOL)create
               error:(NSError **)error;
- (void)close;
- (nullable id)createCollectionWithName:(NSString *)name
                            dimension:(NSUInteger)dimension
                                error:(NSError **)error;
- (BOOL)insertVector:(float *)vector
        intoCollection:(NSString *)collName
                outVid:(uint64_t *)outVid
                  error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
```

### Implementation File

```objc
// VecDBManager.m
#import "VecDBManager.h"

@interface VecDBManager ()
@property (nonatomic, assign) jw_vecdb_t *db;
@end

@implementation VecDBManager

- (instancetype)init {
    self = [super init];
    if (self) {
        _db = NULL;
        jw_init();
    }
    return self;
}

- (void)dealloc {
    [self close];
    jw_cleanup();
}

- (BOOL)isOpen {
    return _db != NULL;
}

- (BOOL)openWithPath:(NSString *)path
              create:(BOOL)create
               error:(NSError **)error {
    uint32_t flags = JW_VECDB_READWRITE;
    if (create) {
        flags |= JW_VECDB_CREATE;
    }

    jw_vecdb_t *db = NULL;
    jw_status_t status;

    if (path) {
        status = jw_vecdb_open(jw_str_new([path UTF8String]), flags, &db);
    } else {
        status = jw_vecdb_open(NULL, flags | JW_VECDB_MEMORY, &db);
    }

    if (status != JW_SUCCESS) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:status
                                    userInfo:@{NSLocalizedDescriptionKey: @"Failed to open database"}];
        }
        return NO;
    }

    _db = db;
    return YES;
}

- (void)close {
    if (_db) {
        jw_vecdb_close(_db);
        _db = NULL;
    }
}

- (nullable id)createCollectionWithName:(NSString *)name
                            dimension:(NSUInteger)dimension
                                error:(NSError **)error {
    if (!_db) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:-1
                                    userInfo:@{NSLocalizedDescriptionKey: @"Database not open"}];
        }
        return nil;
    }

    jw_collection_t *coll = NULL;
    jw_status_t status = jw_vecdb_create_collection(
        _db,
        jw_str_new([name UTF8String]),
        (jw_dim_t)dimension,
        &coll
    );

    if (status != JW_SUCCESS) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:status
                                    userInfo:@{NSLocalizedDescriptionKey: @"Failed to create collection"}];
        }
        return nil;
    }

    return (__bridge id)coll;
}

- (BOOL)insertVector:(float *)vector
        intoCollection:(NSString *)collName
                outVid:(uint64_t *)outVid
                  error:(NSError **)error {
    if (!_db) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:-1
                                    userInfo:@{NSLocalizedDescriptionKey: @"Database not open"}];
        }
        return NO;
    }

    jw_collection_t *coll = jw_vecdb_get_collection(
        _db,
        jw_str_new([collName UTF8String])
    );

    if (!coll) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:-1
                                    userInfo:@{NSLocalizedDescriptionKey: @"Collection not found"}];
        }
        return NO;
    }

    jw_vid_t vid = 0;
    jw_status_t status = jw_collection_insert(coll, vector, &vid);

    if (status != JW_SUCCESS) {
        if (error) {
            *error = [NSError errorWithDomain:@"JinWoVecDB"
                                        code:status
                                    userInfo:@{NSLocalizedDescriptionKey: @"Failed to insert vector"}];
        }
        return NO;
    }

    if (outVid) {
        *outVid = vid;
    }

    return YES;
}

@end
```

### Usage Example

```objc
// AppDelegate.m
#import "VecDBManager.h"

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

    VecDBManager *manager = [[VecDBManager alloc] init];

    NSError *error = nil;
    if (![manager openWithPath:nil create:YES error:&error]) {
        NSLog(@"Failed to open database: %@", error);
        return NO;
    }

    id collection = [manager createCollectionWithName:@"documents"
                                           dimension:128
                                               error:&error];
    if (!collection) {
        NSLog(@"Failed to create collection: %@", error);
        return NO;
    }

    // Insert vector
    float vector[128];
    for (int i = 0; i < 128; i++) {
        vector[i] = arc4random_uniform(1000) / 1000.0f;
    }

    uint64_t vid = 0;
    if ([manager insertVector:vector intoCollection:@"documents" outVid:&vid error:&error]) {
        NSLog(@"Inserted vector with ID: %llu", vid);
    }

    [manager close];
    return YES;
}
```

---

## Memory Management

### iOS-Specific Considerations

1. **Memory Limits**: iOS enforces strict memory limits on apps. Monitor memory usage with Instruments.

2. **Memory Warnings**: Handle `UIApplicationDidReceiveMemoryWarningNotification`.

3. **Automatic Reference Counting (ARC)**: JinWo VecDB is C-based, so manual memory management is required.

### Best Practices

```objc
// Use @autoreleasepool for batch operations
@autoreleasepool {
    for (int i = 0; i < 10000; i++) {
        // Insert vectors
    }
}

// Monitor memory
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Clear caches, close unused collections
}
```

---

## Performance Optimization

### SIMD on iOS

JinWo VecDB automatically detects and uses **NEON** SIMD instructions on ARM64 devices.

### Recommended Settings

```objc
// Enable SIMD (enabled by default)
jw_vecdb_set_simd_enabled(JW_TRUE);

// Check availability
if (jw_vecdb_is_simd_available()) {
    NSLog(@"SIMD (NEON) is available");
}
```

### Optimization Tips

1. **Batch Operations**: Use `jw_collection_insert_batch` for bulk inserts
2. **Memory Pool**: Configure arena size based on available memory
3. **Index Building**: Build index after bulk insert, not during
4. **Background Processing**: Perform heavy operations on background threads

```objc
// Background vector insertion
dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    @autoreleasepool {
        for (int i = 0; i < 10000; i++) {
            [manager insertVector:vector intoCollection:@"documents" outVid:NULL error:nil];
        }
    }
});
```

---

## Troubleshooting

### Common Issues

#### 1. "Undefined symbols" errors

**Solution**: Ensure you've added `libjw_vecdb.a` to your project's linked libraries.

#### 2. "Header file not found"

**Solution**: Add JinWo VecDB include path to **Header Search Paths** in build settings.

#### 3. "Bitcode mismatch" warning

**Solution**: Set **Enable Bitcode** to **NO** in build settings.

#### 4. Memory warnings on device

**Solution**: Reduce arena size, use quantized vectors, or split database.

### Debugging

Enable logging:

```c
jw_vecdb_set_log_callback(^(int level, const char *msg, void *user_data) {
    NSLog(@"JW[%d]: %s", level, msg);
}, NULL);
```

### Performance Profiling

Use **Instruments** (Allocations, Time Profiler) to monitor:
- Memory usage
- CPU usage
- SIMD acceleration effectiveness

---

## Next Steps

1. [API Reference Documentation](../docs/api_reference.md)
2. [Android Integration Guide](./android_integration.md)
3. [Windows Integration Guide](./windows_integration.md)

---

**Document Version**: 1.0.0
**Last Updated**: 2026-04-26
