# tests/CMakeLists.txt

add_executable(test_types test_types.c)
target_link_libraries(test_types PRIVATE jw_vecdb_static)
add_test(NAME test_types COMMAND test_types)

add_executable(test_pool test_pool.c)
target_link_libraries(test_pool PRIVATE jw_vecdb_static)
add_test(NAME test_pool COMMAND test_pool)

add_executable(test_vector test_vector.c)
target_link_libraries(test_vector PRIVATE jw_vecdb_static)
add_test(NAME test_vector COMMAND test_vector)

add_executable(test_string test_string.c)
target_link_libraries(test_string PRIVATE jw_vecdb_static)
add_test(NAME test_string COMMAND test_string)

add_executable(test_math test_math.c)
target_link_libraries(test_math PRIVATE jw_vecdb_static)
add_test(NAME test_math COMMAND test_math)

add_executable(test_sort test_sort.c)
target_link_libraries(test_sort PRIVATE jw_vecdb_static)
add_test(NAME test_sort COMMAND test_sort)

add_executable(test_hash test_hash.c)
target_link_libraries(test_hash PRIVATE jw_vecdb_static)
add_test(NAME test_hash COMMAND test_hash)

# test_lock 需要 jw_lock.c 实现原子操作，先注释
# add_executable(test_lock test_lock.c)
# target_link_libraries(test_lock PRIVATE jw_vecdb_static)
# add_test(NAME test_lock COMMAND test_lock)

add_executable(test_file test_file.c)
target_link_libraries(test_file PRIVATE jw_vecdb_static)
add_test(NAME test_file COMMAND test_file)

add_executable(test_storage test_storage.c)
target_link_libraries(test_storage PRIVATE jw_vecdb_static)
add_test(NAME test_storage COMMAND test_storage)

add_executable(test_index test_index.c)
target_link_libraries(test_index PRIVATE jw_vecdb_static)
add_test(NAME test_index COMMAND test_index)

# jw_index.c 核心逻辑完整测试 (类型×度量矩阵/维度边界/存盘重载/稀疏vid/删除/批量搜索/边界)
add_executable(test_index_core test_index_core.c)
target_link_libraries(test_index_core PRIVATE jw_vecdb_static)
add_test(NAME test_index_core COMMAND test_index_core)

# 2026-10-22 补齐的 24 个向量函数测试 (统计/归一化/叉积/批量/SIMD)
add_executable(test_vector_extra test_vector_extra.c)
target_link_libraries(test_vector_extra PRIVATE jw_vecdb_static)
add_test(NAME test_vector_extra COMMAND test_vector_extra)

# 2026-10-23 回归测试: 固化 HANDOFF §7 实测结论 (度量召回/PQ/HNSW_PQ缺陷/边界/性能缩放)
add_executable(test_regression test_regression.c)
target_link_libraries(test_regression PRIVATE jw_vecdb_static)
add_test(NAME test_regression COMMAND test_regression)

add_executable(test_vecdb test_vecdb.c)
target_link_libraries(test_vecdb PRIVATE jw_vecdb_static)
add_test(NAME test_vecdb COMMAND test_vecdb)

add_executable(test_config test_config.c)
target_link_libraries(test_config PRIVATE jw_vecdb_static)
add_test(NAME test_config COMMAND test_config)

add_executable(test_quantization test_quantization.c)
target_link_libraries(test_quantization PRIVATE jw_vecdb_static)
add_test(NAME test_quantization COMMAND test_quantization)

add_executable(test_collection test_collection.c)
target_link_libraries(test_collection PRIVATE jw_vecdb_static)
add_test(NAME test_collection COMMAND test_collection)

add_executable(test_quantization_serialization_mmap test_quantization_serialization_mmap.c)
target_link_libraries(test_quantization_serialization_mmap PRIVATE jw_vecdb_static)
add_test(NAME test_quantization_serialization_mmap COMMAND test_quantization_serialization_mmap)

# 性能测试 (2026-10-22 暂时从构建中移除)
# 原因: performance/test_performance.c 使用的是过时 API, 从未能编译通过:
#   - jw_str_t 按值传给期望 const jw_str_t * 的参数
#   - jw_collection_close() 在库中不存在
#   - jw_collection_search 第 7 个参数传 int 而非 const jw_search_options_t *
# 该文件测的是性能而非正确性, 且已长期失效。待重写为当前 API 后再启用。
# add_executable(test_performance performance/test_performance.c)
# target_link_libraries(test_performance PRIVATE jw_vecdb_static)
# add_test(NAME test_performance COMMAND test_performance)