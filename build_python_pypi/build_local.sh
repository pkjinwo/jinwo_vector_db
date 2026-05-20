#!/bin/bash
# 本地构建 Python wheel 包

set -e

cd "$(dirname "$0")"

echo "=== 清理旧构建 ==="
rm -rf build/ dist/ *.egg-info/

echo "=== 安装构建依赖 ==="
pip3 install cmake setuptools wheel --upgrade

echo "=== 构建 wheel ==="
pip3 wheel . --no-deps -w dist/ --no-cache-dir

echo "=== 构建完成 ==="
ls -la dist/
