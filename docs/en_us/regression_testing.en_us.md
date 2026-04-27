# JinWo VecDB Regression Testing Guide

**Version**: v1.0.0
**Generated**: 2026-04-26
**Document Type**: Public Release

---

## 1. Regression Testing Overview

### 1.1 Testing Goals

The main goal of regression testing is to ensure that new code changes do not break existing functionality, ensuring system stability and correctness throughout all stages and version iterations. Specifically including:

- **Functional regression**: Ensure modified code does not break existing functionality
- **Defect fix verification**: Ensure fixed defects do not reoccur
- **Performance regression detection**: Ensure modifications do not cause performance degradation
- **Cross-platform regression**: Ensure modifications work correctly on all platforms

### 1.2 Regression Testing Strategy

| Strategy | Description | Application Scenario |
|----------|-------------|----------------------|
| Full regression | Run all test cases | Major version release |
| Selective regression | Run tests related to changes | Small modification release |
| Incremental regression | Test only new and modified functionality | Daily development |
| Automated regression | CI/CD automatically triggered tests | Continuous integration environment |

### 1.3 Testing Scope

| Scope | Content | Priority |
|-------|---------|----------|
| Core functionality | Basic CRUD operations | P0 |
| Index functionality | Vector indexing and search | P0 |
| Concurrency safety | Multi-thread operations | P1 |
| Memory management | Memory leaks and overflows | P1 |
| Cross-platform | Multi-platform compatibility | P1 |
| Performance benchmark | Performance metrics | P2 |

---

## 2. Regression Test Cases

### 2.1 Core Functionality Regression Tests

#### 2.1.1 Database Operations

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_DB_001 | Database open | Normal path | Return JW_OK | ⬜ |
| RT_DB_002 | Database close | Normal close | Return JW_OK | ⬜ |
| RT_DB_003 | Database open | Non-existent path | Create new database | ⬜ |
| RT_DB_004 | Database open | Invalid path | Return error code | ⬜ |
| RT_DB_005 | Duplicate open | Same path twice | Return error or reuse | ⬜ |

#### 2.1.2 Collection Operations

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_COL_001 | Create collection | Normal parameters | Return JW_OK | ⬜ |
| RT_COL_002 | Create collection | Existing name | Return error code | ⬜ |
| RT_COL_003 | List collections | Multiple collections | Return correct list | ⬜ |
| RT_COL_004 | Drop collection | Existing collection | Return JW_OK | ⬜ |
| RT_COL_005 | Drop collection | Non-existent collection | Return error code | ⬜ |
| RT_COL_006 | Open collection | Existing collection | Return collection handle | ⬜ |

### 2.2 Vector Operations Regression Tests

#### 2.2.1 Vector Insertion

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_VEC_INS_001 | Single insert | 128-dimensional vector | Return unique ID | ⬜ |
| RT_VEC_INS_002 | Batch insert | 1000 vectors | All successful | ⬜ |
| RT_VEC_INS_003 | Insert empty vector | dimension=0 | Return error code | ⬜ |
| RT_VEC_INS_004 | Insert large dimension | 10000 dimensions | Normal handling | ⬜ |
| RT_VEC_INS_005 | Duplicate insert | Same vector | Return different ID | ⬜ |

#### 2.2.2 Vector Search

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_VEC_SRH_001 | Exact search | Existing vector | Return matching result | ⬜ |
| RT_VEC_SRH_002 | Top-K search | k=10 | Return 10 results | ⬜ |
| RT_VEC_SRH_003 | Empty collection search | No vectors | Return empty result | ⬜ |
| RT_VEC_SRH_004 | Search non-existent | All-zero vector | Return best match | ⬜ |
| RT_VEC_SRH_005 | Dimension mismatch | Query dimension != collection dimension | Return error code | ⬜ |

#### 2.2.3 Vector Deletion

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_VEC_DEL_001 | Delete existing vector | Valid ID | Return JW_OK | ⬜ |
| RT_VEC_DEL_002 | Delete non-existent vector | Invalid ID | Return error code | ⬜ |
| RT_VEC_DEL_003 | Duplicate delete | Same ID twice | Second return error | ⬜ |
| RT_VEC_DEL_004 | Search after delete | Deleted ID | Return empty or error | ⬜ |

### 2.3 Concurrency Regression Tests

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_CON_001 | Multi-thread insert | 4 concurrent threads | No data loss | ⬜ |
| RT_CON_002 | Multi-thread search | 4 concurrent threads | Correct results | ⬜ |
| RT_CON_003 | Read-write concurrent | 2 read 2 write threads | No crash | ⬜ |
| RT_CON_004 | Extreme concurrent | 16 threads | No deadlock | ⬜ |
| RT_CON_005 | Concurrent delete | Multiple threads deleting | Correct status | ⬜ |

### 2.4 Memory Management Regression Tests

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_MEM_001 | Memory leak detection | Long running | No memory leak | ⬜ |
| RT_MEM_002 | Out of memory handling | Allocation failure scenario | Normal error return | ⬜ |
| RT_MEM_003 | Memory reclamation | Mass insert/delete | Memory correctly reclaimed | ⬜ |
| RT_MEM_004 | Arena memory | Multiple allocations/releases | Arena works normally | ⬜ |

### 2.5 Storage Regression Tests

| Case ID | Test Item | Input | Expected Output | Status |
|---------|-----------|-------|----------------|--------|
| RT_STO_001 | Data persistence | Insert then close then open | Data complete | ⬜ |
| RT_STO_002 | Data recovery | Abnormal close then open | Data complete or recovered | ⬜ |
| RT_STO_003 | Large data storage | 10M vectors | Storage successful | ⬜ |
| RT_STO_004 | Storage corruption | Manually corrupt data | Detect and error | ⬜ |

