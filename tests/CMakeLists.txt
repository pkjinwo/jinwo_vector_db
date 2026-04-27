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

# 性能测试
add_executable(test_performance performance/test_performance.c)
target_link_libraries(test_performance PRIVATE jw_vecdb_static)
add_test(NAME test_performance COMMAND test_performance)