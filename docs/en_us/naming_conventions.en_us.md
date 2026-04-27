# JinWo VecDB Naming Conventions Guide

**Version**: v1.0.0
**Generated**: 2026-04-26
**Document Type**: Public Release

---

## 1. Naming Conventions Overview

### 1.1 Purpose

The purpose of this guide is to establish consistent naming conventions for the JinWo VecDB project, ensuring code readability, maintainability, and consistency across the codebase. These conventions apply to all code, documentation, and project artifacts.

### 1.2 Scope

| Item | Naming Convention |
|------|-------------------|
| Files and Directories | Lowercase with underscores |
| Functions | Snake case (lowercase with underscores) |
| Variables | Snake case |
| Constants | UPPERCASE with underscores |
| Types | Camel case (Pascal case) |
| Macros | UPPERCASE with underscores |
| Enums | Camel case with uppercase first letter |
| Enum values | UPPERCASE with underscores |

---

## 2. File and Directory Naming

### 2.1 Source Files

| Type | Convention | Example |
|------|------------|---------|
| C source files | `jw_<module>.c` | `jw_vecdb.c` |
| Header files | `jw_<module>.h` | `jw_vecdb.h` |
| C++ source files | `jw_<module>.cpp` | `jw_vecdb.cpp` |
| C++ header files | `jw_<module>.hpp` | `jw_vecdb.hpp` |

### 2.2 Directories

| Type | Convention | Example |
|------|------------|---------|
| Source directory | `src/` | `src/` |
| Include directory | `include/` | `include/` |
| Tests directory | `tests/` | `tests/` |
| Examples directory | `examples/` | `examples/` |
| Documentation directory | `docs/` | `docs/` |
| Build directory | `build/` | `build/` |

### 2.3 Documentation Files

| Type | Convention | Example |
|------|------------|---------|
| Markdown files | `<topic>.<language>.md` | `api_reference.en_us.md` |
| Configuration files | `README.md`, `CHANGELOG.md` | `README.md` |

---

## 3. Function Naming

### 3.1 Public Functions

| Convention | Example |
|------------|---------|
| `jw_<module>_<action>` | `jw_vecdb_open`, `jw_collection_insert` |
| `jw_<action>_<object>` | `jw_create_collection`, `jw_search_vectors` |

### 3.2 Private Functions

| Convention | Example |
|------------|---------|
| `_jw_<module>_<action>` | `_jw_vecdb_init`, `_jw_collection_find` |
| `_<module>_<action>` | `_vecdb_init`, `_collection_find` |

### 3.3 Helper Functions

| Convention | Example |
|------------|---------|
| `jw_<module>_<helper>_<action>` | `jw_memory_alloc`, `jw_string_copy` |

---

## 4. Variable Naming

### 4.1 Local Variables

| Convention | Example |
|------------|---------|
| Snake case | `vector_count`, `index_size` |
| Descriptive names | `is_initialized`, `max_vectors` |
| Single-letter variables | Only for loop counters: `i`, `j`, `k` |

### 4.2 Global Variables

| Convention | Example |
|------------|---------|
| Prefix with `g_` | `g_vecdb_instance`, `g_config` |
| Descriptive names | `g_max_memory`, `g_thread_pool_size` |

### 4.3 Member Variables

| Convention | Example |
|------------|---------|
| Prefix with `_` | `_arena`, `_vector_count` |
| Snake case | `_is_open`, `_max_collections` |

---

## 5. Type Naming

### 5.1 Structs and Unions

| Convention | Example |
|------------|---------|
| Camel case | `VecDB`, `Collection` |
| Descriptive names | `VectorEntry`, `IndexConfig` |

### 5.2 Typedefs

| Convention | Example |
|------------|---------|
| Suffix with `_t` | `jw_vecdb_t`, `jw_collection_t` |
| Camel case for types | `Vector`, `Index` |

### 5.3 Enums

| Convention | Example |
|------------|---------|
| Camel case | `IndexType`, `DistanceMetric` |
| Suffix with `_t` (optional) | `IndexType_t` |

---

## 6. Constant and Macro Naming

### 6.1 Constants

| Convention | Example |
|------------|---------|
| UPPERCASE with underscores | `MAX_VECTORS`, `DEFAULT_DIMENSION` |
| Prefix with module name | `JW_VERSION_MAJOR`, `JW_OK` |

### 6.2 Macros

| Convention | Example |
|------------|---------|
| UPPERCASE with underscores | `JW_ASSERT`, `JW_LOG` |
| Prefix with module name | `JW_CHECK_RESULT`, `JW_MEMORY_ALLOC` |

---

## 7. Enum Values

### 7.1 Enum Naming

| Convention | Example |
|------------|---------|
| UPPERCASE with underscores | `INDEX_TYPE_BRUTE_FORCE`, `DISTANCE_L2` |
| Prefix with enum name | `INDEX_TYPE_IVF`, `INDEX_TYPE_PQ` |

