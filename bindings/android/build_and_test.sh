#!/bin/bash
set -e
cd "$(dirname "$0")"

PROJECT_ROOT="$(cd ../.. && pwd)"
INCLUDE_DIR="$PROJECT_ROOT/include"
SRC_DIR="$PROJECT_ROOT/src"
BUILD_DIR="$PWD/build_java_test"
JAVA_INC="/Applications/IntelliJ IDEA CE.app/Contents/jbr/Contents/Home/include"

echo "=== Building JNI native library ==="
mkdir -p "$BUILD_DIR"

# Collect all C source files (except ios_bridge)
C_FILES=()
for f in "$SRC_DIR"/*.c; do
    case "$(basename "$f")" in
        jw_ios_bridge.c) ;; # skip
        *) C_FILES+=("$f") ;;
    esac
done

# Compile each C file to object
OBJ_FILES=()
for cf in "${C_FILES[@]}"; do
    if [ -f "$cf" ]; then
        obj="$BUILD_DIR/$(basename "${cf%.c}.o")"
        gcc -c -O2 -fPIC -I"$INCLUDE_DIR" "$cf" -o "$obj"
        OBJ_FILES+=("$obj")
    fi
done

# Compile JNI bridge (g++ since it's C++)
JNI_CPP="library/src/main/jni/jinwo_vecdb_jni.cpp"
g++ -c -O2 -fPIC \
    -I"$INCLUDE_DIR" \
    -I"$JAVA_INC" \
    -I"$JAVA_INC/darwin" \
    "$JNI_CPP" \
    -o "$BUILD_DIR/jni.o"
OBJ_FILES+=("$BUILD_DIR/jni.o")

# Link into .dylib
g++ -shared -o "$BUILD_DIR/libjinwo_vecdb.dylib" "${OBJ_FILES[@]}"
echo "  -> Built $BUILD_DIR/libjinwo_vecdb.dylib"

echo ""
echo "=== Compiling Java ==="
JAVA_PKG="library/src/main/java/com/jinwo/vecdb"
javac -d "$BUILD_DIR" "$JAVA_PKG/SearchResult.java" "$JAVA_PKG/Collection.java" "$JAVA_PKG/JinWoDB.java" "TestLifecycle.java" 2>&1
echo "  -> Compiled to $BUILD_DIR"

echo ""
echo "=== Running Test ==="
# Cleanup any leftover db files from previous runs
rm -rf "$PWD"/test_lifecycle_java.jwv
java -cp "$BUILD_DIR" -Djava.library.path="$BUILD_DIR" TestLifecycle
