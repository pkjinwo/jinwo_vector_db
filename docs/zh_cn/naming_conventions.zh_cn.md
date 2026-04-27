# JinWo VecDB 命名规范指南

**版本**: v1.0.0
**生成时间**: 2026-04-26
**文档类型**: 对外发布

---

## 一、命名规范概述

### 1.1 规范目标

命名规范的主要目标是确保 JinWo VecDB 代码库的一致性、可读性和可维护性。良好的命名规范可以：

- **提高代码可读性**: 清晰的命名使代码更容易理解
- **减少认知负担**: 一致的命名模式减少开发者的认知负担
- **促进团队协作**: 统一的命名规范便于团队成员之间的协作
- **降低维护成本**: 清晰的命名使代码更易于维护和扩展
- **减少错误**: 明确的命名减少因误解而导致的错误

### 1.2 适用范围

本规范适用于 JinWo VecDB 项目的所有代码文件，包括：

- C/C++ 源代码文件
- 头文件
- 测试文件
- 文档文件
- 配置文件
- 脚本文件

### 1.3 命名原则

| 原则 | 描述 | 示例 |
|------|------|------|
| 清晰性 | 命名应清晰表达其用途和含义 | `vector_search` 而不是 `vs` |
| 一致性 | 相同类型的元素应使用相同的命名模式 | 所有函数使用 `snake_case` |
| 简洁性 | 命名应简洁但不牺牲清晰度 | `db` 而不是 `database` (在上下文明确时) |
| 准确性 | 命名应准确反映其功能 | `insert_vector` 而不是 `add_data` |
| 避免歧义 | 命名应避免歧义 | `size` 可能指大小或尺寸，应使用更具体的名称 |
| 避免缩写 | 除非是广泛认可的缩写，否则应使用完整单词 | `http` 是可接受的缩写，`vec` 也是可接受的 |

---

## 二、文件命名规范

### 2.1 源代码文件

| 文件类型 | 命名规则 | 示例 |
|----------|----------|------|
| C 源文件 | 小写字母，使用下划线分隔单词 | `jw_vecdb.c` |
| C++ 源文件 | 小写字母，使用下划线分隔单词 | `jw_index.cpp` |
| 头文件 | 小写字母，使用下划线分隔单词，`.h` 扩展名 | `jw_vecdb.h` |
| 模板文件 | 小写字母，使用下划线分隔单词，`.hpp` 扩展名 | `jw_utils.hpp` |

### 2.2 测试文件

| 文件类型 | 命名规则 | 示例 |
|----------|----------|------|
| 测试源文件 | `test_` 前缀 + 测试模块名 | `test_vector.c` |
| 测试头文件 | `test_` 前缀 + 测试模块名 + `.h` | `test_utils.h` |
| 测试数据文件 | `testdata_` 前缀 + 数据描述 | `testdata_large_vector.bin` |

### 2.3 文档文件

| 文件类型 | 命名规则 | 示例 |
|----------|----------|------|
| Markdown 文档 | 小写字母，使用下划线分隔单词，`.md` 扩展名 | `api_reference.md` |
| 配置文档 | 小写字母，使用下划线分隔单词，`.md` 扩展名 | `configuration_guide.md` |
| 示例文档 | `example_` 前缀 + 示例名称 | `example_basic_usage.md` |

### 2.4 配置文件

| 文件类型 | 命名规则 | 示例 |
|----------|----------|------|
| CMake 配置文件 | `CMakeLists.txt` (固定名称) | `CMakeLists.txt` |
| 构建配置文件 | 小写字母，使用下划线分隔单词 | `build_config.cmake` |
| 环境配置文件 | `.env` 或 `env_` 前缀 | `.env` 或 `env_development` |

---

## 三、函数命名规范

### 3.1 C 函数

| 函数类型 | 命名规则 | 示例 |
|----------|----------|------|
| 公共 API 函数 | `jw_` 前缀 + 模块名 + `_` + 功能描述 | `jw_vecdb_open` |
| 内部函数 | 模块名 + `_` + 功能描述 | `vecdb_internal_init` |
| 工具函数 | `jw_` 前缀 + 工具名 + `_` + 功能描述 | `jw_utils_malloc` |
| 回调函数 | `on_` 前缀 + 事件名称 | `on_vector_inserted` |
| 测试函数 | `test_` 前缀 + 测试名称 | `test_vector_search` |

