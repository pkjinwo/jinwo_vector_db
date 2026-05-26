/*
 * jw_file.c - JinWo VecDB 文件工具实现
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
 */

#include "jw_file.h"
#include "jw_arena.h"
#include <limits.h>
#include "jw_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Portable fallback for S_ISREG/S_ISDIR.
 * On some compilers (e.g., older MSVC), these macros are missing.
 * We use POSIX standard octal values to avoid dependency on S_IFMT etc.
 */
#ifndef S_ISREG
#define S_ISREG(m) (((m) & 0170000) == 0100000)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & 0170000) == 0040000)
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
#include <windows.h>
#include <io.h>
#include <direct.h>

struct dirent {
    char d_name[MAX_PATH];
};

typedef struct DIR {
    HANDLE handle;
    WIN32_FIND_DATAA data;
    char pattern[MAX_PATH];
    struct dirent result;
    int first;
} DIR;

DIR *opendir(const char *name)
{
    if (!name) {
        return NULL;
    }

    size_t len = strlen(name);
    if (len + 3 >= MAX_PATH) {
        return NULL;
    }

    DIR *dir = (DIR *)malloc(sizeof(DIR));
    if (!dir) {
        return NULL;
    }

    strcpy(dir->pattern, name);
    for (size_t i = 0; i < len; ++i) {
        if (dir->pattern[i] == '/') {
            dir->pattern[i] = '\\';
        }
    }

    if (len > 0 && (dir->pattern[len - 1] == '\\' || dir->pattern[len - 1] == '/')) {
        strcat(dir->pattern, "*");
    } else {
        strcat(dir->pattern, "\\*");
    }

    dir->handle = FindFirstFileA(dir->pattern, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }

    dir->first = 1;
    return dir;
}

struct dirent *readdir(DIR *dir)
{
    if (!dir) {
        return NULL;
    }

    if (dir->first) {
        dir->first = 0;
    } else {
        if (!FindNextFileA(dir->handle, &dir->data)) {
            return NULL;
        }
    }

    strncpy(dir->result.d_name, dir->data.cFileName, MAX_PATH - 1);
    dir->result.d_name[MAX_PATH - 1] = '\0';
    return &dir->result;
}

void rewinddir(DIR *dir)
{
    if (!dir) {
        return;
    }
    if (dir->handle != INVALID_HANDLE_VALUE) {
        FindClose(dir->handle);
    }
    dir->handle = FindFirstFileA(dir->pattern, &dir->data);
    dir->first = 1;
}

int closedir(DIR *dir)
{
    if (!dir) {
        return 0;
    }
    int result = 0;
    if (dir->handle != INVALID_HANDLE_VALUE) {
        result = FindClose(dir->handle) ? 0 : -1;
    }
    free(dir);
    return result;
}

#define open  _open
#define close _close
#define read  _read
#define write _write
#define lseek _lseek
#define unlink _unlink
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <dirent.h>
#endif

/*
 * =============================================================================
 * 文件操作
 * =============================================================================
 */

