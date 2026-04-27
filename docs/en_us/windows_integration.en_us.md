# JinWo VecDB Windows Platform Integration Guide

**Version**: 1.0.0
**Last Updated**: 2026-04-26
**Platform**: Windows

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Build Methods](#build-methods)
4. [Visual Studio Integration](#visual-studio-integration)
5. [CMake with Visual Studio](#cmake-with-visual-studio)
6. [C++/C# Usage](#cc-usage)
7. [.NET Integration](#net-integration)
8. [Memory Management](#memory-management)
9. [Performance Optimization](#performance-optimization)
10. [Unicode and Encoding](#unicode-and-encoding)
11. [Troubleshooting](#troubleshooting)

---

## Overview

JinWo VecDB provides native Windows support through a C library that can be integrated with:
- Visual Studio (MSVC)
- MinGW-w64
- CMake with Visual Studio generator
- .NET via P/Invoke

### Supported Windows Versions

- **Minimum**: Windows 7 (SP1)
- **Recommended**: Windows 10/11
- **Architectures**: x64, x86

---

## Prerequisites

### Required Tools

1. **Visual Studio** 2019 or later (with C++ workload)
2. **CMake** 3.18 or later
3. **Windows SDK** 10.0 or later

### Optional Tools

1. **MinGW-w64** (for GCC-based builds)
2. **vcpkg** (for dependency management)
3. **NuGet** (for .NET integration)

---

## Build Methods

### Method 1: CMake + Visual Studio (Recommended)

#### Build Script

Create `build_windows.bat`:

```batch
@echo off
setlocal

set JW_DIR=%~dp0
set BUILD_DIR=%JW_DIR%build
set INSTALL_DIR=%JW_DIR%install

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure with CMake
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DJW_BUILD_SHARED=OFF ^
    -DJW_BUILD_TESTS=OFF ^
    -DJW_BUILD_EXAMPLES=ON ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%"

REM Build
cmake --build . --config Release --parallel

REM Install
cmake --install . --config Release

echo.
echo Build complete!
echo Library installed to: %INSTALL_DIR%
echo.

endlocal
```

### Method 2: MSVC Command Line

```batch
@echo off
setlocal

set JW_DIR=%~dp0
set BUILD_DIR=%JW_DIR%build_msvc

mkdir "%BUILD_DIR%" 2>nul
cd /d "%BUILD_DIR%"

REM Run CMake
cmake "%JW_DIR%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DJW_BUILD_SHARED=OFF

REM Build
cmake --build . --config Release --parallel

endlocal
```

### Method 3: MinGW-w64

```bash
#!/bin/bash
# build_mingw.sh

set -e

JW_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${JW_DIR}/build_mingw"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
cmake "${JW_DIR}" \
    -G "MinGW Makefiles" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_BUILD_TYPE=Release \
    -DJW_BUILD_SHARED=OFF

# Build
mingw32-make -j$(nproc)

# Install
mingw32-make install
```

---

## Visual Studio Integration

### Step-by-Step Instructions

#### Step 1: Create New Project

1. Open Visual Studio
2. Create a new **Console App (C++)** or **Empty Project**
3. Set **Platform** to x64

#### Step 2: Configure Include and Library Paths

1. Right-click project → **Properties**
2. **C/C++** → **General** → **Additional Include Directories**:
   ```
   $(SolutionDir)..\jinwo_vector_db\include
   ```

3. **Linker** → **General** → **Additional Library Directories**:
   ```
   $(SolutionDir)..\lib\windows
   ```

4. **Linker** → **Input** → **Additional Dependencies**:
   ```
   jw_vecdb.lib
   ```

#### Step 3: Copy Library Files

1. Build JinWo VecDB library (see Build Methods)
2. Copy output files:
   ```
   build/
   ├── jw_vecdb.lib      (static library)
   └── jw_vecdb.dll      (DLL, if shared)
   ```
   to your project's `lib/` directory

#### Step 4: Configure Preprocessor

In **C/C++** → **Preprocessor** → **Preprocessor Definitions**, add:
```
_JW_WINDOWS=1
_CRT_SECURE_NO_WARNINGS
```

---

## CMake with Visual Studio

### Project CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)
project(MyVecDBApp)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_JW_WINDOWS=1)
endif()

# JinWo VecDB directory
set(JW_VECDB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jinwo_vector_db")

# Include directories
include_directories(
    "${JW_VECDB_DIR}/include"
)

# Create executable
add_executable(my_vecdb_app
    main.cpp
)

# Link JinWo VecDB
add_subdirectory("${JW_VECDB_DIR}" jw_vecdb_build)
target_link_libraries(my_vecdb_app jw_vecdb)
```

### Building

```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## C++/C# Usage

### C++ Usage

#### Header-Only Wrapper

Create `vecdb_wrapper.hpp`:

```cpp
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include "jw_vecdb.h"

class VecDB {
public:
    VecDB() : db_(nullptr), is_open_(false) {
        jw_init();
    }

    ~VecDB() {
        Close();
        jw_cleanup();
    }

    bool Open(const std::wstring& path, bool create = true) {
        uint32_t flags = JW_VECDB_READWRITE;
        if (create) flags |= JW_VECDB_CREATE;

        jw_vecdb_t* db = nullptr;
        jw_status_t status;

        if (path.empty()) {
            status = jw_vecdb_open(nullptr, flags | JW_VECDB_MEMORY, &db);
        } else {
            status = jw_vecdb_open(jw_str_new(path.c_str()), flags, &db);
        }

        if (status != JW_SUCCESS) {
            return false;
        }

        db_ = db;
        is_open_ = true;
        return true;
    }

    void Close() {
        if (db_) {
            jw_vecdb_close(db_);
            db_ = nullptr;
            is_open_ = false;
        }
    }

    bool CreateCollection(const std::wstring& name, int dim) {
        if (!db_) return false;

        jw_collection_t* coll = nullptr;
        jw_status_t status = jw_vecdb_create_collection(
            db_,
            jw_str_new(name.c_str()),
            dim,
            &coll
        );

        return status == JW_SUCCESS;
    }

    uint64_t Insert(const std::wstring& coll_name, const float* vector, int dim) {
        if (!db_) return 0;

        jw_collection_t* coll = jw_vecdb_get_collection(
            db_,
            jw_str_new(coll_name.c_str())
        );

        if (!coll) return 0;

        jw_vid_t vid = 0;
        jw_status_t status = jw_collection_insert(coll, vector, &vid);

        return (status == JW_SUCCESS) ? vid : 0;
    }

    std::vector<std::pair<uint64_t, float>> Search(
        const std::wstring& coll_name,
        const float* query,
        int dim,
        int k
    ) {
        std::vector<std::pair<uint64_t, float>> results;

        if (!db_) return results;

        jw_collection_t* coll = jw_vecdb_get_collection(
            db_,
            jw_str_new(coll_name.c_str())
        );

        if (!coll) return results;

        std::vector<jw_search_result_t> search_results(k);
        size_t count = jw_collection_search(coll, query, k, search_results.data());

        for (size_t i = 0; i < count; i++) {
            results.emplace_back(search_results[i].vid, search_results[i].score);
        }

        return results;
    }

    bool IsOpen() const { return is_open_; }

private:
    jw_vecdb_t* db_;
    bool is_open_;
};
```

#### Usage Example

```cpp
// main.cpp
#include <iostream>
#include <vector>
#include "vecdb_wrapper.hpp"

int main() {
    VecDB db;

    // Open in-memory database
    if (!db.Open(L"", true)) {
        std::cerr << "Failed to open database" << std::endl;
        return 1;
    }

    // Create collection
    if (!db.CreateCollection(L"documents", 128)) {
        std::cerr << "Failed to create collection" << std::endl;
        return 1;
    }

    // Insert vectors
    std::vector<float> embedding(128);
    for (int i = 0; i < 128; i++) {
        embedding[i] = static_cast<float>(rand()) / RAND_MAX;
    }

    uint64_t vid = db.Insert(L"documents", embedding.data(), 128);
    std::cout << "Inserted vector with ID: " << vid << std::endl;

    // Search
    auto results = db.Search(L"documents", embedding.data(), 128, 5);
    std::cout << "Found " << results.size() << " results:" << std::endl;
    for (const auto& [id, score] : results) {
        std::cout << "  ID: " << id << ", Score: " << score << std::endl;
    }

    db.Close();
    return 0;
}
```

---

## .NET Integration

### C++/CLI Wrapper

Create `VecDBWrapper.cpp`:

```cpp
// VecDBWrapper.cpp
#include "vecdb_wrapper.hpp"

namespace JinWoVecDB {

    public ref class VecDBManaged {
    public:
        VecDBManaged() {
            db_ = new VecDB();
        }

        ~VecDBManaged() {
            this->!VecDBManaged();
        }

        !VecDBManaged() {
            if (db_ != nullptr) {
                db_->Close();
                delete db_;
                db_ = nullptr;
            }
        }

        bool Open(String^ path, bool create) {
            std::wstring wpath;
            if (path != nullptr) {
                pin_ptr<const wchar_t> ptr = PtrToStringChars(path);
                wpath = ptr;
            }
            return db_->Open(wpath, create);
        }

        bool CreateCollection(String^ name, int dim) {
            pin_ptr<const wchar_t> ptr = PtrToStringChars(name);
            std::wstring wname(ptr);
            return db_->CreateCollection(wname, dim);
        }

        uint64_t Insert(String^ collName, array<float>^ vector) {
            pin_ptr<const wchar_t> ptr = PtrToStringChars(collName);
            std::wstring wname(ptr);

            pin_ptr<float> vecPtr = &vector[0];
            return db_->Insert(wname, vecPtr, vector->Length);
        }

        array<SearchResult>^ Search(String^ collName, array<float>^ query, int k) {
            pin_ptr<const wchar_t> ptr = PtrToStringChars(collName);
            std::wstring wname(ptr);

            pin_ptr<float> queryPtr = &query[0];
            auto results = db_->Search(wname, queryPtr, query->Length, k);

            array<SearchResult>^ managedResults = gcnew array<SearchResult>(results.size());
            for (size_t i = 0; i < results.size(); i++) {
                managedResults[i] = SearchResult(results[i].first, results[i].second);
            }

            return managedResults;
        }

        property bool IsOpen {
            bool get() { return db_->IsOpen(); }
        }

    private:
        VecDB* db_;
    };

    public value struct SearchResult {
        uint64_t Id;
        float Score;
    };
}
```

### C# Usage

```csharp
// Program.cs
using System;

namespace JinWoVecDB.NET
{
    class Program
    {
        static void Main(string[] args)
        {
            using (var db = new VecDBManaged())
            {
                // Open in-memory database
                if (!db.Open(null, true))
                {
                    Console.WriteLine("Failed to open database");
                    return;
                }

                Console.WriteLine($"Database opened: {db.IsOpen}");

                // Create collection
                if (!db.CreateCollection("documents", 128))
                {
                    Console.WriteLine("Failed to create collection");
                    return;
                }

                // Insert vectors
                var embedding = new float[128];
                var random = new Random();
                for (int i = 0; i < 128; i++)
                {
                    embedding[i] = (float)random.NextDouble();
                }

                ulong vid = db.Insert("documents", embedding);
                Console.WriteLine($"Inserted vector with ID: {vid}");

                // Search
                var results = db.Search("documents", embedding, 5);
                Console.WriteLine($"Found {results.Length} results:");
                foreach (var result in results)
                {
                    Console.WriteLine($"  ID: {result.Id}, Score: {result.Score}");
                }
            }
        }
    }
}
```

---

## Memory Management

### Windows-Specific Considerations

1. **Virtual Memory**: Windows provides virtual memory; monitor with Task Manager
2. **Heap Fragmentation**: Use arena allocator for predictable memory patterns
3. **DLL Loading**: Consider static linking for simpler deployment

### Best Practices

```cpp
// Configure arena size for Windows
jw_vecdb_config_t config = JW_VECDB_CONFIG_DEFAULT;
config.arena_size = 64 * 1024 * 1024; // 64MB
config.cache_size = 128 * 1024 * 1024; // 128MB

// Use memory-mapped files for large datasets
config.storage_mode = JW_STORAGE_MMAP;
```

---

## Performance Optimization

### SIMD on Windows

JinWo VecDB automatically detects and uses **SSE/AVX** SIMD instructions on Windows.

### Optimization Tips

1. **Use Static Linking**: Avoid DLL overhead
2. **AVX Support**: Compile with `/arch:AVX` or `/arch:AVX2` for newer CPUs
3. **Release Builds**: Always use Release configuration for production
4. **Bulk Operations**: Use batch APIs for better performance

### Compiler Flags

In Visual Studio **Property Pages**:

```
C/C++ → Code Generation → Enable Enhanced Instruction Set: /arch:AVX2
C/C++ → Optimization → Optimization: /O2
C/C++ → Optimization → Whole Program Optimization: /GL
Linker → Optimization → Whole Program Optimization: /LTCG
```

---

## Unicode and Encoding

### Windows-Specific Considerations

JinWo VecDB supports UTF-8 encoded paths on Windows 10+.

### Best Practices

```cpp
// Use UTF-8 paths (Windows 10+)
std::string path_u8 = "C:\\数据\\my_vecs.jwv";
db.Open(path_u8, true);

// Or use wide strings
std::wstring path_w = L"C:\\数据\\my_vecs.jwv";
db.Open(path_w, true);
```

### Character Encoding

```cpp
// Convert UTF-8 to UTF-16 for Windows API
std::wstring utf8_to_utf16(const std::string& utf8) {
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring utf16(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], size);
    return utf16;
}
```

---

## Troubleshooting

### Common Issues

#### 1. "LINK : fatal error LNK1181: cannot open 'jw_vecdb.lib'"

**Solution**: Ensure library path is correctly set:

```
Linker → General → Additional Library Directories: $(SolutionDir)lib
```

#### 2. "Access violation" on database operations

**Solution**: Ensure database is opened before any operations:
```cpp
if (!db.IsOpen()) {
    db.Open(path, true);
}
```

#### 3. "Stack overflow" with large vector dimensions

**Solution**: Increase stack size or use heap allocation:

```
Linker → System → Stack Reserve Size: 1048576
```

#### 4. DLL not found at runtime

**Solution**:
- Copy DLL to executable directory
- Or add DLL directory to PATH
- Or use static linking (`/MT` flag)

#### 5. Memory leaks detected

**Solution**: Ensure proper cleanup:
```cpp
// Always close database before cleanup
db.Close();
jw_cleanup();
```

### Debugging

Enable logging:

```cpp
jw_vecdb_set_log_callback([](int level, const char* msg, void* user_data) {
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}, nullptr);
```

Use **Visual Studio Debugger**:
1. Enable **Native Memory Debugging** (Debug → Options → Debugging)
2. Run under debugger
3. Check **Memory** window for leaks

### Performance Profiling

Use **Visual Studio Profiler**:
1. **Debug → Performance Profiler**
2. Select **CPU Usage** and **Memory Usage**
3. Analyze hot paths and memory allocations

---

## Next Steps

1. [API Reference Documentation](./api_reference.md)
2. [iOS Integration Guide](./ios_integration.md)
3. [Android Integration Guide](./android_integration.md)

---

**Document Version**: 1.0.0
**Last Updated**: 2026-04-26
