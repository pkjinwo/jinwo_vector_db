# JinWo VecDB Windows 平台集成指南

**版本**：v1.0.0
**生成时间**：2026-04-26
**文档类型**：对外发布

---

## 1. 平台概述

### 1.1 支持的Windows版本

| Windows版本 | 支持状态 | 最低版本 |
|------------|---------|----------|
| Windows 10 | ✅ 支持 | 1709 |
| Windows 11 | ✅ 支持 | 21H2 |
| Windows Server 2016 | ✅ 支持 | - |
| Windows Server 2019 | ✅ 支持 | - |
| Windows Server 2022 | ✅ 支持 | - |

### 1.2 系统要求

| 组件 | 要求 |
|------|------|
| CPU | x64 处理器 |
| 内存 | 至少 4GB RAM |
| 存储 | 至少 200MB 可用空间 |
| .NET Framework | 4.7.2+ |
| Visual Studio | 2022+ |
| CMake | 3.20+ |

---

## 2. 集成方式

### 2.1 NuGet 包集成

1. **打开 Visual Studio 项目**

2. **打开 NuGet 包管理器**
   - 右键点击项目 → 管理 NuGet 包

3. **搜索并安装**
   - 搜索 "JinWoVecDB"
   - 选择最新版本并安装

4. **验证安装**
   - 检查项目引用中是否包含 JinWoVecDB

### 2.2 CMake 集成（源码编译）

1. **克隆源码**
   ```bash
   git clone https://github.com/pkjinwo/jinwo_vector_db.git
   cd jinwo_vecdb
   ```

2. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

3. **配置 CMake**
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

4. **编译**
   ```bash
   cmake --build . --config Release
   ```

5. **添加到项目**
   - 将生成的 `jinwo_vecdb.lib` 和 `jinwo_vecdb.dll` 添加到项目
   - 添加头文件路径到包含目录

---

## 3. API 封装

### 3.1 C# 封装类