JW_API jw_os_handle_t jw_file_open(const jw_str_t *path, const jw_str_t *mode)
{
    int flags = 0;
    const char *mode_str = jw_str_cstr(mode);
    
    if (mode_str[0] == 'r' && mode_str[1] == '\0') {
        flags = O_RDONLY;
    } else if (mode_str[0] == 'r' && mode_str[1] == 'b') {
        flags = O_RDONLY;
    } else if (mode_str[0] == 'w' && mode_str[1] == '\0') {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mode_str[0] == 'w' && mode_str[1] == 'b') {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mode_str[0] == 'a' && mode_str[1] == '\0') {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (mode_str[0] == 'a' && mode_str[1] == 'b') {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (mode_str[0] == 'r' && mode_str[1] == '+') {
        flags = O_RDWR;
    } else if (mode_str[0] == 'w' && mode_str[1] == '+') {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    }
    
    int fd = open(jw_str_cstr(path), flags, 0644);
    if (fd < 0) {
        return JW_INVALID_OS_HANDLE;
    }
    return (jw_os_handle_t)fd;
}

JW_API int jw_file_close(jw_os_handle_t handle)
{
    return close((int)handle);
}

JW_API jw_ssize_t jw_file_read(jw_os_handle_t handle, void *buffer, jw_size_t size)
{
    return read((int)handle, buffer, size);
}

JW_API jw_ssize_t jw_file_write(jw_os_handle_t handle, const void *buffer, jw_size_t size)
{
    return write((int)handle, buffer, size);
}

JW_API jw_off_t jw_file_seek(jw_os_handle_t handle, jw_off_t offset, int whence)
{
    return lseek((int)handle, offset, whence);
}

JW_API char *jw_file_read_all(const jw_str_t *path, jw_size_t *size)
{
    jw_str_t mode_rb = jw_str("rb");
    jw_os_handle_t handle = jw_file_open(path, &mode_rb);
    if (handle == JW_INVALID_OS_HANDLE) {
        return NULL;
    }
    
    /* 获取文件大小 */
    jw_off_t file_size = jw_file_seek(handle, 0, SEEK_END);
    if (file_size < 0) {
        jw_file_close(handle);
        return NULL;
    }
    jw_file_seek(handle, 0, SEEK_SET);
    
    /* 分配内存 */
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        jw_file_close(handle);
        return NULL;
    }
    
    /* 读取文件 */
    jw_size_t read_size = jw_file_read(handle, buffer, file_size);
    if (read_size != file_size) {
        free(buffer);
        jw_file_close(handle);
        return NULL;
    }
    
    buffer[file_size] = '\0';
    
    if (size) {
        *size = file_size;
    }
    
    jw_file_close(handle);
    return buffer;
}

JW_API jw_status_t jw_file_write_all(const jw_str_t *path, const void *content, jw_size_t size)
{
    jw_str_t mode_wb = jw_str("wb");
    jw_os_handle_t handle = jw_file_open(path, &mode_wb);
    if (handle == JW_INVALID_OS_HANDLE) {
        return JW_IO_ERROR;
    }
    
    jw_size_t written = jw_file_write(handle, content, size);
    jw_file_close(handle);
    
    if (written != size) {
        return JW_IO_ERROR;
    }
    
    return JW_SUCCESS;
}

JW_API jw_int64_t jw_file_size(const jw_str_t *path)
{
    struct stat st;
    if (stat(jw_str_cstr(path), &st) != 0) {
        return -1;
    }
    return (jw_int64_t)st.st_size;
}

JW_API jw_bool_t jw_file_exists(const jw_str_t *path)
{
    struct stat st;
    return (stat(jw_str_cstr(path), &st) == 0) ? JW_TRUE : JW_FALSE;
}

JW_API jw_bool_t jw_file_is_regular(const jw_str_t *path)
{
    struct stat st;
    if (stat(jw_str_cstr(path), &st) != 0) {
        return JW_FALSE;
    }
    return S_ISREG(st.st_mode) ? JW_TRUE : JW_FALSE;
}

JW_API jw_bool_t jw_file_is_directory(const jw_str_t *path)
{
    struct stat st;
    if (stat(jw_str_cstr(path), &st) != 0) {
        return JW_FALSE;
    }
    return S_ISDIR(st.st_mode) ? JW_TRUE : JW_FALSE;
}

/*
 * =============================================================================
 * 目录操作
 * =============================================================================
 */

JW_API jw_status_t jw_file_mkdir(const jw_str_t *path)
{
    if (mkdir(jw_str_cstr(path), 0755) == 0) {
        return JW_SUCCESS;
    }
    return JW_IO_ERROR;
}

JW_API jw_status_t jw_file_mkdir_recursive(const jw_str_t *path)
{
    const char *path_str = jw_str_cstr(path);
    char *copy = strdup(path_str);
    if (!copy) {
        return JW_OUT_OF_MEMORY;
    }
    
    char *p = copy;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            if (strlen(copy) > 0) {
                jw_str_t dir_path = jw_str(copy);
                if (!jw_file_exists(&dir_path)) {
                    if (mkdir(copy, 0755) != 0) {
                        free(copy);
                        return JW_IO_ERROR;
                    }
                }
            }
            *p = '/';
        }
        p++;
    }
    
    /* 创建最后一个目录 */
    jw_str_t final_path = jw_str(copy);
    if (!jw_file_exists(&final_path)) {
        if (mkdir(copy, 0755) != 0) {
            free(copy);
            return JW_IO_ERROR;
        }
    }
    
    free(copy);
    return JW_SUCCESS;
}

