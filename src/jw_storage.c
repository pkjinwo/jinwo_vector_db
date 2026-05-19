/*
 * jw_storage.c - JinWo VecDB 存储层实现
 * 
 * Copyright 2026 北京金幄科技有限公司
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "jw_storage.h"
#include "jw_string.h"
#include "jw_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <stdint.h>

typedef SSIZE_T ssize_t;

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif
#ifndef PROT_READ
#define PROT_READ 1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 2
#endif
#ifndef MAP_SHARED
#define MAP_SHARED 1
#endif

static void *jw_mmap_wrapper(void *addr, size_t length, int prot, int flags, int fd, jw_off_t offset)
{
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) {
        return MAP_FAILED;
    }

    DWORD protect = (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
    LARGE_INTEGER mapSize;
    mapSize.QuadPart = length;
    HANDLE hMap = CreateFileMappingA(hFile, NULL, protect, mapSize.HighPart, mapSize.LowPart, NULL);
    if (hMap == NULL) {
        return MAP_FAILED;
    }

    DWORD access = (prot & PROT_WRITE) ? FILE_MAP_WRITE : FILE_MAP_READ;
    DWORD offsetLow = (DWORD)(offset & 0xFFFFFFFF);
    DWORD offsetHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);
    void *mapped = MapViewOfFile(hMap, access, offsetHigh, offsetLow, length);
    CloseHandle(hMap);

    if (mapped == NULL) {
        return MAP_FAILED;
    }

    return mapped;
}

static int jw_munmap_wrapper(void *addr, size_t length)
{
    (void)length;
    return UnmapViewOfFile(addr) ? 0 : -1;
}

static int jw_msync_wrapper(void *addr, size_t length, int flags)
{
    (void)flags;
    return FlushViewOfFile(addr, length) ? 0 : -1;
}

static int jw_ftruncate_wrapper(int fd, jw_off_t length)
{
    return _chsize_s(fd, length);
}

static int jw_fsync_wrapper(int fd)
{
    return _commit(fd);
}

#define open _open
#define close _close
#define read _read
#define write _write
#define lseek _lseeki64
#define unlink _unlink
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir
#define mmap(a,b,c,d,e,f) jw_mmap_wrapper((a),(b),(c),(d),(e),(f))
#define munmap(a,b) jw_munmap_wrapper((a),(b))
#define msync(a,b,c) jw_msync_wrapper((a),(b),(c))
#define ftruncate(a,b) jw_ftruncate_wrapper((a),(b))
#define fsync(a) jw_fsync_wrapper((a))
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/*
 * =============================================================================
 * 内部结构定义
 * =============================================================================
 */

/* 存储结构 */
struct jw_storage {
    jw_storage_config_t config;
    jw_arena_t *arena;
    
    /* 文件存储 */
    int fd;                     /* 文件描述符 */
    char *path;                 /* 文件路径 */
    
    /* 文件头 */
    jw_storage_header_t header;
    
    /* 内存存储 */
    jw_uint8_t *data;           /* 内存数据 */
    jw_size_t data_size;        /* 数据大小 */
    jw_size_t data_capacity;    /* 数据容量 */
    
    /* 统计 */
    jw_storage_stats_t stats;
    
    /* 锁 */
    jw_lock_t *lock;
};

/*
 * =============================================================================
 * 辅助函数
 * =============================================================================
 */

/* 检查文件是否存在 */
static jw_bool_t file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

/* 获取文件大小 */
static jw_size_t file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return (jw_size_t)st.st_size;
    }
    return 0;
}

/* 初始化文件头 */
static void init_header(jw_storage_header_t *header)
{
    jw_memset(header, 0, sizeof(jw_storage_header_t));

    header->magic = JW_STORAGE_MAGIC;
    header->version = JW_STORAGE_VERSION;
    header->create_time = jw_time_now();
    header->update_time = header->create_time;
    header->data_offset = sizeof(jw_storage_header_t);
    header->data_size = 0;
    header->index_offset = 0;
    header->index_size = 0;
}