```csharp
// JinWoVecDB.cs
using System;
using System.Runtime.InteropServices;

namespace JinWoVecDB
{
    public class VecDBException : Exception
    {
        public int ErrorCode { get; }

        public VecDBException(int errorCode) : base(GetErrorMessage(errorCode))
        {
            ErrorCode = errorCode;
        }

        private static string GetErrorMessage(int errorCode)
        {
            switch (errorCode)
            {
                case 0: return "成功";
                case -1: return "无效参数";
                case -2: return "内存不足";
                case -3: return "文件系统错误";
                case -4: return "集合已存在";
                case -5: return "集合不存在";
                case -6: return "向量不存在";
                case -7: return "索引创建失败";
                case -8: return "维度不匹配";
                case -9: return "内部错误";
                default: return "未知错误";
            }
        }
    }

    public class Collection : IDisposable
    {
        private IntPtr _collectionPtr;
        private bool _disposed = false;

        internal Collection(IntPtr collectionPtr)
        {
            _collectionPtr = collectionPtr;
        }

        public ulong Insert(float[] vector)
        {
            ulong id = 0;
            int result = NativeMethods.jw_collection_insert(_collectionPtr, vector, ref id);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
            return id;
        }

        public SearchResult[] Search(float[] query, int k)
        {
            IntPtr resultsPtr = IntPtr.Zero;
            ulong resultCount = 0;
            int result = NativeMethods.jw_collection_search(_collectionPtr, query, (ulong)k, ref resultsPtr, ref resultCount);
            if (result != 0)
            {
                throw new VecDBException(result);
            }

            try
            {
                SearchResult[] results = new SearchResult[resultCount];
                for (int i = 0; i < resultCount; i++)
                {
                    IntPtr itemPtr = IntPtr.Add(resultsPtr, i * Marshal.SizeOf(typeof(SearchResult)));
                    results[i] = Marshal.PtrToStructure<SearchResult>(itemPtr);
                }
                return results;
            }
            finally
            {
                if (resultsPtr != IntPtr.Zero)
                {
                    NativeMethods.jw_free(resultsPtr);
                }
            }
        }

        public void Delete(ulong id)
        {
            int result = NativeMethods.jw_collection_delete(_collectionPtr, id);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
        }

        public void CreateIndex(string indexType, string parameters)
        {
            int result = NativeMethods.jw_collection_create_index(_collectionPtr, indexType, parameters);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_collectionPtr != IntPtr.Zero)
                {
                    NativeMethods.jw_collection_close(_collectionPtr);
                    _collectionPtr = IntPtr.Zero;
                }
                _disposed = true;
            }
        }

        ~Collection()
        {
            Dispose();
        }
    }

    public class VecDB : IDisposable
    {
        private IntPtr _dbPtr;
        private bool _disposed = false;

        public VecDB(string path, bool create = true)
        {
            IntPtr dbPtr = IntPtr.Zero;
            int result = NativeMethods.jw_vecdb_open(ref dbPtr, path, create);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
            _dbPtr = dbPtr;
        }

        public Collection CreateCollection(string name, int dimension)
        {
            IntPtr collectionPtr = IntPtr.Zero;
            int result = NativeMethods.jw_collection_create(ref collectionPtr, _dbPtr, name, dimension);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
            return new Collection(collectionPtr);
        }

        public Collection OpenCollection(string name)
        {
            IntPtr collectionPtr = IntPtr.Zero;
            int result = NativeMethods.jw_collection_open(ref collectionPtr, _dbPtr, name);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
            return new Collection(collectionPtr);
        }

        public void DropCollection(string name)
        {
            int result = NativeMethods.jw_collection_drop(_dbPtr, name);
            if (result != 0)
            {
                throw new VecDBException(result);
            }
        }

        public string[] ListCollections()
        {
            IntPtr namesPtr = IntPtr.Zero;
            ulong count = 0;
            int result = NativeMethods.jw_collection_list(_dbPtr, ref namesPtr, ref count);
            if (result != 0)
            {
                throw new VecDBException(result);
            }

            try
            {
                string[] names = new string[count];
                for (int i = 0; i < count; i++)
                {
                    IntPtr namePtr = Marshal.ReadIntPtr(IntPtr.Add(namesPtr, i * IntPtr.Size));
                    names[i] = Marshal.PtrToStringAnsi(namePtr);
                    NativeMethods.jw_free(namePtr);
                }
                return names;
            }
            finally
            {
                if (namesPtr != IntPtr.Zero)
                {
                    NativeMethods.jw_free(namesPtr);
                }
            }
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_dbPtr != IntPtr.Zero)
                {
                    NativeMethods.jw_vecdb_close(_dbPtr);
                    _dbPtr = IntPtr.Zero;
                }
                _disposed = true;
            }
        }

        ~VecDB()
        {
            Dispose();
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SearchResult
    {
        public ulong id;
        public float distance;
    }

    internal static class NativeMethods
    {
        private const string DllName = "jinwo_vecdb.dll";

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_vecdb_open(ref IntPtr db, string path, bool create);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_vecdb_close(IntPtr db);

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_create(ref IntPtr collection, IntPtr db, string name, int dimension);

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_open(ref IntPtr collection, IntPtr db, string name);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_close(IntPtr collection);

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_drop(IntPtr db, string name);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_list(IntPtr db, ref IntPtr names, ref ulong count);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_insert(IntPtr collection, float[] vector, ref ulong id);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_search(IntPtr collection, float[] query, ulong k, ref IntPtr results, ref ulong count);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_delete(IntPtr collection, ulong id);

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_create_index(IntPtr collection, string type, string params);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void jw_free(IntPtr ptr);
    }
}
```

### 3.2 C++/CLI 封装