JW_API jw_status_t jw_file_rmdir(const jw_str_t *path)
{
    if (rmdir(jw_str_cstr(path)) == 0) {
        return JW_SUCCESS;
    }
    return JW_IO_ERROR;
}

JW_API jw_status_t jw_file_rmdir_recursive(const jw_str_t *path)
{
    DIR *dir = opendir(jw_str_cstr(path));
    if (!dir) {
        return JW_IO_ERROR;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        jw_str_t *full_path = jw_strprintf(NULL, "%s/%s", jw_str_cstr(path), entry->d_name);
        if (!full_path) {
            closedir(dir);
            return JW_OUT_OF_MEMORY;
        }

        if (jw_file_is_directory(full_path)) {
            if (jw_file_rmdir_recursive(full_path) != JW_SUCCESS) {
                free(full_path);
                closedir(dir);
                return JW_IO_ERROR;
            }
        } else {
            if (unlink(jw_str_cstr(full_path)) != 0) {
                free(full_path);
                closedir(dir);
                return JW_IO_ERROR;
            }
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    if (rmdir(jw_str_cstr(path)) != 0) {
        return JW_IO_ERROR;
    }
    
    return JW_SUCCESS;
}

JW_API char **jw_file_list_dir(const jw_str_t *path, jw_arena_t *arena)
{
    DIR *dir = opendir(jw_str_cstr(path));
    if (!dir) {
        return NULL;
    }
    
    /* 计算文件数量 */
    jw_size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
    }
    
    /* 重置目录指针 */
    rewinddir(dir);
    
    /* 分配文件名数组 */
    char **files = jw_arena_calloc(arena, count + 1, sizeof(char *));
    if (!files) {
        closedir(dir);
        return NULL;
    }
    
    /* 读取文件名 */
    jw_size_t idx = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        files[idx++] = strdup(entry->d_name);
    }
    
    files[idx] = NULL;
    closedir(dir);
    
    return files;
}

/*
 * =============================================================================
 * 文件路径操作
 * =============================================================================
 */

JW_API const char *jw_file_basename(const jw_str_t *path)
{
    const char *path_str = jw_str_cstr(path);
    const char *last_slash = strrchr(path_str, '/');
    if (!last_slash) {
        last_slash = strrchr(path_str, '\\');
    }
    if (last_slash) {
        return last_slash + 1;
    }
    return path_str;
}

JW_API char *jw_file_dirname(jw_arena_t *arena, const jw_str_t *path)
{
    const char *path_str = jw_str_cstr(path);
    const char *last_slash = strrchr(path_str, '/');
    if (!last_slash) {
        last_slash = strrchr(path_str, '\\');
    }
    
    if (last_slash) {
        jw_size_t len = last_slash - path_str;
        char *dir = jw_arena_calloc(arena, len + 1, sizeof(char));
        if (dir) {
            jw_memcpy(dir, path_str, len);
            dir[len] = '\0';
        }
        return dir;
    }
    
    char *result = strdup(".");
    if (result) {
        return result;
    }
    return NULL;
}

JW_API char *jw_file_join(jw_arena_t *arena, ...)
{
    va_list args;
    va_start(args, arena);
    
    jw_size_t total_len = 0;
    char *path = NULL;
    
    /* 计算总长度 */
    while ((path = va_arg(args, char *)) != NULL) {
        total_len += strlen(path) + 1; /* +1 for '/' */
    }
    
    va_end(args);
    
    /* 分配内存 */
    char *result = jw_arena_calloc(arena, total_len, sizeof(char));
    if (!result) {
        return NULL;
    }
    
    /* 拼接路径 */
    va_start(args, arena);
    char *ptr = result;
    
    while ((path = va_arg(args, char *)) != NULL) {
        if (ptr != result) {
            *ptr++ = '/';
        }
        strcpy(ptr, path);
        ptr += strlen(path);
    }
    
    va_end(args);
    
    return result;
}