/* 验证文件头 */
static jw_status_t validate_header(const jw_storage_header_t *header)
{
    if (header->magic != JW_STORAGE_MAGIC) {
        return JW_FILE_CORRUPTED;
    }
    
    if ((header->version >> 16) != (JW_STORAGE_VERSION >> 16)) {
        return JW_NOT_SUPPORTED;
    }
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 存储生命周期
 * =============================================================================
 */

JW_API jw_storage_t *jw_storage_create(jw_arena_t *arena,
                                        const jw_storage_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }
    
    jw_arena_t *local_arena = arena;
    if (!local_arena) {
        if (jw_arena_create(4096 * 1024, &local_arena) != JW_SUCCESS) {
            return NULL;
        }
    }
    
    jw_storage_t *storage = jw_arena_calloc(local_arena, 1, sizeof(jw_storage_t));
    if (storage == NULL) {
        if (!arena) jw_arena_destroy(local_arena);
        return NULL;
    }
    
    storage->arena = local_arena;
    storage->config = *config;
    storage->fd = -1;
    
    /* 初始化文件头 */
    init_header(&storage->header);
    
    /* 初始化锁 */
    jw_mutex_t *lock;
    jw_mutex_create(local_arena, NULL, &lock);
    storage->lock = (jw_lock_t *)lock;
    
    if (config->type == JW_STORAGE_TYPE_MEMORY) {
        /* 内存存储 */
        storage->data_capacity = config->cache_size > 0 ? config->cache_size : 16 * 1024 * 1024;
        storage->data = jw_arena_alloc(local_arena, storage->data_capacity);
        
        if (storage->data == NULL) {
            jw_mutex_destroy((jw_mutex_t *)storage->lock);
            if (!arena) jw_arena_destroy(local_arena);
            return NULL;
        }
        
        storage->data_size = 0;
    } 
    else if (config->type == JW_STORAGE_TYPE_FILE || config->type == JW_STORAGE_TYPE_MMAP) {
        /* 文件存储 */
        if (config->path.ptr == NULL) {
            jw_mutex_destroy((jw_mutex_t *)storage->lock);
            if (!arena) jw_arena_destroy(local_arena);
            return NULL;
        }
        
        storage->path = jw_arena_strdup(local_arena, config->path.ptr);
        
        /* 打开或创建文件 */
        int flags = 0;
        mode_t mode = 0644;
        
        switch (config->mode) {
            case JW_STORAGE_READ:
                flags = O_RDONLY;
                break;
            case JW_STORAGE_WRITE:
                flags = O_WRONLY;
                break;
            case JW_STORAGE_READWRITE:
                flags = O_RDWR;
                break;
            case JW_STORAGE_CREATE:
                flags = O_RDWR | O_CREAT;
                break;
            case JW_STORAGE_TRUNCATE:
                flags = O_RDWR | O_CREAT | O_TRUNC;
                break;
            case JW_STORAGE_APPEND:
                flags = O_WRONLY | O_APPEND | O_CREAT;
                break;
        }
        
        storage->fd = open(config->path.ptr, flags, mode);
        if (storage->fd < 0) {
            jw_mutex_destroy((jw_mutex_t *)storage->lock);
            if (!arena) jw_arena_destroy(local_arena);
            return NULL;
        }
        
        /* 如果是新文件，写入头部 */
        if (config->mode == JW_STORAGE_CREATE || config->mode == JW_STORAGE_TRUNCATE) {
            write(storage->fd, &storage->header, sizeof(jw_storage_header_t));
        } else {
            /* 读取现有头部 */
            ssize_t bytes = read(storage->fd, &storage->header, sizeof(jw_storage_header_t));
            if (bytes != sizeof(jw_storage_header_t)) {
                close(storage->fd);
                jw_mutex_destroy((jw_mutex_t *)storage->lock);
                if (!arena) jw_arena_destroy(local_arena);
                return NULL;
            }
            
            jw_status_t status = validate_header(&storage->header);
            if (status != JW_SUCCESS) {
                close(storage->fd);
                jw_mutex_destroy((jw_mutex_t *)storage->lock);
                if (!arena) jw_arena_destroy(local_arena);
                return NULL;
            }
        }
        
        /* MMAP存储 */
        if (config->type == JW_STORAGE_TYPE_MMAP) {
            jw_size_t fsize = file_size(config->path.ptr);
            if (fsize < sizeof(jw_storage_header_t)) {
                fsize = sizeof(jw_storage_header_t);
                if (ftruncate(storage->fd, fsize) < 0) {
                    close(storage->fd);
                    jw_mutex_destroy((jw_mutex_t *)storage->lock);
                    if (!arena) jw_arena_destroy(local_arena);
                    return NULL;
                }
            }
            
            storage->data = mmap(NULL, fsize, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, storage->fd, 0);
            if (storage->data == MAP_FAILED) {
                storage->data = NULL;
            } else {
                storage->data_size = storage->header.data_size + sizeof(jw_storage_header_t);
                storage->data_capacity = fsize;
            }
        }
    }
    
    /* 初始化统计 */
    jw_memset(&storage->stats, 0, sizeof(jw_storage_stats_t));

    return storage;
}