```cpp
// JinWoVecDB.h
#pragma once

using namespace System;
using namespace System::Collections::Generic;

namespace JinWoVecDB {

    public ref class VecDBException : public Exception
    {
    private:
        int errorCode;
    public:
        property int ErrorCode { int get() { return errorCode; } }

        VecDBException(int code) : Exception(GetErrorMessage(code))
        {
            errorCode = code;
        }

    private:
        static String^ GetErrorMessage(int errorCode);
    };

    public ref class SearchResult
    {
    public:
        property UInt64 Id { UInt64 get(); void set(UInt64 value); }
        property float Distance { float get(); void set(float value); }

    private:
        UInt64 id;
        float distance;
    };

    public ref class Collection : public IDisposable
    {
    public:
        UInt64 Insert(array<float>^ vector);
        array<SearchResult^>^ Search(array<float>^ query, int k);
        void Delete(UInt64 id);
        void CreateIndex(String^ indexType, String^ parameters);

        virtual void Dispose() sealed;
        !Collection();

    internal:
        Collection(IntPtr collectionPtr);

    private:
        IntPtr collectionPtr;
        bool disposed;
    };

    public ref class VecDB : public IDisposable
    {
    public:
        VecDB(String^ path, bool create);
        Collection^ CreateCollection(String^ name, int dimension);
        Collection^ OpenCollection(String^ name);
        void DropCollection(String^ name);
        array<String^>^ ListCollections();

        virtual void Dispose() sealed;
        !VecDB();

    private:
        IntPtr dbPtr;
        bool disposed;
    };
}

// JinWoVecDB.cpp
#include "JinWoVecDB.h"
#include <msclr/marshal_cppstd.h>
#include "jw_vecdb.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace msclr::interop;

namespace JinWoVecDB {

    String^ VecDBException::GetErrorMessage(int errorCode)
    {
        switch (errorCode)
        {
        case 0: return "成功";
        case -1: return "无效参数";
        case -2: return "内存不足";
        case -3: return "文件系统错误";
        case -4: return "集合已存在";
        case -5: return "集合不存在";
        case -6: return "向量不存在";
        case -7: return "索引创建失败";
        case -8: return "维度不匹配";
        case -9: return "内部错误";
        default: return "未知错误";
        }
    }

    UInt64 SearchResult::Id::get() { return id; }
    void SearchResult::Id::set(UInt64 value) { id = value; }
    float SearchResult::Distance::get() { return distance; }
    void SearchResult::Distance::set(float value) { distance = value; }

    Collection::Collection(IntPtr collectionPtr) : collectionPtr(collectionPtr), disposed(false) {}

    Collection::~Collection()
    {
        Dispose();
    }

    void Collection::Dispose()
    {
        if (!disposed)
        {
            if (collectionPtr != IntPtr::Zero)
            {
                jw_collection_close((jw_collection_t*)collectionPtr.ToPointer());
                collectionPtr = IntPtr::Zero;
            }
            disposed = true;
            GC::SuppressFinalize(this);
        }
    }

    Collection::!Collection()
    {
        if (!disposed && collectionPtr != IntPtr::Zero)
        {
            jw_collection_close((jw_collection_t*)collectionPtr.ToPointer());
        }
    }

    UInt64 Collection::Insert(array<float>^ vector)
    {
        pin_ptr<float> vectorPtr = &vector[0];
        uint64_t id = 0;
        int result = jw_collection_insert((jw_collection_t*)collectionPtr.ToPointer(), vectorPtr, &id);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
        return id;
    }

    array<SearchResult^>^ Collection::Search(array<float>^ query, int k)
    {
        pin_ptr<float> queryPtr = &query[0];
        jw_search_result_t* results = nullptr;
        size_t count = 0;
        int result = jw_collection_search((jw_collection_t*)collectionPtr.ToPointer(), queryPtr, k, &results, &count);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }

        try
        {
            array<SearchResult^>^ resultArray = gcnew array<SearchResult^>(count);
            for (size_t i = 0; i < count; i++)
            {
                SearchResult^ item = gcnew SearchResult();
                item->Id = results[i].id;
                item->Distance = results[i].distance;
                resultArray[i] = item;
            }
            return resultArray;
        }
        finally
        {
            if (results != nullptr)
            {
                free(results);
            }
        }
    }

    void Collection::Delete(UInt64 id)
    {
        int result = jw_collection_delete((jw_collection_t*)collectionPtr.ToPointer(), id);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
    }

    void Collection::CreateIndex(String^ indexType, String^ parameters)
    {
        std::string type = marshal_as<std::string>(indexType);
        std::string params = marshal_as<std::string>(parameters);
        int result = jw_collection_create_index((jw_collection_t*)collectionPtr.ToPointer(), type.c_str(), params.c_str());
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
    }

    VecDB::VecDB(String^ path, bool create) : disposed(false)
    {
        std::string pathStr = marshal_as<std::string>(path);
        jw_vecdb_t* db = nullptr;
        int result = jw_vecdb_open(&db, pathStr.c_str(), create);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
        dbPtr = IntPtr(db);
    }

    VecDB::~VecDB()
    {
        Dispose();
    }

    void VecDB::Dispose()
    {
        if (!disposed)
        {
            if (dbPtr != IntPtr::Zero)
            {
                jw_vecdb_close((jw_vecdb_t*)dbPtr.ToPointer());
                dbPtr = IntPtr::Zero;
            }
            disposed = true;
            GC::SuppressFinalize(this);
        }
    }

    VecDB::!VecDB()
    {
        if (!disposed && dbPtr != IntPtr::Zero)
        {
            jw_vecdb_close((jw_vecdb_t*)dbPtr.ToPointer());
        }
    }

    Collection^ VecDB::CreateCollection(String^ name, int dimension)
    {
        std::string nameStr = marshal_as<std::string>(name);
        jw_collection_t* collection = nullptr;
        int result = jw_collection_create(&collection, (jw_vecdb_t*)dbPtr.ToPointer(), nameStr.c_str(), dimension);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
        return gcnew Collection(IntPtr(collection));
    }

    Collection^ VecDB::OpenCollection(String^ name)
    {
        std::string nameStr = marshal_as<std::string>(name);
        jw_collection_t* collection = nullptr;
        int result = jw_collection_open(&collection, (jw_vecdb_t*)dbPtr.ToPointer(), nameStr.c_str());
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
        return gcnew Collection(IntPtr(collection));
    }

    void VecDB::DropCollection(String^ name)
    {
        std::string nameStr = marshal_as<std::string>(name);
        int result = jw_collection_drop((jw_vecdb_t*)dbPtr.ToPointer(), nameStr.c_str());
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }
    }

    array<String^>^ VecDB::ListCollections()
    {
        char** names = nullptr;
        size_t count = 0;
        int result = jw_collection_list((jw_vecdb_t*)dbPtr.ToPointer(), &names, &count);
        if (result != 0)
        {
            throw gcnew VecDBException(result);
        }

        try
        {
            array<String^>^ resultArray = gcnew array<String^>(count);
            for (size_t i = 0; i < count; i++)
            {
                resultArray[i] = gcnew String(names[i]);
                free(names[i]);
            }
            return resultArray;
        }
        finally
        {
            if (names != nullptr)
            {
                free(names);
            }
        }
    }
}
```

