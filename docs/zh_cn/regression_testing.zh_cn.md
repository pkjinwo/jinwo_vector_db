# JinWo VecDB 回归测试指南

**版本**: v1.0.0
**生成时间**: 2026-04-26
**文档类型**: 对外发布

---

## 一、回归测试概述

### 1.1 测试目标

回归测试的主要目标是确保新代码的修改不会破坏现有功能，保证系统在各个阶段和版本迭代中的稳定性和正确性。具体包括：

- **功能回归**: 确保修改后的代码没有破坏现有功能
- **缺陷修复验证**: 确保已修复的缺陷不再重现
- **性能回归检测**: 确保修改没有导致性能下降
- **跨平台回归**: 确保修改在各平台上都能正常工作

### 1.2 回归测试策略

| 策略 | 描述 | 适用场景 |
|------|------|----------|
| 完全回归 | 运行全部测试用例 | 重大版本发布 |
| 选择性回归 | 运行与修改相关的测试用例 | 小修改发布 |
| 增量回归 | 只测试新增和修改的功能 | 日常开发 |
| 自动化回归 | CI/CD自动触发测试 | 持续集成环境 |

### 1.3 测试范围

| 范围 | 内容 | 优先级 |
|------|------|--------|
| 核心功能 | 基本CRUD操作 | P0 |
| 索引功能 | 向量索引和搜索 | P0 |
| 并发安全 | 多线程操作 | P1 |
| 内存管理 | 内存泄漏和溢出 | P1 |
| 跨平台 | 多平台兼容性 | P1 |
| 性能基准 | 性能指标 | P2 |

---

## 二、回归测试用例

### 2.1 核心功能回归测试

#### 2.1.1 数据库操作

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_DB_001 | 数据库打开 | 正常路径 | 返回 JW_OK | ⬜ |
| RT_DB_002 | 数据库关闭 | 正常关闭 | 返回 JW_OK | ⬜ |
| RT_DB_003 | 数据库打开 | 不存在的路径 | 创建新数据库 | ⬜ |
| RT_DB_004 | 数据库打开 | 非法路径 | 返回错误码 | ⬜ |
| RT_DB_005 | 重复打开 | 同一路径两次 | 返回错误或重用 | ⬜ |

#### 2.1.2 集合操作

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_COL_001 | 创建集合 | 正常参数 | 返回 JW_OK | ⬜ |
| RT_COL_002 | 创建集合 | 已存在的名称 | 返回错误码 | ⬜ |
| RT_COL_003 | 列举集合 | 有多个集合 | 返回正确列表 | ⬜ |
| RT_COL_004 | 删除集合 | 存在的集合 | 返回 JW_OK | ⬜ |
| RT_COL_005 | 删除集合 | 不存在的集合 | 返回错误码 | ⬜ |
| RT_COL_006 | 打开集合 | 存在的集合 | 返回集合句柄 | ⬜ |

### 2.2 向量操作回归测试

#### 2.2.1 向量插入

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_VEC_INS_001 | 单次插入 | 128维向量 | 返回唯一ID | ⬜ |
| RT_VEC_INS_002 | 批量插入 | 1000个向量 | 全部成功 | ⬜ |
| RT_VEC_INS_003 | 插入空向量 | dimension=0 | 返回错误码 | ⬜ |
| RT_VEC_INS_004 | 插入极大维度 | 10000维 | 正常处理 | ⬜ |
| RT_VEC_INS_005 | 重复插入 | 相同向量 | 返回不同ID | ⬜ |

#### 2.2.2 向量搜索

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_VEC_SRH_001 | 精确搜索 | 存在的向量 | 返回匹配结果 | ⬜ |
| RT_VEC_SRH_002 | Top-K搜索 | k=10 | 返回10个结果 | ⬜ |
| RT_VEC_SRH_003 | 空集合搜索 | 无向量 | 返回空结果 | ⬜ |
| RT_VEC_SRH_004 | 搜索不存在 | 全0向量 | 返回最佳匹配 | ⬜ |
| RT_VEC_SRH_005 | 维度不匹配 | 查询维度!=集合维度 | 返回错误码 | ⬜ |

#### 2.2.3 向量删除

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_VEC_DEL_001 | 删除存在的向量 | 有效ID | 返回 JW_OK | ⬜ |
| RT_VEC_DEL_002 | 删除不存在的向量 | 无效ID | 返回错误码 | ⬜ |
| RT_VEC_DEL_003 | 重复删除 | 同一ID两次 | 第二次返回错误 | ⬜ |
| RT_VEC_DEL_004 | 删除后搜索 | 已删除ID | 返回空或错误 | ⬜ |