### 3.2 C++ 函数

| 函数类型 | 命名规则 | 示例 |
|----------|----------|------|
| 成员函数 | `snake_case` | `insert_vector` |
| 静态成员函数 | `snake_case` | `create_instance` |
| 构造函数 | 类名 | `VecDB()` |
| 析构函数 | `~` + 类名 | `~VecDB()` |
| 运算符重载 | 运算符符号 | `operator+` |
| 私有辅助函数 | `_` 前缀 + `snake_case` | `_internal_init` |

### 3.3 函数参数

| 参数类型 | 命名规则 | 示例 |
|----------|----------|------|
| 输入参数 | `snake_case` | `vector_data` |
| 输出参数 | `out_` 前缀 + `snake_case` | `out_vector_id` |
| 输入输出参数 | `io_` 前缀 + `snake_case` | `io_buffer` |
| 布尔参数 | `is_` 或 `has_` 前缀 | `is_create` |
| 大小参数 | `size` 或 `count` 后缀 | `vector_size` |
| 指针参数 | 正常命名，避免 `p_` 前缀 | `vector` 而不是 `p_vector` |

---

## 四、变量命名规范

### 4.1 局部变量

| 变量类型 | 命名规则 | 示例 |
|----------|----------|------|
| 普通变量 | `snake_case` | `vector_id` |
| 布尔变量 | `is_` 或 `has_` 前缀 | `is_open` |
| 计数器变量 | 单字母或 `snake_case` | `i` 或 `counter` |
| 临时变量 | `temp_` 前缀 | `temp_buffer` |
| 循环变量 | 单字母或 `snake_case` | `i`, `j` 或 `loop_counter` |

### 4.2 全局变量

| 变量类型 | 命名规则 | 示例 |
|----------|----------|------|
| 全局变量 | `g_` 前缀 + `snake_case` | `g_db_instance` |
| 静态全局变量 | `s_` 前缀 + `snake_case` | `s_initialized` |
| 常量全局变量 | 全大写，下划线分隔 | `MAX_VECTOR_SIZE` |

### 4.3 成员变量

| 变量类型 | 命名规则 | 示例 |
|----------|----------|------|
| 公共成员变量 | `snake_case` | `vector_count` |
| 私有成员变量 | `_` 前缀 + `snake_case` | `_db_path` |
| 保护成员变量 | `_` 前缀 + `snake_case` | `_index` |
| 静态成员变量 | `s_` 前缀 + `snake_case` | `s_instance` |

---

## 五、常量命名规范

### 5.1 宏定义

| 常量类型 | 命名规则 | 示例 |
|----------|----------|------|
| 普通宏 | 全大写，下划线分隔 | `MAX_COLLECTION_SIZE` |
| 条件编译宏 | 全大写，下划线分隔 | `ENABLE_DEBUG` |
| 位掩码宏 | 全大写，下划线分隔 | `FLAG_READ_ONLY` |
| 错误码宏 | `JW_` 前缀 + 全大写 | `JW_OK` |

### 5.2 常量变量

| 常量类型 | 命名规则 | 示例 |
|----------|----------|------|
| `const` 变量 | `snake_case` | `default_vector_size` |
| `constexpr` 变量 | `snake_case` | `pi_value` |
| 枚举常量 | 全大写，下划线分隔 | `SUCCESS` |
| 命名空间常量 | `snake_case` | `default_timeout` |

---

## 六、类型命名规范

### 6.1 结构体

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| C 结构体 | `jw_` 前缀 + 名称 + `_t` 后缀 | `jw_vecdb_t` |
| C++ 结构体 | `PascalCase` | `VectorData` |
| 结构体成员 | `snake_case` | `vector_id` |

### 6.2 联合体

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| C 联合体 | `jw_` 前缀 + 名称 + `_u` 后缀 | `jw_value_u` |
| C++ 联合体 | `PascalCase` | `ValueUnion` |

### 6.3 枚举

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| C 枚举 | `jw_` 前缀 + 名称 + `_e` 后缀 | `jw_error_e` |
| C++ 枚举 | `PascalCase` | `ErrorType` |
| 枚举值 | 全大写，下划线分隔 | `JW_OK` |