---

## 4. 示例代码

### 4.1 C# 示例

```csharp
// Program.cs
using System;
using JinWoVecDB;

namespace VecDBDemo
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                // 打开数据库
                string dbPath = @"C:\vecdb";
                using (var vecdb = new VecDB(dbPath, true))
                {
                    // 创建集合
                    using (var collection = vecdb.CreateCollection("test", 128))
                    {
                        // 插入向量
                        float[] vector = new float[128];
                        for (int i = 0; i < 128; i++)
                        {
                            vector[i] = (float)i / 128.0f;
                        }
                        ulong id = collection.Insert(vector);
                        Console.WriteLine($"Inserted vector with id: {id}");

                        // 搜索向量
                        var results = collection.Search(vector, 10);
                        Console.WriteLine($"Found {results.Length} results:");
                        for (int i = 0; i < results.Length; i++)
                        {
                            Console.WriteLine($"  {i}: id={results[i].id}, distance={results[i].distance}");
                        }

                        // 删除向量
                        collection.Delete(id);
                        Console.WriteLine($"Deleted vector with id: {id}");
                    }
                }
            }
            catch (VecDBException ex)
            {
                Console.WriteLine($"VecDB error: {ex.Message} (Code: {ex.ErrorCode})");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Unexpected error: {ex.Message}");
            }

            Console.WriteLine("Press any key to exit...");
            Console.ReadKey();
        }
    }
}
```

### 4.2 C++/CLI 示例

