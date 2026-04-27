using System;
using System.Runtime.InteropServices;

namespace JinWoVecDBDemo
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("========================================");
            Console.WriteLine("  JinWo VecDB Windows C# 演示程序");
            Console.WriteLine("========================================\n");

            try
            {
                // 打开数据库
                Console.WriteLine("打开数据库...");
                IntPtr dbPtr = IntPtr.Zero;
                int result = NativeMethods.jw_vecdb_open(ref dbPtr, "./vecdb", true);
                if (result != 0)
                {
                    throw new Exception($"打开数据库失败: {result}");
                }
                Console.WriteLine("数据库打开成功");

                // 获取版本
                IntPtr versionPtr = NativeMethods.jw_vecdb_version();
                string version = Marshal.PtrToStringAnsi(versionPtr);
                Console.WriteLine($"版本: {version}\n");

                // 创建集合
                Console.WriteLine("创建集合...");
                IntPtr collectionPtr = IntPtr.Zero;
                result = NativeMethods.jw_collection_create(ref collectionPtr, dbPtr, "test", 128);
                if (result != 0)
                {
                    throw new Exception($"创建集合失败: {result}");
                }
                Console.WriteLine("集合创建成功: test\n");

                // 插入向量
                Console.WriteLine("插入10个向量...");
                Random random = new Random();
                for (int i = 0; i < 10; i++)
                {
                    float[] vector = new float[128];
                    for (int j = 0; j < 128; j++)
                    {
                        vector[j] = (float)(random.NextDouble() * 2.0 - 1.0);
                    }
                    ulong id = 0;
                    result = NativeMethods.jw_collection_insert(collectionPtr, vector, ref id);
                    if (result != 0)
                    {
                        Console.WriteLine($"插入向量 {i} 失败: {result}");
                    }
                    else
                    {
                        Console.WriteLine($"插入向量 {i} 成功，ID: {id}");
                    }
                }
                Console.WriteLine("插入完成\n");

                // 搜索向量
                Console.WriteLine("搜索相似向量...");
                float[] query = new float[128];
                for (int j = 0; j < 128; j++)
                {
                    query[j] = (float)(random.NextDouble() * 2.0 - 1.0);
                }

                IntPtr resultsPtr = IntPtr.Zero;
                ulong resultCount = 0;
                result = NativeMethods.jw_collection_search(collectionPtr, query, 5, ref resultsPtr, ref resultCount);
                if (result != 0)
                {
                    throw new Exception($"搜索失败: {result}");
                }

                if (resultCount > 0)
                {
                    Console.WriteLine($"搜索结果 (前{resultCount}个):");
                    for (int i = 0; i < resultCount; i++)
                    {
                        IntPtr itemPtr = IntPtr.Add(resultsPtr, i * Marshal.SizeOf(typeof(SearchResult)));
                        SearchResult resultItem = Marshal.PtrToStructure<SearchResult>(itemPtr);
                        Console.WriteLine($"ID: {resultItem.id}, 距离: {resultItem.distance:F4}");
                    }
                }
                else
                {
                    Console.WriteLine("搜索失败，无结果");
                }

                // 清理
                if (resultsPtr != IntPtr.Zero)
                {
                    NativeMethods.jw_free(resultsPtr);
                }
                NativeMethods.jw_collection_close(collectionPtr);
                NativeMethods.jw_vecdb_close(dbPtr);

                Console.WriteLine("\n演示完成!");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"错误: {ex.Message}");
            }

            Console.WriteLine("\n按任意键退出...");
            Console.ReadKey();
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct SearchResult
    {
        public ulong id;
        public float distance;
    }

    static class NativeMethods
    {
        private const string DllName = "jinwo_vecdb.dll";

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_vecdb_open(ref IntPtr db, string path, bool create);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_vecdb_close(IntPtr db);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr jw_vecdb_version();

        [DllImport(DllName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_create(ref IntPtr collection, IntPtr db, string name, int dimension);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_close(IntPtr collection);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_insert(IntPtr collection, float[] vector, ref ulong id);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int jw_collection_search(IntPtr collection, float[] query, ulong k, ref IntPtr results, ref ulong count);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void jw_free(IntPtr ptr);
    }
}