### 2.3 并发回归测试

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_CON_001 | 多线程插入 | 4线程并发 | 无数据丢失 | ⬜ |
| RT_CON_002 | 多线程搜索 | 4线程并发 | 结果正确 | ⬜ |
| RT_CON_003 | 读写并发 | 2读2写线程 | 无崩溃 | ⬜ |
| RT_CON_004 | 极端并发 | 16线程 | 无死锁 | ⬜ |
| RT_CON_005 | 并发删除 | 多线程同时删除 | 状态正确 | ⬜ |

### 2.4 内存管理回归测试

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_MEM_001 | 内存泄漏检测 | 长时间运行 | 无内存泄漏 | ⬜ |
| RT_MEM_002 | 内存不足处理 | 分配失败场景 | 正常返回错误 | ⬜ |
| RT_MEM_003 | 内存回收 | 大量插入删除 | 内存正确回收 | ⬜ |
| RT_MEM_004 | Arena内存 | 多次分配释放 | Arena正常工作 | ⬜ |

### 2.5 存储回归测试

| 用例ID | 测试项 | 输入 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_STO_001 | 数据持久化 | 插入后关闭再打开 | 数据完整 | ⬜ |
| RT_STO_002 | 数据恢复 | 异常关闭后打开 | 数据完整或恢复 | ⬜ |
| RT_STO_003 | 大数据存储 | 10M向量 | 存储成功 | ⬜ |
| RT_STO_004 | 存储损坏 | 手动损坏数据 | 检测并报错 | ⬜ |

### 2.6 跨平台回归测试

| 用例ID | 测试项 | 平台 | 预期输出 | 状态 |
|--------|--------|------|----------|------|
| RT_CP_001 | Linux构建 | Linux | 编译运行成功 | ⬜ |
| RT_CP_002 | macOS构建 | macOS | 编译运行成功 | ⬜ |
| RT_CP_003 | iOS构建 | iOS | 编译成功 | ⬜ |
| RT_CP_004 | Android构建 | Android | 编译成功 | ⬜ |
| RT_CP_005 | Windows构建 | Windows | 编译运行成功 | ⬜ |

---

## 三、回归测试执行

### 3.1 本地回归测试

#### 3.1.1 快速回归测试

```bash
#!/bin/bash
# 快速回归测试脚本

echo "=== JinWo VecDB 快速回归测试 ==="

# 1. 编译
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_TESTS=ON
make -j$(nproc)

# 2. 运行核心测试
echo "运行核心功能测试..."
ctest -R "test_vecdb|test_collection|test_vector" -V

# 3. 检查结果
if [ $? -eq 0 ]; then
    echo "核心测试通过!"
else
    echo "核心测试失败!"
    exit 1
fi

# 4. 清理
cd ..
rm -rf build

echo "=== 快速回归测试完成 ==="
```

#### 3.1.2 完整回归测试

```bash
#!/bin/bash
# 完整回归测试脚本

echo "=== JinWo VecDB 完整回归测试 ==="

# 1. Debug构建
mkdir -p build_debug && cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DJW_BUILD_TESTS=ON
make -j$(nproc)

# 2. 运行所有测试
echo "运行所有测试..."
ctest -V

# 3. 返回码检查
TEST_RESULT=$?

cd ..

# 4. 清理
rm -rf build_debug

if [ $TEST_RESULT -eq 0 ]; then
    echo "完整测试通过!"
else
    echo "测试失败，请检查日志"
    exit 1
fi

echo "=== 完整回归测试完成 ==="
```

### 3.2 CI/CD 自动回归

#### 3.2.1 GitHub Actions 配置