### 6.4 类

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 普通类 | `PascalCase` | `VecDB` |
| 抽象类 | `Abstract` 前缀 + `PascalCase` | `AbstractIndex` |
| 接口类 | `I` 前缀 + `PascalCase` | `IVectorStore` |
| 工具类 | `Utils` 后缀 | `VectorUtils` |
| 异常类 | `Exception` 后缀 | `VectorException` |

### 6.5 类型别名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| C 类型别名 | `jw_` 前缀 + 名称 + `_t` 后缀 | `jw_vector_t` |
| C++ 类型别名 | `snake_case` | `vector_id_type` |
| `using` 声明 | `snake_case` | `using vector_id = uint64_t;` |

---

## 七、命名风格指南

### 7.1 大小写规范

| 风格 | 适用范围 | 示例 |
|------|----------|------|
| `snake_case` | 函数、变量、文件 | `vector_search` |
| `PascalCase` | 类、结构体、枚举类型 | `VecDBManager` |
| `ALL_CAPS` | 宏、常量、枚举值 | `MAX_VECTOR_SIZE` |
| `camelCase` | 不推荐使用 | 避免使用 |

### 7.2 前缀和后缀

| 前缀/后缀 | 适用范围 | 示例 |
|-----------|----------|------|
| `jw_` | 公共 API 函数、类型 | `jw_vecdb_open` |
| `_t` | C 类型定义 | `jw_vecdb_t` |
| `_e` | C 枚举类型 | `jw_error_e` |
| `_u` | C 联合体类型 | `jw_value_u` |
| `g_` | 全局变量 | `g_db_instance` |
| `s_` | 静态变量 | `s_initialized` |
| `_` | 私有成员变量 | `_db_path` |
| `is_` | 布尔变量 | `is_open` |
| `has_` | 布尔变量 | `has_index` |
| `out_` | 输出参数 | `out_vector_id` |
| `io_` | 输入输出参数 | `io_buffer` |
| `temp_` | 临时变量 | `temp_buffer` |

### 7.3 命名长度

| 命名类型 | 推荐长度 | 示例 |
|----------|----------|------|
| 函数名 | 1-30 字符 | `vector_search` |
| 变量名 | 1-20 字符 | `vector_id` |
| 类名 | 1-20 字符 | `VecDB` |
| 常量名 | 1-30 字符 | `MAX_VECTOR_SIZE` |
| 文件名 | 1-50 字符 | `jw_vecdb.c` |

### 7.4 命名一致性

| 概念 | 推荐命名 | 避免命名 |
|------|----------|----------|
| 数据库 | `db` | `database` |
| 集合 | `collection` | `col` |
| 向量 | `vector` | `vec` (除非上下文明确) |
| 索引 | `index` | `idx` (除非上下文明确) |
| 搜索 | `search` | `find` |
| 插入 | `insert` | `add` |
| 删除 | `delete` | `remove` |
| 更新 | `update` | `modify` |
| 创建 | `create` | `new` |
| 销毁 | `destroy` | `delete` |
| 打开 | `open` | `start` |
| 关闭 | `close` | `stop` |

---

## 八、命名检查工具

### 8.1 静态分析工具

| 工具 | 用途 | 配置方法 |
|------|------|----------|
| ClangFormat | 代码格式化 | `.clang-format` 配置文件 |
| Clang-Tidy | 代码风格检查 | `.clang-tidy` 配置文件 |
| Cppcheck | 静态代码分析 | `cppcheck --enable=style` |
| SonarQube | 代码质量分析 | 配置 SonarQube 规则 |

### 8.2 自定义检查脚本

#### 8.2.1 命名规范检查脚本

```bash
#!/bin/bash

# 检查文件命名
check_file_names() {
    echo "Checking file names..."
    find . -name "*.c" -o -name "*.cpp" -o -name "*.h" | grep -v "build" | while read file; do
        if [[ ! "$file" =~ ^[a-z0-9_/]+\.(c|cpp|h)$ ]]; then
            echo "File name violation: $file"
        fi
    done
}

# 检查函数命名
check_function_names() {
    echo "Checking function names..."
    grep -r "^[a-zA-Z_][a-zA-Z0-9_]*\(" --include="*.c" --include="*.cpp" | grep -v "build" | while read line; do
        if [[ ! "$line" =~ ^[a-z0-9_]+\( ]]; then
            echo "Function name violation: $line"
        fi
    done
}

# 运行检查
check_file_names
check_function_names
```