JW_API jw_storage_t *jw_storage_open(jw_arena_t *arena,
                                      const char *path,
                                      jw_storage_open_mode_t mode)
{
    jw_storage_config_t config = JW_STORAGE_CONFIG_DEFAULT;
    config.path = (jw_str_t){.ptr = (char*)path, .slen = path ? strlen(path) : 0};
    config.mode = mode;
    
    return jw_storage_create(arena, &config);
}

JW_API jw_status_t jw_storage_close(jw_storage_t *storage)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_mutex_lock((jw_mutex_t *)storage->lock);
    
    /* 同步数据 */
    if (storage->config.type == JW_STORAGE_TYPE_FILE && storage->fd >= 0) {
        if (storage->config.sync_on_write) {
            fsync(storage->fd);
        }
        
        /* 更新头部 */
        storage->header.update_time = jw_time_now();
        lseek(storage->fd, 0, SEEK_SET);
        write(storage->fd, &storage->header, sizeof(jw_storage_header_t));
        
        close(storage->fd);
        storage->fd = -1;
    }
    
    /* MMAP解映射 */
    if (storage->config.type == JW_STORAGE_TYPE_MMAP && storage->data != NULL) {
        munmap(storage->data, storage->data_capacity);
        storage->data = NULL;
    }
    
    jw_mutex_unlock((jw_mutex_t *)storage->lock);
    jw_mutex_destroy((jw_mutex_t *)storage->lock);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_destroy(jw_storage_t *storage)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_status_t status = jw_storage_close(storage);
    if (status != JW_SUCCESS) {
        return status;
    }
    
    /* 删除文件 */
    if (storage->path != NULL && storage->config.type != JW_STORAGE_TYPE_MEMORY) {
        unlink(storage->path);
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_sync(jw_storage_t *storage)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    if (storage->config.type == JW_STORAGE_TYPE_MMAP && storage->data != NULL) {
        /* MMAP同步 */
        if (msync(storage->data, storage->data_size, MS_SYNC) < 0) {
            return JW_UNKNOWN_ERROR;
        }
    } else if (storage->fd >= 0) {
        /* 文件同步 */
        if (fsync(storage->fd) != 0) {
            return JW_UNKNOWN_ERROR;
        }
    }
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 读写操作
 * =============================================================================
 */

JW_API jw_status_t jw_storage_write(jw_storage_t *storage,
                                     const void *data,
                                     jw_size_t size,
                                     jw_uint64_t *offset)
{
    if (storage == NULL || data == NULL || size == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_mutex_lock((jw_mutex_t *)storage->lock);
    
    jw_status_t status = JW_SUCCESS;
    jw_uint64_t write_offset = 0;
    
    if (storage->config.type == JW_STORAGE_TYPE_MEMORY) {
        /* 内存存储 */
        if (storage->data_size + size > storage->data_capacity) {
            /* 扩容 */
            jw_size_t new_capacity = storage->data_capacity * 2;
            while (new_capacity < storage->data_size + size) {
                new_capacity *= 2;
            }
            
            jw_uint8_t *new_data = jw_arena_alloc(storage->arena, new_capacity);
            if (new_data == NULL) {
                jw_mutex_unlock((jw_mutex_t *)storage->lock);
                return JW_OUT_OF_MEMORY;
            }
            
            jw_memcpy(new_data, storage->data, storage->data_size);
            storage->data = new_data;
            storage->data_capacity = new_capacity;
        }

        write_offset = storage->data_size;
        jw_memcpy(storage->data + storage->data_size, data, size);
        storage->data_size += size;
        
    } else if (storage->config.type == JW_STORAGE_TYPE_MMAP && storage->data != NULL) {
        /* MMAP存储 */
        if (storage->data_size + size > storage->data_capacity) {
            /* 扩展文件大小 */
            jw_size_t new_size = storage->data_size + size;
            jw_size_t new_capacity = storage->data_capacity * 2;
            while (new_capacity < new_size) {
                new_capacity *= 2;
            }
            
            /* 解除现有映射 */
            if (munmap(storage->data, storage->data_capacity) < 0) {
                status = JW_IO_ERROR;
                goto cleanup;
            }
            storage->data = NULL;
            
            /* 扩展文件大小 */
            if (ftruncate(storage->fd, new_capacity) < 0) {
                status = JW_IO_ERROR;
                goto cleanup;
            }
            
            /* 重新映射 */
            storage->data = mmap(NULL, new_capacity, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, storage->fd, 0);
            if (storage->data == MAP_FAILED) {
                storage->data = NULL;
                status = JW_IO_ERROR;
                goto cleanup;
            }
            
            storage->data_capacity = new_capacity;
        }
        
        write_offset = storage->data_size;
        jw_memcpy(storage->data + storage->data_size, data, size);
        storage->data_size += size;
        
        /* 同步到磁盘 */
        if (storage->config.sync_on_write) {
            /* 同步整个映射区域，确保地址是页面对齐的 */
            if (msync(storage->data, storage->data_capacity, MS_SYNC) < 0) {
                status = JW_UNKNOWN_ERROR;
            }
        }
        
    } else if (storage->fd >= 0) {
        /* 文件存储 */
        if (offset != NULL) {
            /* 使用指定的offset */
            write_offset = lseek(storage->fd, *offset, SEEK_SET);
        } else {
            /* 从文件末尾写入 */
            write_offset = lseek(storage->fd, 0, SEEK_END);
        }
        
        if (write_offset < 0) {
            status = JW_IO_ERROR;
            goto cleanup;
        }
        
        ssize_t written = write(storage->fd, data, size);
        if (written < 0 || (jw_size_t)written != size) {
            status = JW_IO_ERROR;
        }
        
        if (storage->config.sync_on_write) {
            fsync(storage->fd);
        }
    }
    
    if (status == JW_SUCCESS) {
        storage->header.data_size += size;
        storage->stats.used_size += size;
        
        if (offset != NULL) {
            *offset = write_offset;
        }
    }

cleanup:
    jw_mutex_unlock((jw_mutex_t *)storage->lock);
    
    return status;
}

JW_API jw_ssize_t jw_storage_append(jw_storage_t *storage,
                                     const void *data,
                                     jw_size_t size)
{
    jw_uint64_t offset;
    jw_status_t status = jw_storage_write(storage, data, size, &offset);
    
    if (status != JW_SUCCESS) {
        return -1;
    }
    
    return (jw_ssize_t)offset;
}

JW_API jw_ssize_t jw_storage_read(jw_storage_t *storage,
                                   jw_uint64_t offset,
                                   void *buffer,
                                   jw_size_t size)
{
    if (storage == NULL || buffer == NULL || size == 0) {
        return -1;
    }
    
    jw_mutex_lock((jw_mutex_t *)storage->lock);
    
    ssize_t bytes_read = -1;
    
    if (storage->config.type == JW_STORAGE_TYPE_MEMORY) {
        /* 内存存储 */
        if (offset + size > storage->data_size) {
            jw_mutex_unlock((jw_mutex_t *)storage->lock);
            return -1;
        }
        
        jw_memcpy(buffer, storage->data + offset, size);
        bytes_read = size;

    } else if (storage->config.type == JW_STORAGE_TYPE_MMAP && storage->data != NULL) {
        /* MMAP存储 */
        if (offset + size > storage->data_size) {
            jw_mutex_unlock((jw_mutex_t *)storage->lock);
            return -1;
        }

        jw_memcpy(buffer, storage->data + offset, size);
        bytes_read = size;
        
    } else if (storage->fd >= 0) {
        /* 文件存储 */
        lseek(storage->fd, offset, SEEK_SET);
        bytes_read = read(storage->fd, buffer, size);
    }
    
    if (bytes_read > 0) {
    }
    
    jw_mutex_unlock((jw_mutex_t *)storage->lock);
    
    return bytes_read;
}

JW_API jw_status_t jw_storage_write_vector(jw_storage_t *storage,
                                            jw_uint64_t vid,
                                            jw_cvec_t vec,
                                            jw_dim_t dim,
                                            jw_uint64_t *offset)
{
    if (storage == NULL || vec == NULL || dim == 0) {
        return JW_INVALID_PARAM;
    }
    
    /* 写入格式: [vid(8字节)][dim(4字节)][向量数据] */
    jw_size_t total_size = sizeof(jw_uint64_t) + sizeof(jw_uint32_t) + dim * sizeof(jw_float32_t);
    jw_uint8_t *buffer = jw_arena_alloc(storage->arena, total_size);
    
    if (buffer == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    jw_uint8_t *ptr = buffer;

    /* 写入vid */
    jw_memcpy(ptr, &vid, sizeof(jw_uint64_t));
    ptr += sizeof(jw_uint64_t);

    /* 写入dim */
    jw_uint32_t dim32 = dim;
    jw_memcpy(ptr, &dim32, sizeof(jw_uint32_t));
    ptr += sizeof(jw_uint32_t);

    /* 写入向量 */
    jw_memcpy(ptr, vec, dim * sizeof(jw_float32_t));
    
    jw_status_t status = jw_storage_write(storage, buffer, total_size, offset);
    
    return status;
}

JW_API jw_status_t jw_storage_read_vector(jw_storage_t *storage,
                                           jw_uint64_t offset,
                                           jw_uint64_t *vid,
                                           jw_vec_t vec,
                                           jw_dim_t dim)
{
    if (storage == NULL || vid == NULL || vec == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* 读取头部 */
    jw_uint8_t header[sizeof(jw_uint64_t) + sizeof(jw_uint32_t)];
    
    jw_ssize_t bytes = jw_storage_read(storage, offset, header, sizeof(header));
    if (bytes != sizeof(header)) {
        return JW_UNKNOWN_ERROR;
    }
    
    /* 解析vid和dim */
    jw_memcpy(vid, header, sizeof(jw_uint64_t));

    jw_uint32_t stored_dim;
    jw_memcpy(&stored_dim, header + sizeof(jw_uint64_t), sizeof(jw_uint32_t));
    
    if (stored_dim != dim) {
        return JW_INVALID_PARAM;
    }
    
    /* 读取向量 */
    bytes = jw_storage_read(storage, offset + sizeof(header), vec, dim * sizeof(jw_float32_t));
    if (bytes != (jw_ssize_t)(dim * sizeof(jw_float32_t))) {
        return JW_UNKNOWN_ERROR;
    }
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 块操作
 * =============================================================================
 */

JW_API jw_ssize_t jw_storage_alloc_block(jw_storage_t *storage, jw_size_t size)
{
    /* 简化实现：直接在末尾分配 */
    jw_uint64_t offset;
    
    /* 预留块头空间 */
    jw_size_t total = sizeof(jw_data_block_t) + size;
    jw_uint8_t *buffer = jw_arena_calloc(storage->arena, 1, total);
    
    if (buffer == NULL) {
        return -1;
    }
    
    jw_data_block_t *block = (jw_data_block_t *)buffer;
    block->block_id = storage->stats.block_count;
    block->size = size;
    block->flags = 0;
    block->compression = JW_COMPRESS_NONE;
    
    jw_status_t status = jw_storage_write(storage, buffer, total, &offset);
    
    if (status != JW_SUCCESS) {
        return -1;
    }
    
    storage->stats.block_count++;
    
    return (jw_ssize_t)offset;
}

JW_API jw_status_t jw_storage_free_block(jw_storage_t *storage,
                                          jw_uint64_t block_id)
{
    /* 简化实现：标记为已释放 */
    (void)storage;
    (void)block_id;
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_get_block_info(jw_storage_t *storage,
                                              jw_uint64_t block_id,
                                              jw_data_block_t *info)
{
    if (storage == NULL || info == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* 从文件读取块头 */
    jw_ssize_t bytes = jw_storage_read(storage, block_id, info, sizeof(jw_data_block_t));
    
    if (bytes != sizeof(jw_data_block_t)) {
        return JW_UNKNOWN_ERROR;
    }
    
    return JW_SUCCESS;
}

/*
 * =============================================================================
 * 迭代器
 * =============================================================================
 */

JW_API jw_storage_iterator_t *jw_storage_iterator_create(jw_storage_t *storage)
{
    if (storage == NULL) {
        return NULL;
    }
    
    jw_storage_iterator_t *iter = jw_arena_alloc(storage->arena, sizeof(jw_storage_iterator_t));
    
    if (iter == NULL) {
        return NULL;
    }
    
    iter->storage = storage;
    iter->current_offset = storage->header.data_offset;
    iter->end_offset = storage->header.data_offset + storage->header.data_size;
    jw_memset(&iter->current_block, 0, sizeof(jw_data_block_t));

    return iter;
}

JW_API void jw_storage_iterator_destroy(jw_storage_iterator_t *iter)
{
    /* 内存池管理，无需释放 */
    (void)iter;
}

JW_API jw_bool_t jw_storage_iterator_next(jw_storage_iterator_t *iter)
{
    if (iter == NULL || iter->storage == NULL) {
        return JW_FALSE;
    }
    
    /* 跳过当前块 */
    if (iter->current_block.size > 0) {
        iter->current_offset += sizeof(jw_data_block_t) + iter->current_block.size;
    }
    
    if (iter->current_offset >= iter->end_offset) {
        return JW_FALSE;
    }
    
    /* 读取下一个块头 */
    jw_ssize_t bytes = jw_storage_read(iter->storage, iter->current_offset,
                                        &iter->current_block, sizeof(jw_data_block_t));
    
    return (bytes == sizeof(jw_data_block_t));
}

JW_API jw_size_t jw_storage_iterator_get_data(jw_storage_iterator_t *iter,
                                               void *buffer,
                                               jw_size_t size)
{
    if (iter == NULL || buffer == NULL || size == 0) {
        return 0;
    }
    
    jw_size_t read_size = (iter->current_block.size < size) 
                         ? iter->current_block.size : size;
    
    jw_ssize_t bytes = jw_storage_read(iter->storage,
                                        iter->current_offset + sizeof(jw_data_block_t),
                                        buffer, read_size);
    
    return (bytes > 0) ? (jw_size_t)bytes : 0;
}

/*
 * =============================================================================
 * 统计与维护
 * =============================================================================
 */

JW_API jw_status_t jw_storage_get_stats(const jw_storage_t *storage,
                                         jw_storage_stats_t *stats)
{
    if (storage == NULL || stats == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_mutex_lock((jw_mutex_t *)&storage->lock);
    *stats = storage->stats;
    
    if (storage->config.type == JW_STORAGE_TYPE_MEMORY) {
        stats->total_size = storage->data_capacity;
        stats->used_size = storage->data_size;
        stats->free_size = storage->data_capacity - storage->data_size;
    }
    
    jw_mutex_unlock((jw_mutex_t *)&storage->lock);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_compact(jw_storage_t *storage)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* TODO: 实现压缩整理 */
    (void)storage;
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_verify(jw_storage_t *storage)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_mutex_lock((jw_mutex_t *)storage->lock);
    
    jw_status_t status = validate_header(&storage->header);
    
    jw_mutex_unlock((jw_mutex_t *)storage->lock);
    
    return status;
}

/*
 * =============================================================================
 * 自定义存储后端
 * =============================================================================
 */

JW_API jw_status_t jw_storage_register_backend(const char *name,
                                                const jw_storage_ops_t *ops)
{
    /* TODO: 实现自定义后端注册 */
    (void)name;
    (void)ops;
    return JW_NOT_SUPPORTED;
}
/*
 * =============================================================================
 * 字节序安全读写函数
 * =============================================================================
 */

JW_API jw_status_t jw_storage_write_u32(jw_storage_t *storage,
                                         jw_uint32_t value,
                                         jw_uint64_t *offset)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* 转换为小端序 */
    jw_uint32_t le_value = jw_htole32(value);
    
    return jw_storage_write(storage, &le_value, sizeof(le_value), offset);
}

JW_API jw_status_t jw_storage_read_u32(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_uint32_t *value)
{
    if (storage == NULL || value == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint32_t le_value;
    jw_ssize_t bytes = jw_storage_read(storage, offset, &le_value, sizeof(le_value));
    
    if (bytes != sizeof(le_value)) {
        return JW_UNKNOWN_ERROR;
    }
    
    /* 从小端序转换 */
    *value = jw_letoh32(le_value);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_write_u64(jw_storage_t *storage,
                                         jw_uint64_t value,
                                         jw_uint64_t *offset)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t le_value = jw_htole64(value);
    
    return jw_storage_write(storage, &le_value, sizeof(le_value), offset);
}

JW_API jw_status_t jw_storage_read_u64(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_uint64_t *value)
{
    if (storage == NULL || value == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t le_value;
    jw_ssize_t bytes = jw_storage_read(storage, offset, &le_value, sizeof(le_value));
    
    if (bytes != sizeof(le_value)) {
        return JW_UNKNOWN_ERROR;
    }
    
    *value = jw_letoh64(le_value);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_write_f32(jw_storage_t *storage,
                                         jw_float32_t value,
                                         jw_uint64_t *offset)
{
    if (storage == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_float32_t le_value = jw_htolef32(value);
    
    return jw_storage_write(storage, &le_value, sizeof(le_value), offset);
}

JW_API jw_status_t jw_storage_read_f32(jw_storage_t *storage,
                                        jw_uint64_t offset,
                                        jw_float32_t *value)
{
    if (storage == NULL || value == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_float32_t le_value;
    jw_ssize_t bytes = jw_storage_read(storage, offset, &le_value, sizeof(le_value));
    
    if (bytes != sizeof(le_value)) {
        return JW_UNKNOWN_ERROR;
    }
    
    *value = jw_letohf32(le_value);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_write_f32_array(jw_storage_t *storage,
                                               const jw_float32_t *values,
                                               jw_size_t count,
                                               jw_uint64_t *offset)
{
    if (storage == NULL || values == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size = count * sizeof(jw_float32_t);
    jw_float32_t *buffer = jw_arena_alloc(storage->arena, size);
    
    if (buffer == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    /* 转换为小端序 */
    for (jw_size_t i = 0; i < count; i++) {
        buffer[i] = jw_htolef32(values[i]);
    }
    
    jw_status_t status = jw_storage_write(storage, buffer, size, offset);
    
    return status;
}

JW_API jw_status_t jw_storage_read_f32_array(jw_storage_t *storage,
                                              jw_uint64_t offset,
                                              jw_float32_t *values,
                                              jw_size_t count)
{
    if (storage == NULL || values == NULL || count == 0) {
        return JW_INVALID_PARAM;
    }
    
    jw_size_t size = count * sizeof(jw_float32_t);
    jw_float32_t *buffer = jw_arena_alloc(storage->arena, size);
    
    if (buffer == NULL) {
        return JW_OUT_OF_MEMORY;
    }
    
    jw_ssize_t bytes = jw_storage_read(storage, offset, buffer, size);
    
    if (bytes != (jw_ssize_t)size) {
        return JW_UNKNOWN_ERROR;
    }
    
    /* 从小端序转换 */
    for (jw_size_t i = 0; i < count; i++) {
        values[i] = jw_letohf32(buffer[i]);
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_write_header(jw_storage_t *storage,
                                            const jw_storage_header_fixed_t *header)
{
    if (storage == NULL || header == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t offset = 0;
    
    /* 写入魔数和版本 (始终用小端) */
    JW_CHECK(jw_storage_write_u32(storage, header->magic, &offset));
    JW_CHECK(jw_storage_write_u32(storage, header->version, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->create_time, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->update_time, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->data_offset, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->data_size, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->index_offset, &offset));
    JW_CHECK(jw_storage_write_u64(storage, header->index_size, &offset));
    JW_CHECK(jw_storage_write_u32(storage, header->flags, &offset));
    JW_CHECK(jw_storage_write_u32(storage, header->checksum, &offset));
    
    /* 写入保留空间 */
    if (sizeof(header->reserved) > 0) {
        JW_CHECK(jw_storage_write(storage, header->reserved,
                                    sizeof(header->reserved), &offset));
    }
    
    /* 设置字节序标志 */
    jw_storage_write_u32(storage, JW_STORAGE_NATIVE_ENDIAN, &offset);
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_read_header(jw_storage_t *storage,
                                           jw_storage_header_fixed_t *header)
{
    if (storage == NULL || header == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t offset = 0;
    
    /* 读取固定结构体 */
    jw_ssize_t bytes = jw_storage_read(storage, offset, (void*)header,
                                        sizeof(jw_storage_header_fixed_t));
    
    if (bytes != sizeof(jw_storage_header_fixed_t)) {
        return JW_UNKNOWN_ERROR;
    }
    
    /* 验证魔数 */
    if (header->magic != JW_STORAGE_MAGIC) {
        return JW_FILE_CORRUPTED;
    }
    
    /* 检查字节序是否需要转换 */
    if ((header->flags & 0x03) != JW_STORAGE_NATIVE_ENDIAN) {
        /* 需要字节序转换 */
        header->version = jw_letoh32(header->version);
        header->create_time = jw_letoh64(header->create_time);
        header->update_time = jw_letoh64(header->update_time);
        header->data_offset = jw_letoh64(header->data_offset);
        header->data_size = jw_letoh64(header->data_size);
        header->index_offset = jw_letoh64(header->index_offset);
        header->index_size = jw_letoh64(header->index_size);
        header->flags = jw_letoh32(header->flags);
        header->checksum = jw_letoh32(header->checksum);
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_write_collection_header(
    jw_storage_t *storage,
    const jw_collection_header_fixed_t *header,
    jw_uint64_t *offset)
{
    if (storage == NULL || header == NULL) {
        return JW_INVALID_PARAM;
    }
    
    jw_uint64_t write_offset = (offset != NULL) ? *offset : 0;
    
    /* 写入固定结构体 */
    JW_CHECK(jw_storage_write_u32(storage, header->magic, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->version, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->create_time, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->update_time, &write_offset));
    
    /* 写入名称 */
    JW_CHECK(jw_storage_write(storage, header->name,
                                sizeof(header->name), &write_offset));
    
    /* 写入整数 */
    JW_CHECK(jw_storage_write_u32(storage, header->dim, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->metric, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->index_type, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->vector_count, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->next_vid, &write_offset));
    
    /* 索引配置 */
    JW_CHECK(jw_storage_write_u32(storage, header->hnsw_m, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->hnsw_ef_construction, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->hnsw_ef_search, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->ivf_nlist, &write_offset));
    JW_CHECK(jw_storage_write_u32(storage, header->ivf_nprobe, &write_offset));
    
    /* 数据位置 */
    JW_CHECK(jw_storage_write_u64(storage, header->vectors_offset, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->vectors_size, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->index_offset, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->index_size, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->meta_offset, &write_offset));
    JW_CHECK(jw_storage_write_u64(storage, header->meta_size, &write_offset));
    
    /* 校验和 */
    JW_CHECK(jw_storage_write_u32(storage, header->checksum, &write_offset));
    
    /* 保留空间 */
    JW_CHECK(jw_storage_write(storage, header->reserved,
                                sizeof(header->reserved), &write_offset));
    
    if (offset != NULL) {
        *offset = write_offset;
    }
    
    return JW_SUCCESS;
}

JW_API jw_status_t jw_storage_read_collection_header(
    jw_storage_t *storage,
    jw_uint64_t offset,
    jw_collection_header_fixed_t *header)
{
    if (storage == NULL || header == NULL) {
        return JW_INVALID_PARAM;
    }
    
    /* 读取固定结构体 */
    jw_ssize_t bytes = jw_storage_read(storage, offset, (void*)header,
                                        sizeof(jw_collection_header_fixed_t));
    
    if (bytes != sizeof(jw_collection_header_fixed_t)) {
        return JW_UNKNOWN_ERROR;
    }
    
    /* 转换字节序 */
    header->version = jw_letoh32(header->version);
    header->create_time = jw_letoh64(header->create_time);
    header->update_time = jw_letoh64(header->update_time);
    header->dim = jw_letoh32(header->dim);
    header->metric = jw_letoh32(header->metric);
    header->index_type = jw_letoh32(header->index_type);
    header->vector_count = jw_letoh32(header->vector_count);
    header->next_vid = jw_letoh64(header->next_vid);
    header->hnsw_m = jw_letoh32(header->hnsw_m);
    header->hnsw_ef_construction = jw_letoh32(header->hnsw_ef_construction);
    header->hnsw_ef_search = jw_letoh32(header->hnsw_ef_search);
    header->ivf_nlist = jw_letoh32(header->ivf_nlist);
    header->ivf_nprobe = jw_letoh32(header->ivf_nprobe);
    header->vectors_offset = jw_letoh64(header->vectors_offset);
    header->vectors_size = jw_letoh64(header->vectors_size);
    header->index_offset = jw_letoh64(header->index_offset);
    header->index_size = jw_letoh64(header->index_size);
    header->meta_offset = jw_letoh64(header->meta_offset);
    header->meta_size = jw_letoh64(header->meta_size);
    header->checksum = jw_letoh32(header->checksum);
    
    return JW_SUCCESS;
}