```cpp
// Program.cpp
#include "JinWoVecDB.h"
using namespace System;
using namespace JinWoVecDB;

int main(array<String^>^ args)
{
    try
    {
        // 打开数据库
        String^ dbPath = "C:\\vecdb";
        VecDB^ vecdb = gcnew VecDB(dbPath, true);
        
        // 创建集合
        Collection^ collection = vecdb->CreateCollection("test", 128);
        
        // 插入向量
        array<float>^ vector = gcnew array<float>(128);
        for (int i = 0; i < 128; i++)
        {
            vector[i] = (float)i / 128.0f;
        }
        UInt64 id = collection->Insert(vector);
        Console::WriteLine("Inserted vector with id: {0}", id);
        
        // 搜索向量
        array<SearchResult^>^ results = collection->Search(vector, 10);
        Console::WriteLine("Found {0} results:", results->Length);
        for (int i = 0; i < results->Length; i++)
        {
            Console::WriteLine("  {0}: id={1}, distance={2}", i, results[i]->Id, results[i]->Distance);
        }
        
        // 删除向量
        collection->Delete(id);
        Console::WriteLine("Deleted vector with id: {0}", id);
        
        // 清理
        delete collection;
        delete vecdb;
    }
    catch (VecDBException^ ex)
    {
        Console::WriteLine("VecDB error: {0} (Code: {1})", ex->Message, ex->ErrorCode);
    }
    catch (Exception^ ex)
    {
        Console::WriteLine("Unexpected error: {0}", ex->Message);
    }
    
    Console::WriteLine("Press any key to exit...");
    Console::ReadKey();
    return 0;
}
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
| 线程管理 | 使用 Task 处理数据库操作 |
| 异步操作 | 使用 async/await 处理长时间操作 |
| 批量操作 | 合并多个操作减少线程切换 |
| 超时处理 | 设置合理的操作超时时间 |

### 5.3 存储优化

| 优化项 | 建议 |
|-------|------|
| 存储路径 | 使用应用数据目录 |
| 数据压缩 | 启用向量压缩减少存储 |
| 定期清理 | 定期清理不需要的数据 |
| 备份策略 | 实现数据备份机制 |

---

## 6. 权限配置

### 6.1 Windows 权限

| 权限 | 用途 | 是否必需 |
|------|------|----------|
| 读写权限 | 数据库文件访问 | 是 |
| 管理员权限 | 某些系统目录访问 | 否 |

### 6.2 推荐路径

| 路径类型 | 用途 | 示例路径 |
|---------|------|----------|
| 应用数据目录 | 应用专用数据 | `%APPDATA%\YourApp\vecdb` |
| 程序数据目录 | 共享数据 | `%ProgramData%\YourApp\vecdb` |
| 临时目录 | 临时数据 | `%TEMP%\vecdb` |

---

## 7. 常见问题

### 7.1 编译问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 找不到 DLL | DLL 未复制到输出目录 | 将 DLL 复制到输出目录 |
| 编译错误 | 缺少依赖 | 检查项目引用 |
| 链接错误 | 库文件缺失 | 确保所有库文件都被包含 |

### 7.2 运行时问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 找不到 DLL | DLL 未在 PATH 中 | 将 DLL 所在目录添加到 PATH |
| 内存不足 | 向量数据过大 | 减少批处理大小 |
| 权限错误 | 缺少文件访问权限 | 检查文件权限 |
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

```csharp
// 启用详细日志
System.Environment.SetEnvironmentVariable("jinwo.vecdb.log.level", "debug");

// 自定义日志处理
class VecDBLogger
{
    public static void Log(string message)
    {
        Console.WriteLine($"[VecDB] {message}");
    }
    
    public static void LogError(string message, Exception ex = null)
    {
        if (ex != null)
        {
            Console.WriteLine($"[VecDB ERROR] {message}: {ex.Message}");
        }
        else
        {
            Console.WriteLine($"[VecDB ERROR] {message}");
        }
    }
}
```

### 8.2 性能分析

| 工具 | 用途 | 使用方法 |
|------|------|----------|
| Visual Studio Profiler | 性能分析 | 调试 → 性能分析器 |
| dotTrace | .NET 性能分析 | 安装并运行 |
| Process Explorer | 进程监控 | 监控内存和CPU使用 |

### 8.3 错误处理

```csharp
try
{
    var id = collection.Insert(vector);
    // 处理成功
}
catch (VecDBException ex)
{
    VecDBLogger.LogError("Insert failed", ex);
    // 处理错误
}
```

---

## 9. 发布注意事项

### 9.1 应用打包

| 配置项 | 要求 |
|--------|------|
| DLL 复制 | 将 DLL 复制到输出目录 |
| 安装程序 | 包含 DLL 在安装包中 |
| 版本兼容性 | 确保 DLL 版本匹配 |

### 9.2 架构支持

| 架构 | 支持状态 |
|------|----------|
| x64 | ✅ 支持 |
| x86 | ❌ 不支持 |
| ARM64 | ❌ 不支持 |

### 9.3 Windows 认证

| 认证项 | 要求 |
|--------|------|
| 数字签名 | 推荐 |
| Windows Defender | 确保通过扫描 |
| 应用兼容性 | 符合 Windows 要求 |

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
