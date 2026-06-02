#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

BUILD_DIR="/tmp/jw_ios_test_build"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

INCLUDE_DIR="$PROJECT_ROOT/include"
SRC_DIR="$PROJECT_ROOT/src"
SWIFT_DIR="$SCRIPT_DIR/Sources"

C_FILES=(
    "$SRC_DIR/jw_types.c"
    "$SRC_DIR/jw_string.c"
    "$SRC_DIR/jw_stdio.c"
    "$SRC_DIR/jw_hash.c"
    "$SRC_DIR/jw_sort.c"
    "$SRC_DIR/jw_math.c"
    "$SRC_DIR/jw_error.c"
    "$SRC_DIR/jw_log.c"
    "$SRC_DIR/jw_file.c"
    "$SRC_DIR/jw_lock.c"
    "$SRC_DIR/jw_config.c"
    "$SRC_DIR/jw_arena.c"
    "$SRC_DIR/jw_vector.c"
    "$SRC_DIR/jw_quant.c"
    "$SRC_DIR/jw_index.c"
    "$SRC_DIR/jw_collection.c"
    "$SRC_DIR/jw_storage.c"
    "$SRC_DIR/jw_vecdb.c"
    "$SRC_DIR/jw_ios_bridge.c"
)

# Compile all C files to object files
echo "Compiling C files..."
OBJ_FILES=()
for cf in "${C_FILES[@]}"; do
    obj="$BUILD_DIR/$(basename "$cf" .c).o"
    clang -c -std=c11 -O2 -I"$INCLUDE_DIR" -I"$SRC_DIR" \
        -o "$obj" "$cf" 2>/dev/null
    OBJ_FILES+=("$obj")
done
echo "  C compilation done ($(ls "$BUILD_DIR"/*.o 2>/dev/null | wc -l) .o files)"

# Compile Swift bindings + C objects into one executable
# main.swift is the entry point with top-level code
cp "$SCRIPT_DIR/test_lifecycle.swift" "$BUILD_DIR/main.swift"
echo "Compiling Swift and linking..."
swiftc \
    -I"$INCLUDE_DIR" \
    "${OBJ_FILES[@]}" \
    "$SWIFT_DIR/CBridge.swift" \
    "$SWIFT_DIR/SearchResult.swift" \
    "$SWIFT_DIR/Collection.swift" \
    "$SWIFT_DIR/VecDB.swift" \
    "$BUILD_DIR/main.swift" \
    -o "$BUILD_DIR/test_lifecycle" \
    2>&1 | grep -v "^#\|warning:" || true

if [ ! -f "$BUILD_DIR/test_lifecycle" ]; then
    echo "BUILD FAILED"
    exit 1
fi

echo "Build successful!"
echo ""
"$BUILD_DIR/test_lifecycle"