### 8.3 IDE 集成

| IDE | 插件 | 功能 |
|-----|------|------|
| VS Code | C/C++ Extension Pack | 代码格式化和风格检查 |
| CLion | ClangFormat | 代码格式化 |
| Visual Studio | Code Analysis | 代码风格检查 |
| Xcode | ClangFormat | 代码格式化 |

---

## 九、最佳实践

### 9.1 命名最佳实践

1. **使用描述性名称**
   - 函数名应描述其功能
   - 变量名应描述其用途
   - 类名应描述其职责

2. **保持一致性**
   - 同一项目中使用相同的命名风格
   - 相似功能使用相似的命名模式
   - 遵循项目已有的命名约定

3. **避免歧义**
   - 避免使用可能有多种含义的名称
   - 对于模糊的名称，添加更多描述性信息
   - 使用具体的动词和名词

4. **考虑上下文**
   - 在不同的上下文中，相同的概念可能需要不同的命名
   - 局部变量可以更简短，全局变量应更详细

5. **使用标准缩写**
   - 只使用广泛认可的缩写
   - 对于不常见的缩写，在注释中说明
   - 保持缩写的一致性

### 9.2 命名反模式

| 反模式 | 问题 | 改进 |
|--------|------|------|
| 单字母变量 | 难以理解 | 使用描述性名称 |
| 过度缩写 | 难以理解 | 使用完整单词 |
| 不一致的大小写 | 降低可读性 | 遵循统一的大小写规则 |
| 模糊的名称 | 容易误解 | 使用具体的描述性名称 |
| 过长的名称 | 降低可读性 | 保持名称简洁但清晰 |
| 与关键字冲突 | 可能导致错误 | 避免使用关键字作为名称 |
| 拼音命名 | 国际化支持差 | 使用英文命名 |

---

## 十、附录

### A. 命名规范示例

#### A.1 函数命名示例

```c
// 公共 API 函数
int jw_vecdb_open(jw_vecdb_t** db, const char* path, bool create);
int jw_vecdb_close(jw_vecdb_t* db);
int jw_vecdb_insert_vector(jw_vecdb_t* db, const float* vector, size_t dimension, uint64_t* out_id);

// 内部函数
static int vecdb_internal_init(jw_vecdb_t* db);
static void vecdb_internal_cleanup(jw_vecdb_t* db);

// 工具函数
void* jw_utils_malloc(size_t size);
void jw_utils_free(void* ptr);
```

#### A.2 变量命名示例

```c
// 局部变量
uint64_t vector_id;
bool is_open;
size_t vector_size;

// 全局变量
static bool s_initialized = false;
static jw_vecdb_t* g_db_instance = NULL;

// 常量
#define MAX_VECTOR_SIZE 1000000
#define JW_OK 0
#define JW_ERROR -1
```

#### A.3 类型命名示例

```c
// 结构体
typedef struct {
    uint64_t id;
    float* data;
    size_t dimension;
} jw_vector_t;

// 枚举
typedef enum {
    JW_OK = 0,
    JW_ERROR = -1,
    JW_OUT_OF_MEMORY = -2,
    JW_INVALID_PARAMETER = -3
} jw_error_e;

// 类
class VecDB {
private:
    char* _db_path;
    bool _is_open;
    
public:
    VecDB();
    ~VecDB();
    
    int open(const char* path, bool create);
    int close();
    int insert_vector(const float* vector, size_t dimension, uint64_t* out_id);
};
```

### B. 命名规范检查清单

- [ ] 文件命名符合规范
- [ ] 函数命名符合规范
- [ ] 变量命名符合规范
- [ ] 常量命名符合规范
- [ ] 类型命名符合规范
- [ ] 命名风格一致
- [ ] 命名清晰描述其用途
- [ ] 避免使用反模式
- [ ] 遵循项目约定

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
