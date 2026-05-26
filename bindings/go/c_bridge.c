/*
 * c_bridge.c - JinWo VecDB Go C bridge
 *
 * 将所有 C 源文件编译到一个翻译单元，供 cgo 使用。
 * 顺序很重要，需先编译被依赖的模块。
 */

/*
 * c_bridge.c - JinWo VecDB Go C bridge
 *
 * 注意: jw_string.c 内部有 static jw_vsnprintf，与 jw_stdio.c 的公开版本冲突。
 * 使用宏将 jw_string.c 的内部版本重命名以避免冲突。
 */

/* 工具类（无依赖） */
#include "../../src/jw_types.c"
#define jw_vsnprintf jw_string_vsnprintf
#include "../../src/jw_string.c"
#undef jw_vsnprintf
#include "../../src/jw_stdio.c"
#include "../../src/jw_hash.c"
#include "../../src/jw_sort.c"
#include "../../src/jw_math.c"
#include "../../src/jw_error.c"
#include "../../src/jw_log.c"
#include "../../src/jw_file.c"

/* 平台/并发 */
#include "../../src/jw_lock.c"
#include "../../src/jw_config.c"

/* 内存 */
#include "../../src/jw_arena.c"

/* 核心模块 */
#include "../../src/jw_vector.c"
#include "../../src/jw_quant.c"
#include "../../src/jw_index.c"
#include "../../src/jw_collection.c"
#include "../../src/jw_storage.c"
#include "../../src/jw_vecdb.c"
