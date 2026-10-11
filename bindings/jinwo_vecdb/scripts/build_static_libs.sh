#!/usr/bin/env bash
# =============================================================================
# 为各平台预编译 JinWo VecDB 静态库 libjinwo.a，供 Go binding 按 GOOS/GOARCH 链接。
#
# 用法：
#   bash scripts/build_static_libs.sh
#
# 产物： bindings/jinwo_vecdb/lib/<goos>_<goarch>/libjinwo.a
#
# 说明：
#   - 本地（macOS）可同时构建 darwin/amd64 与 darwin/arm64（Apple clang -arch）。
#   - linux / windows 需要对应交叉工具链（x86_64-linux-gnu-gcc、aarch64-linux-gnu-gcc、
#     x86_64-w64-mingw32-gcc、aarch64-w64-mingw32-gcc）。缺失时脚本会自动跳过，
#     建议在对应平台或 CI 中分别构建后提交产物。
#   - 源文件清单与根 CMakeLists.txt 的 JINWO_SOURCES 保持一致（不含 jw_ios_bridge.c，
#     该文件仅 iOS 平台使用）。
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SRC="$ROOT/src"
INC="$ROOT/include"
OUT="$ROOT/bindings/jinwo_vecdb/lib"
AR="${AR:-ar}"

# 与根 CMakeLists.txt 的 JINWO_SOURCES 保持一致（顺序无关，逐个 TU 编译）
SOURCES=(jw_arena jw_collection jw_config jw_error jw_file jw_hash jw_index \
         jw_lock jw_log jw_math jw_quant jw_sort jw_stdio jw_storage \
         jw_string jw_types jw_vecdb jw_vector)

mkdir -p "$OUT"

build_one() {
  local goos=$1 goarch=$2 cc=$3 flags=$4
  local outdir="$OUT/${goos}_${goarch}"
  echo ">> building libjinwo.a for ${goos}/${goarch} (cc=$cc)"
  mkdir -p "$outdir"
  local objdir
  objdir="$(mktemp -d)"
  local objs=()
  for s in "${SOURCES[@]}"; do
    local o="$objdir/$s.o"
    "$cc" $flags -I"$INC" -DJW_EXPORTS -std=c99 -c "$SRC/$s.c" -o "$o"
    objs+=("$o")
  done
  "$AR" rcs "$outdir/libjinwo.a" "${objs[@]}"
  rm -rf "$objdir"
  echo "   -> $outdir/libjinwo.a"
}

# ---- macOS（本地可直接构建两个架构）----
build_one darwin amd64  clang "-O2 -fPIC -arch x86_64 -mmacosx-version-min=11.0"
build_one darwin arm64  clang "-O2 -fPIC -arch arm64 -mmacosx-version-min=11.0"

# ---- Linux（需交叉工具链，缺失则跳过）----
if command -v x86_64-linux-gnu-gcc >/dev/null 2>&1; then
  build_one linux amd64 x86_64-linux-gnu-gcc "-O2 -fPIC"
fi
if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
  build_one linux arm64 aarch64-linux-gnu-gcc "-O2 -fPIC"
fi

# ---- Windows / MinGW-w64（需 MinGW 工具链，缺失则跳过）----
if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  build_one windows amd64 x86_64-w64-mingw32-gcc "-O2 -fPIC"
fi
if command -v aarch64-w64-mingw32-gcc >/dev/null 2>&1; then
  build_one windows arm64 aarch64-w64-mingw32-gcc "-O2 -fPIC"
fi

echo "done. 静态库输出目录: $OUT"