JW_API char *jw_file_abs_path(jw_arena_t *arena, const jw_str_t *path)
{
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    char buffer[MAX_PATH];
    DWORD ret = GetFullPathNameA(jw_str_cstr(path), MAX_PATH, buffer, NULL);
    if (ret != 0 && ret < MAX_PATH) {
        return strdup(buffer);
    }
    return NULL;
#else
    char buffer[PATH_MAX];
    if (realpath(jw_str_cstr(path), buffer) != NULL) {
        char *result = strdup(buffer);
        if (result) {
            return result;
        }
        return NULL;
    }
    return NULL;
#endif
}

JW_API char *jw_file_normalize_path(jw_arena_t *arena, const jw_str_t *path)
{
    /* 简单的路径规范化 */
    /* TODO: 实现完整的路径规范化 */
    char *result = strdup(jw_str_cstr(path));
    if (result) {
        return result;
    }
    return NULL;
}

/*
 * =============================================================================
 * 文件系统操作
 * =============================================================================
 */

JW_API jw_status_t jw_file_unlink(const jw_str_t *path)
{
    if (unlink(jw_str_cstr(path)) == 0) {
        return JW_SUCCESS;
    }
    return JW_IO_ERROR;
}

JW_API jw_status_t jw_file_rename(const jw_str_t *old_path, const jw_str_t *new_path)
{
    if (rename(jw_str_cstr(old_path), jw_str_cstr(new_path)) == 0) {
        return JW_SUCCESS;
    }
    return JW_IO_ERROR;
}

JW_API jw_status_t jw_file_copy(const jw_str_t *src_path, const jw_str_t *dest_path)
{
    jw_str_t mode_rb = jw_str("rb");
    jw_os_handle_t src = jw_file_open(src_path, &mode_rb);
    if (src == JW_INVALID_OS_HANDLE) {
        return JW_IO_ERROR;
    }
    
    jw_str_t mode_wb = jw_str("wb");
    jw_os_handle_t dest = jw_file_open(dest_path, &mode_wb);
    if (dest == JW_INVALID_OS_HANDLE) {
        jw_file_close(src);
        return JW_IO_ERROR;
    }
    
    char buffer[4096];
    jw_ssize_t read;
    while ((read = jw_file_read(src, buffer, sizeof(buffer))) > 0) {
        if (jw_file_write(dest, buffer, (jw_size_t)read) != (jw_ssize_t)(jw_size_t)read) {
            jw_file_close(src);
            jw_file_close(dest);
            return JW_IO_ERROR;
        }
    }
    
    jw_file_close(src);
    jw_file_close(dest);
    
    return JW_SUCCESS;
}

JW_API jw_int64_t jw_file_mtime(const jw_str_t *path)
{
    struct stat st;
    if (stat(jw_str_cstr(path), &st) != 0) {
        return -1;
    }
    return (jw_int64_t)st.st_mtime;
}

/*
 * =============================================================================
 * 临时文件
 * =============================================================================
 */

JW_API char *jw_file_mktemp(char *template)
{
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    /* Use secure _mktemp_s on Windows */
    if (_mktemp_s(template, strlen(template) + 1) == 0) {
        return template;
    }
    return NULL;
#else
    {
        int fd = mkstemp(template);
        if (fd < 0) return NULL;
        close(fd);
        return template;
    }
#endif
}

JW_API char *jw_file_mkdtemp(char *template)
{
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    if (_mktemp_s(template, strlen(template) + 1) != 0) {
        return NULL;
    }
    if (_mkdir(template) == 0) {
        return template;
    }
    return NULL;
#else
    return mkdtemp(template);
#endif
}