### 2.6 Cross-platform Regression Tests

| Case ID | Test Item | Platform | Expected Output | Status |
|---------|-----------|----------|----------------|--------|
| RT_CP_001 | Linux build | Linux | Compile and run successfully | ⬜ |
| RT_CP_002 | macOS build | macOS | Compile and run successfully | ⬜ |
| RT_CP_003 | iOS build | iOS | Compile successfully | ⬜ |
| RT_CP_004 | Android build | Android | Compile successfully | ⬜ |
| RT_CP_005 | Windows build | Windows | Compile and run successfully | ⬜ |

---

## 3. Regression Test Execution

### 3.1 Local Regression Testing

#### 3.1.1 Quick Regression Test

```bash
#!/bin/bash
# Quick regression test script

echo "=== JinWo VecDB Quick Regression Test ==="

# 1. Compile
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DJW_BUILD_TESTS=ON
make -j$(nproc)

# 2. Run core tests
echo "Running core functionality tests..."
ctest -R "test_vecdb|test_collection|test_vector" -V

# 3. Check results
if [ $? -eq 0 ]; then
    echo "Core tests passed!"
else
    echo "Core tests failed!"
    exit 1
fi

# 4. Cleanup
cd ..
rm -rf build

echo "=== Quick regression test completed ==="
```

#### 3.1.2 Full Regression Test

```bash
#!/bin/bash
# Full regression test script

echo "=== JinWo VecDB Full Regression Test ==="

# 1. Debug build
mkdir -p build_debug && cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DJW_BUILD_TESTS=ON
make -j$(nproc)

# 2. Run all tests
echo "Running all tests..."
ctest -V

# 3. Return code check
TEST_RESULT=$?

cd ..

# 4. Cleanup
rm -rf build_debug

if [ $TEST_RESULT -eq 0 ]; then
    echo "Full tests passed!"
else
    echo "Tests failed, please check logs"
    exit 1
fi

echo "=== Full regression test completed ==="
```

### 3.2 CI/CD Automatic Regression

#### 3.2.1 GitHub Actions Configuration

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

### 3.3 Regression Test Report

#### 3.3.1 Test Result Record

| Item | Result |
|------|--------|
| Test date | 2026-04-26 |
| Tester | [Tester Name] |
| Test environment | Linux x86_64, GCC 11.0 |
| Total test cases | N |
| Passed test cases | N |
| Failed test cases | N |
| Pass rate | XX% |

#### 3.3.2 Failed Test Details

| Case ID | Test Description | Failure Reason | Solution | Fix Date |
|---------|------------------|----------------|----------|----------|
| RT_XXX | Description | Reason | Solution | Date |

---

## 4. Regression Test Checklist

### 4.1 Pre-release Check

| Check Item | Description | Status |
|------------|-------------|--------|
| [ ] Core functionality tests passed | Basic CRUD operations normal | ⬜ |
| [ ] Vector operations tests passed | Insert, search, delete normal | ⬜ |
| [ ] Concurrency tests passed | Multi-thread operations no issues | ⬜ |
| [ ] Memory tests passed | No memory leaks | ⬜ |
| [ ] Storage tests passed | Data persistence normal | ⬜ |
| [ ] Cross-platform tests passed | All platforms compile and run normal | ⬜ |
| [ ] Performance tests passed | No significant performance degradation | ⬜ |
| [ ] Documentation updated | Related documentation updated | ⬜ |
| [ ] Changelog updated | Change content recorded | ⬜ |

### 4.2 Regression Test Passing Criteria

| Criteria | Requirement |
|----------|-------------|
| Test case pass rate | ≥ 95% |
| P0 test case pass rate | 100% |
| Memory leaks | None |
| Deadlocks | None |
| Crashes | None |
| Performance degradation | ≤ 10% |

---

## 5. Troubleshooting

### 5.1 Common Test Failures

| Issue | Possible Cause | Solution |
|-------|----------------|----------|
| Compilation failure | Missing dependencies | Install required dependencies |
| Link error | Library path error | Check link configuration |
| Test timeout | High system load | Reduce concurrency or retry |
| Memory leak | Resource not released | Check and fix code |
| Deadlock | Lock order issue | Check lock usage |
| Crash | Null pointer/out of bounds | Check boundary conditions |

### 5.2 Debugging Steps

1. **View detailed logs**
   ```bash
   ctest -V > test_log.txt 2>&1
   ```

2. **Run failed test individually**
   ```bash
   ./tests/test_xxx --gtest_filter=TestName
   ```

3. **Use debugger**
   ```bash
   gdb ./tests/test_xxx
   (gdb) run --gtest_filter=TestName
   ```

4. **Memory check**
   ```bash
   valgrind --leak-check=full ./tests/test_xxx
   ```

---

## 6. Best Practices

### 6.1 Test Writing Standards

1. **Test independence**: Each test case should run independently, not dependent on other tests
2. **Test repeatability**: Test results should be stable and repeatable
3. **Clear assertions**: Use clear assertion messages
4. **Complete coverage**: Cover normal and exceptional cases

### 6.2 Test Execution Standards

1. **Regular execution**: Run related tests after each code modification
2. **Automation**: Integrate into CI/CD process
3. **Monitoring**: Track test result trends
4. **Timely fixes**: Fix discovered issues promptly

### 6.3 Regression Test Management

1. **Case maintenance**: Regularly review and update test cases
2. **Priority management**: Assign priorities based on importance
3. **Classification management**: Categorize test cases by functional module
4. **Version management**: Record test results for different versions

---

**Document Update Records**

| Date | Version | Update Content |
|------|---------|----------------|
| 2026-04-26 | v1.0.0 | Initial version |