```yaml
# .github/workflows/regression.yml
name: Regression Tests

on:
  push:
    branches: [main, master, release/*]
  pull_request:
    branches: [main, master]

jobs:
  regression:
    runs-on: ubuntu-20.04
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake gcc g++ valgrind

      - name: Configure
        run: |
          mkdir -p build
          cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_TESTS=ON

      - name: Build
        run: |
          cd build
          make -j$(nproc)

      - name: Core Tests
        run: |
          cd build
          ctest -R "test_vecdb|test_collection|test_vector" --output-on-failure

      - name: Memory Check
        run: |
          cd build
          valgrind --leak-check=full --error-exitcode=1 ./tests/test_vecdb || true

      - name: Full Tests
        run: |
          cd build
          ctest --output-on-failure

  regression-macos:
    runs-on: macos-12
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: brew install cmake
      - name: Build and Test
        run: |
          mkdir -p build && cd build
          cmake .. && make -j$(sysctl -n hw.ncpu)
          ctest --output-on-failure

  regression-windows:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v3
      - name: Build
        shell: pwsh
        run: |
          cmake .. -G "Visual Studio 17 2022" -A x64
          cmake --build . --config Release
      - name: Test
        shell: pwsh
        run: ctest -C Release --output-on-failure
```

### 3.3 回归测试报告

#### 3.3.1 测试结果记录

| 项目 | 结果 |
|------|------|
| 测试日期 | 2026-04-26 |
| 测试人员 | [测试者姓名] |
| 测试环境 | Linux x86_64, GCC 11.0 |
| 总用例数 | N |
| 通过用例数 | N |
| 失败用例数 | N |
| 通过率 | XX% |

#### 3.3.2 失败用例详情

| 用例ID | 用例描述 | 失败原因 | 解决方案 | 修复日期 |
|--------|----------|----------|----------|----------|
| RT_XXX | 描述 | 原因 | 方案 | 日期 |

---

## 四、回归测试检查清单

### 4.1 发布前检查

| 检查项 | 描述 | 状态 |
|--------|------|------|
| [ ] 核心功能测试通过 | 基本CRUD操作正常 | ⬜ |
| [ ] 向量操作测试通过 | 插入、搜索、删除正常 | ⬜ |
| [ ] 并发测试通过 | 多线程操作无问题 | ⬜ |
| [ ] 内存测试通过 | 无内存泄漏 | ⬜ |
| [ ] 存储测试通过 | 数据持久化正常 | ⬜ |
| [ ] 跨平台测试通过 | 各平台编译运行正常 | ⬜ |
| [ ] 性能测试通过 | 性能无明显下降 | ⬜ |
| [ ] 文档更新 | 相关文档已更新 | ⬜ |
| [ ] 变更日志更新 | 变更内容已记录 | ⬜ |

### 4.2 回归测试通过标准

| 标准 | 要求 |
|------|------|
| 用例通过率 | ≥ 95% |
| P0用例通过率 | 100% |
| 内存泄漏 | 无 |
| 死锁 | 无 |
| 崩溃 | 无 |
| 性能下降 | ≤ 10% |

---

## 五、故障排除

### 5.1 常见测试失败

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| 编译失败 | 依赖缺失 | 安装所需依赖 |
| 链接错误 | 库路径错误 | 检查链接配置 |
| 测试超时 | 系统负载高 | 减少并发或重试 |
| 内存泄漏 | 资源未释放 | 检查并修复代码 |
| 死锁 | 锁顺序问题 | 检查锁使用 |
| 崩溃 | 空指针/越界 | 检查边界条件 |

### 5.2 调试步骤

1. **查看详细日志**
   ```bash
   ctest -V > test_log.txt 2>&1
   ```

2. **单独运行失败测试**
   ```bash
   ./tests/test_xxx --gtest_filter=TestName
   ```

3. **使用调试器**
   ```bash
   gdb ./tests/test_xxx
   (gdb) run --gtest_filter=TestName
   ```

4. **内存检查**
   ```bash
   valgrind --leak-check=full ./tests/test_xxx
   ```

---

## 六、最佳实践

### 6.1 测试编写规范

1. **测试独立性**: 每个测试用例应独立运行，不依赖其他测试
2. **测试可重复性**: 测试结果应稳定可重复
3. **清晰的断言**: 使用明确的断言消息
4. **完整的覆盖**: 覆盖正常和异常情况

### 6.2 测试执行规范

1. **定期执行**: 每次代码修改后运行相关测试
2. **自动化**: 集成到CI/CD流程中
3. **监控**: 跟踪测试结果趋势
4. **及时修复**: 发现问题及时修复

### 6.3 回归测试管理

1. **用例维护**: 定期审查和更新测试用例
2. **优先级管理**: 根据重要程度分配优先级
3. **分类管理**: 按功能模块分类测试用例
4. **版本管理**: 记录不同版本的测试结果

---

**文档更新记录**

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-04-26 | v1.0.0 | 初始版本 |
