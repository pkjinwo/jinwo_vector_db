/*
 * jw_file.h - JinWo VecDB 文件工具
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * =============================================================================
 * 
 * 文件工具说明:
 * 
 * 提供文件操作相关的工具函数，包括:
 *   - 文件读写
 *   - 文件状态检查
 *   - 目录操作
 *   - 文件路径操作
 * 
 * 版本: 0.1.20
 * 作者: 灵活就业码农
 */

#ifndef JW_FILE_H
#define JW_FILE_H

#include "jw_types.h"
#include "jw_arena.h"
#include "jw_string.h"

JW_BEGIN_DECL

/*
 * =============================================================================
 * 文件操作
 * =============================================================================
 */

/**
 * 打开文件
 *
 * @param path 文件路径
 * @param mode 打开模式 ("r", "w", "a", "rb", "wb", etc.)
 * @return 文件句柄，失败返回JW_INVALID_OS_HANDLE
 */
JW_API jw_os_handle_t jw_file_open(const jw_str_t *path, const jw_str_t *mode);

/**
 * 关闭文件
 *
 * @param handle 文件句柄
 * @return 0表示成功，非0表示失败
 */
JW_API int jw_file_close(jw_os_handle_t handle);

/**
 * 读取文件内容
 *
 * @param handle 文件句柄
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 实际读取的字节数，失败返回-1
 */
JW_API jw_ssize_t jw_file_read(jw_os_handle_t handle, void *buffer, jw_size_t size);

/**
 * 写入文件内容
 *
 * @param handle 文件句柄
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 实际写入的字节数，失败返回-1
 */
JW_API jw_ssize_t jw_file_write(jw_os_handle_t handle, const void *buffer, jw_size_t size);

/**
 * 移动文件指针
 *
 * @param handle 文件句柄
 * @param offset 偏移量
 * @param whence 起始位置 (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return 新的文件位置，失败返回-1
 */
JW_API jw_off_t jw_file_seek(jw_os_handle_t handle, jw_off_t offset, int whence);

/**
 * 读取整个文件
 * 
 * @param path 文件路径
 * @param size 输出文件大小
 * @return 文件内容，失败返回NULL
 */
JW_API char *jw_file_read_all(const jw_str_t *path, jw_size_t *size);

/**
 * 写入整个文件
 * 
 * @param path 文件路径
 * @param content 文件内容
 * @param size 文件大小
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_write_all(const jw_str_t *path, const void *content, jw_size_t size);

/**
 * 获取文件大小
 * 
 * @param path 文件路径
 * @return 文件大小，失败返回-1
 */
JW_API jw_int64_t jw_file_size(const jw_str_t *path);

/**
 * 检查文件是否存在
 * 
 * @param path 文件路径
 * @return JW_TRUE 存在
 */
JW_API jw_bool_t jw_file_exists(const jw_str_t *path);

/**
 * 检查是否为文件
 * 
 * @param path 文件路径
 * @return JW_TRUE 是文件
 */
JW_API jw_bool_t jw_file_is_regular(const jw_str_t *path);

/**
 * 检查是否为目录
 * 
 * @param path 文件路径
 * @return JW_TRUE 是目录
 */
JW_API jw_bool_t jw_file_is_directory(const jw_str_t *path);

/*
 * =============================================================================
 * 目录操作
 * =============================================================================
 */

/**
 * 创建目录
 * 
 * @param path 目录路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_mkdir(const jw_str_t *path);

/**
 * 创建目录（递归）
 * 
 * @param path 目录路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_mkdir_recursive(const jw_str_t *path);

/**
 * 删除目录
 * 
 * @param path 目录路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_rmdir(const jw_str_t *path);

/**
 * 删除目录（递归）
 * 
 * @param path 目录路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_rmdir_recursive(const jw_str_t *path);

/**
 * 列出目录内容
 * 
 * @param path 目录路径
 * @param arena 内存池
 * @return 文件名数组，最后一个元素为NULL
 */
JW_API char **jw_file_list_dir(const jw_str_t *path, jw_arena_t *arena);

/*
 * =============================================================================
 * 文件路径操作
 * =============================================================================
 */

/**
 * 获取文件名
 * 
 * @param path 文件路径
 * @return 文件名
 */
JW_API const char *jw_file_basename(const jw_str_t *path);

/**
 * 获取目录名
 * 
 * @param path 文件路径
 * @param arena 内存池
 * @return 目录名
 */
JW_API char *jw_file_dirname(jw_arena_t *arena, const jw_str_t *path);

/**
 * 拼接路径
 * 
 * @param arena 内存池
 * @param ... 路径组件
 * @return 拼接后的路径
 */
JW_API char *jw_file_join(jw_arena_t *arena, ...);

/**
 * 获取绝对路径
 * 
 * @param path 相对路径
 * @param arena 内存池
 * @return 绝对路径
 */
JW_API char *jw_file_abs_path(jw_arena_t *arena, const jw_str_t *path);

/**
 * 规范化路径
 * 
 * @param path 路径
 * @param arena 内存池
 * @return 规范化后的路径
 */
JW_API char *jw_file_normalize_path(jw_arena_t *arena, const jw_str_t *path);

/*
 * =============================================================================
 * 文件系统操作
 * =============================================================================
 */

/**
 * 删除文件
 * 
 * @param path 文件路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_unlink(const jw_str_t *path);

/**
 * 重命名文件
 * 
 * @param old_path 旧路径
 * @param new_path 新路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_rename(const jw_str_t *old_path, const jw_str_t *new_path);

/**
 * 复制文件
 * 
 * @param src_path 源路径
 * @param dest_path 目标路径
 * @return JW_SUCCESS 成功
 */
JW_API jw_status_t jw_file_copy(const jw_str_t *src_path, const jw_str_t *dest_path);

/**
 * 获取文件修改时间
 * 
 * @param path 文件路径
 * @return 修改时间戳，失败返回-1
 */
JW_API jw_int64_t jw_file_mtime(const jw_str_t *path);

/*
 * =============================================================================
 * 临时文件
 * =============================================================================
 */

/**
 * 创建临时文件
 *
 * @param path_template 模板 (包含XXXXXX)
 * @return 临时文件路径
 */
JW_API char *jw_file_mktemp(char *path_template);

/**
 * 创建临时目录
 *
 * @param path_template 模板 (包含XXXXXX)
 * @return 临时目录路径
 */
JW_API char *jw_file_mkdtemp(char *path_template);

JW_END_DECL

#endif /* JW_FILE_H */
