#!/bin/bash
# ============================================================
# 编译 JinWo VecDB C 库 → WebAssembly (WASM)
# 需要安装 Emscripten: https://emscripten.org/docs/getting_started/downloads.html
# ============================================================

set -e

WASM_DIR="./wasm"
SRC_DIR="../src"
INCLUDE_DIR="../include"

echo "========================================"
echo "JinWo VecDB WASM 编译"
echo "========================================"

# 检查 emcc
if ! command -v emcc &> /dev/null; then
    echo ""
    echo "❌ 未找到 emcc (Emscripten)"
    echo "   请安装 Emscripten SDK:"
    echo "   https://emscripten.org/docs/getting_started/downloads.html"
    echo ""
    echo "   安装步骤:"
    echo "   1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "   2. cd emsdk"
    echo "   3. ./emsdk install latest"
    echo "   4. ./emsdk activate latest"
    echo "   5. source ./emsdk_env.sh"
    exit 1
fi

echo "emcc 版本: $(emcc --version | head -1)"
echo ""

# 创建输出目录
mkdir -p "$WASM_DIR"

# 收集所有 C 源文件
SOURCES=(
    "$SRC_DIR/jw_vecdb.c"
    "$SRC_DIR/jw_collection.c"
    "$SRC_DIR/jw_index.c"
    "$SRC_DIR/jw_lock.c"
    "$SRC_DIR/jw_arena.c"
    "$SRC_DIR/jw_file.c"
    "$SRC_DIR/jw_storage.c"
    "$SRC_DIR/jw_vector.c"
    "$SRC_DIR/jw_math.c"
    "$SRC_DIR/jw_sort.c"
    "$SRC_DIR/jw_quant.c"
    "$SRC_DIR/jw_hash.c"
    "$SRC_DIR/jw_string.c"
    "$SRC_DIR/jw_config.c"
    "$SRC_DIR/jw_error.c"
    "$SRC_DIR/jw_log.c"
    "$SRC_DIR/jw_stdio.c"
    "$SRC_DIR/jw_types.c"
)

echo "编译 $SRC_DIR 下的 C 源文件..."
echo ""

emcc \
    -O2 \
    -s WASM=1 \
    -s EXPORT_ES6=1 \
    -s MODULARIZE=1 \
    -s EXPORTED_FUNCTIONS='[
        "_jw_vecdb_open",
        "_jw_vecdb_close",
        "_jw_vecdb_create_collection",
        "_jw_vecdb_get_collection",
        "_jw_vecdb_insert",
        "_jw_vecdb_search",
        "_jw_vecdb_version",
        "_jw_collection_insert",
        "_jw_collection_delete",
        "_jw_collection_build_index",
        "_malloc",
        "_free"
    ]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","setValue","UTF8ToString","stringToUTF8","lengthBytesUTF8","HEAPU8","HEAPF32","HEAP32","FS"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=64MB \
    -s MAXIMUM_MEMORY=512MB \
    -lnodefs.js \
    -I "$INCLUDE_DIR" \
    "${SOURCES[@]}" \
    -o "$WASM_DIR/jinwo.js"

echo ""
echo "✅ WASM 编译成功!"
echo "   输出: $WASM_DIR/jinwo.wasm"
echo "         $WASM_DIR/jinwo.js"
ls -lh "$WASM_DIR/jinwo.wasm" "$WASM_DIR/jinwo.js"
