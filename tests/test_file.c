/*
 * test_file.c - JinWo VecDB 文件操作测试
 */

#include "jw_file.h"
#include "jw_arena.h"

static jw_uint32_t test_passed = 0;
static jw_uint32_t test_failed = 0;

#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    test_##name(); \
    test_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        test_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

static jw_arena_t* create_test_arena(void)
{
    jw_arena_t *arena = NULL;
    jw_status_t status = jw_arena_create(65536, &arena);
    if (status != JW_SUCCESS) {
        return NULL;
    }
    return arena;
}

static void cleanup_file(const char *path)
{
    jw_str_t p = {.ptr = (char*)path, .slen = (jw_size_t)strlen(path)};
    if (jw_file_exists(&p)) {
        jw_file_unlink(&p);
    }
}

TEST(file_open_close)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_file.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_file.txt", .slen = 21};
    jw_str_t mode_w = {.ptr = "wb", .slen = 2};

    jw_os_handle_t file = jw_file_open(&path, &mode_w);
    ASSERT_TRUE(file != JW_INVALID_OS_HANDLE);

    jw_file_close(file);

    cleanup_file("/tmp/jw_test_file.txt");
    jw_arena_destroy(arena);
}

TEST(file_write_read)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_read.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_read.txt", .slen = 20};
    jw_str_t mode_w = {.ptr = "wb", .slen = 2};

    jw_os_handle_t file = jw_file_open(&path, &mode_w);
    ASSERT_TRUE(file != JW_INVALID_OS_HANDLE);

    const char *data = "Hello, World!";
    jw_ssize_t written = jw_file_write(file, data, 13);
    ASSERT_EQ(written, 13);

    jw_file_close(file);

    jw_str_t mode_r = {.ptr = "rb", .slen = 2};
    file = jw_file_open(&path, &mode_r);
    ASSERT_TRUE(file != JW_INVALID_OS_HANDLE);

    char buffer[64] = {0};
    jw_ssize_t read_len = jw_file_read(file, buffer, 64);
    ASSERT_EQ(read_len, 13);
    ASSERT_TRUE(memcmp(buffer, data, 13) == 0);

    jw_file_close(file);

    cleanup_file("/tmp/jw_test_read.txt");
    jw_arena_destroy(arena);
}

TEST(file_seek)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_seek.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_seek.txt", .slen = 19};
    jw_str_t mode_w = {.ptr = "wb", .slen = 2};

    jw_os_handle_t file = jw_file_open(&path, &mode_w);
    ASSERT_TRUE(file != JW_INVALID_OS_HANDLE);

    const char *data = "0123456789";
    jw_file_write(file, data, 10);

    jw_off_t pos = jw_file_seek(file, 0, 0);
    ASSERT_EQ(pos, 0);

    pos = jw_file_seek(file, 5, 0);
    ASSERT_EQ(pos, 5);

    pos = jw_file_seek(file, -2, 1);
    ASSERT_EQ(pos, 3);

    jw_file_close(file);

    cleanup_file("/tmp/jw_test_seek.txt");
    jw_arena_destroy(arena);
}

TEST(file_exists)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t existing = {.ptr = "/tmp", .slen = 4};
    jw_str_t nonexistent = {.ptr = "/tmp/jw_nonexistent_12345.txt", .slen = 31};

    ASSERT_TRUE(jw_file_exists(&existing));
    ASSERT_FALSE(jw_file_exists(&nonexistent));

    jw_arena_destroy(arena);
}

TEST(file_size)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_size.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_size.txt", .slen = 19};
    jw_str_t mode_w = {.ptr = "wb", .slen = 2};

    jw_os_handle_t file = jw_file_open(&path, &mode_w);
    ASSERT_TRUE(file != JW_INVALID_OS_HANDLE);

    const char *data = "12345";
    jw_file_write(file, data, 5);
    jw_file_close(file);

    jw_int64_t size = jw_file_size(&path);
    ASSERT_EQ(size, 5);

    cleanup_file("/tmp/jw_test_size.txt");
    jw_arena_destroy(arena);
}

TEST(file_write_all)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_write_all.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_write_all.txt", .slen = 25};
    const char *content = "Test content for write_all";
    jw_size_t len = (jw_size_t)strlen(content);

    jw_status_t status = jw_file_write_all(&path, content, len);
    ASSERT_EQ(status, JW_SUCCESS);

    ASSERT_TRUE(jw_file_exists(&path));
    jw_int64_t size = jw_file_size(&path);
    ASSERT_EQ(size, (jw_int64_t)len);

    cleanup_file("/tmp/jw_test_write_all.txt");
    jw_arena_destroy(arena);
}

TEST(file_read_all)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    cleanup_file("/tmp/jw_test_read_all.txt");

    jw_str_t path = {.ptr = "/tmp/jw_test_read_all.txt", .slen = 23};
    const char *content = "Content for read_all test";
    jw_size_t len = (jw_size_t)strlen(content);

    jw_file_write_all(&path, content, len);

    jw_size_t size = 0;
    char *data = jw_file_read_all(&path, &size);
    ASSERT_NOT_NULL(data);
    ASSERT_EQ((jw_size_t)size, len);
    ASSERT_TRUE(memcmp(data, content, len) == 0);

    jw_free(data);
    cleanup_file("/tmp/jw_test_read_all.txt");
    jw_arena_destroy(arena);
}

TEST(file_delete)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t path = {.ptr = "/tmp/jw_test_delete.txt", .slen = 22};

    jw_file_write_all(&path, "test", 4);
    ASSERT_TRUE(jw_file_exists(&path));

    jw_status_t status = jw_file_unlink(&path);
    ASSERT_EQ(status, JW_SUCCESS);
    ASSERT_FALSE(jw_file_exists(&path));

    jw_arena_destroy(arena);
}

TEST(file_mkdir)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    jw_str_t path = {.ptr = "/tmp/jw_test_dir", .slen = 14};

    if (jw_file_exists(&path)) {
        jw_file_rmdir(&path);
    }

    jw_status_t status = jw_file_mkdir(&path);
    ASSERT_EQ(status, JW_SUCCESS);
    ASSERT_TRUE(jw_file_is_directory(&path));

    jw_file_rmdir(&path);

    jw_arena_destroy(arena);
}

TEST(file_join)
{
    jw_arena_t *arena = create_test_arena();
    ASSERT_NOT_NULL(arena);

    char *result = jw_file_join(arena, "/tmp", "subdir", "file.txt", NULL);
    ASSERT_NOT_NULL(result);

    ASSERT_TRUE(strstr(result, "/tmp") != NULL);
    ASSERT_TRUE(strstr(result, "subdir") != NULL);
    ASSERT_TRUE(strstr(result, "file.txt") != NULL);

    jw_arena_destroy(arena);
}

int main(void)
{
    RUN_TEST(file_open_close);
    RUN_TEST(file_write_read);
    RUN_TEST(file_seek);
    RUN_TEST(file_exists);
    RUN_TEST(file_size);
    RUN_TEST(file_write_all);
    RUN_TEST(file_read_all);
    RUN_TEST(file_delete);
    RUN_TEST(file_mkdir);
    RUN_TEST(file_join);

    return (int)test_failed;
}