---

## 8. Parameter Naming

### 8.1 Function Parameters

| Convention | Example |
|------------|---------|
| Snake case | `vector_data`, `dimension` |
| Descriptive names | `collection_name`, `search_results` |
| Pointer parameters | Prefix with `p_` (optional): `p_vector`, `p_result` |

### 8.2 Callback Parameters

| Convention | Example |
|------------|---------|
| Snake case | `callback_data`, `user_context` |
| Descriptive names | `search_callback`, `error_handler` |

---

## 9. Documentation Naming

### 9.1 Documentation Files

| Type | Convention | Example |
|------|------------|---------|
| API reference | `api_reference.<language>.md` | `api_reference.en_us.md` |
| Integration guides | `<platform>_integration.<language>.md` | `ios_integration.en_us.md` |
| Design documents | `<topic>_design.<language>.md` | `architecture_design.en_us.md` |
| Testing guides | `<type>_testing.<language>.md` | `performance_testing.en_us.md` |

### 9.2 Section Names

| Convention | Example |
|------------|---------|
| Title case | `Function Naming`, `File Structure` |
| Numeric prefix | `1. Introduction`, `2. Core Concepts` |

---

## 10. Testing Naming

### 10.1 Test Files

| Convention | Example |
|------------|---------|
| `test_<module>.c` | `test_vecdb.c`, `test_collection.c` |
| `test_<feature>.c` | `test_concurrent.c`, `test_performance.c` |

### 10.2 Test Functions

| Convention | Example |
|------------|---------|
| `test_<module>_<feature>` | `test_vecdb_open`, `test_collection_insert` |
| `test_<feature>_<scenario>` | `test_vector_search_edge_cases` |

---

## 11. Best Practices

### 11.1 General Principles

| Principle | Description |
|-----------|-------------|
| Consistency | Follow the same convention throughout the codebase |
| Clarity | Use descriptive names that clearly indicate purpose |
| Brevity | Keep names concise but descriptive |
| Avoid abbreviations | Use full words unless the abbreviation is widely understood |
| Avoid Hungarian notation | Don't use type prefixes like `i_`, `f_`, etc. |

### 11.2 Naming Dos and Don'ts

| Do | Don't |
|-----|-------|
| `vector_count` | `vec_cnt` |
| `is_initialized` | `init` |
| `MAX_VECTORS` | `maxVectors` |
| `jw_vecdb_open` | `OpenVecDB` |
| `Collection` | `collection_t` |

### 11.3 Common Abbreviations

| Abbreviation | Full Word | Usage |
|--------------|-----------|--------|
| `vec` | Vector | `vector_data` (not `vec_data`) |
| `idx` | Index | `index_size` (not `idx_size`) |
| `col` | Collection | `collection_name` (not `col_name`) |
| `mem` | Memory | `memory_alloc` (not `mem_alloc`) |
| `cfg` | Configuration | `config` (not `cfg`) |

---

## 12. Naming Tools

### 12.1 Static Analysis Tools

| Tool | Purpose | Usage |
|------|---------|--------|
| clang-format | Code formatting | `clang-format -i *.c *.h` |
| cppcheck | Static analysis | `cppcheck src/ include/` |
| sonarqube | Code quality | SonarQube analysis |

### 12.2 IDE Integration

| IDE | Plugin | Usage |
|------|---------|--------|
| VS Code | C/C++ Extension | Automatic formatting |
| CLion | Code Style | Automatic formatting |
| Visual Studio | Code Analysis | Naming convention checks |

---

## 13. Migration Guidelines

### 13.1 Renaming Process

1. **Identify files to rename**
2. **Update include paths**
3. **Update references in code**
4. **Update documentation**
5. **Run tests to verify**

### 13.2 Example Renaming

**Before**:
```c
// Bad naming
JwVecDB* db;
int vec_dim;
#define MAXVECS 10000
```

**After**:
```c
// Good naming
jw_vecdb_t* db;
int vector_dimension;
#define MAX_VECTORS 10000
```

---

## 14. Validation

### 14.1 Naming Checklist

| Check Item | Description | Status |
|------------|-------------|--------|
| [ ] File names follow convention | All files use lowercase with underscores | ⬜ |
| [ ] Function names follow convention | All functions use snake case | ⬜ |
| [ ] Variable names follow convention | All variables use snake case | ⬜ |
| [ ] Type names follow convention | All types use camel case | ⬜ |
| [ ] Constants follow convention | All constants use UPPERCASE | ⬜ |
| [ ] Macros follow convention | All macros use UPPERCASE | ⬜ |
| [ ] Enum values follow convention | All enum values use UPPERCASE | ⬜ |
| [ ] Documentation follows convention | All documentation uses proper naming | ⬜ |

---

**Document Update Records**

| Date | Version | Update Content |
|------|---------|----------------|
| 2026-04-26 | v1.0.0 | Initial version |
