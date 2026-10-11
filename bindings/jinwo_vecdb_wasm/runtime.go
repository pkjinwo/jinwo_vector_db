package jinwo_vecdb_wasm

// runtime.go - wazero 运行时封装
//
// 通过纯 Go 的 wazero 运行时加载并运行 Emscripten 构建的 jinwo.wasm，
// 完全不依赖 C 工具链 / C 链接器（区别于 bindings/jinwo_vecdb 的 cgo 静态库方案）。
//
// wasm 导入需求：
//   - wasi_snapshot_preview1.*  -> 由 wazero 内置 WASI 实现满足（fd_read/write/seek/close/sync）
//   - env.* (Emscripten 专用)  -> 本文件提供 host function 桩（内存模式下基本不会被调用）
//
// 注意：jinwo.wasm 是 Emscripten standalone 构建，带 start 段，wazero 实例化时
// 会自动执行 __wasm_call_ctors 完成 C 运行时初始化，无需手动调用。

import (
	"context"
	"fmt"
	"math"
	"os"
	"sync"
	"time"

	"github.com/tetratelabs/wazero"
	"github.com/tetratelabs/wazero/api"
	"github.com/tetratelabs/wazero/imports/wasi_snapshot_preview1"
)

// wasm32 ABI 布局（与 npm_package/dist/index.js 保持一致）
const (
	szJwStrT      = 8  // jw_str_t: { ptr(4), slen(4) }
	szJwVidT      = 8  // jw_vid_t: u64
	szJwSearchRes = 32 // 见 index.js 注释：id(u64@0)+score(f32@8)+...
)

// 打开标志（与 C 头文件 jw_vecdb_flag_t 对应）
const (
	flagReadOnly  uint32 = 0x01
	flagReadWrite uint32 = 0x02
	flagCreate    uint32 = 0x04
	flagTruncate  uint32 = 0x08
	flagMemory    uint32 = 0x10
	flagNoMmap    uint32 = 0x20
	flagNoCache   uint32 = 0x40
	flagSync      uint32 = 0x80
)

// runtime 持有 wazero 运行时与已实例化的 wasm 模块（C 库为全局单例）。
type runtime struct {
	rt  wazero.Runtime
	mod api.Module

	fnVersion        api.Function
	fnOpen           api.Function
	fnClose          api.Function
	fnCreateColl     api.Function
	fnGetColl        api.Function
	fnInsert         api.Function
	fnCollInsert     api.Function
	fnSearch         api.Function
	fnCollDelete     api.Function
	fnCollBuildIndex api.Function
	fnMalloc         api.Function
	fnFree           api.Function
}

var (
	shared *runtime
	initErr error
	once   sync.Once
)

// instance 惰性初始化并复用全局 wasm 运行时。
func instance() (*runtime, error) {
	once.Do(func() { shared, initErr = newRuntime() })
	return shared, initErr
}

func newRuntime() (*runtime, error) {
	ctx := context.Background()

	// 使用默认运行时（解释器模式，纯 Go，无需 CGO / C 链接器）
	rt := wazero.NewRuntime(ctx)

	// 1) 内置 WASI 实现，满足 wasi_snapshot_preview1.fd_*（stdout/stderr 由主模块 ModuleConfig 提供）
	if _, err := wasi_snapshot_preview1.NewBuilder(rt).Instantiate(ctx); err != nil {
		return nil, fmt.Errorf("wasi instantiate: %w", err)
	}

	// 2) env 模块：Emscripten 专用导入的 host 桩
	//    内存模式下这些函数不会被调用，但实例化必须存在；返回值让调用方优雅失败。
	env := rt.NewHostModuleBuilder("env")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module) float64 {
		return float64(time.Now().UnixMilli())
	}).Export("emscripten_date_now")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c, d uint32) uint32 {
		return ^uint32(0) // ENOSYS-ish：文件模式在 wazero 下不支持
	}).Export("__syscall_openat")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a uint32, b uint64) uint32 {
		return ^uint32(0)
	}).Export("__syscall_ftruncate64")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c uint32) uint32 {
		return ^uint32(0)
	}).Export("__syscall_mkdirat")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c uint32) uint32 {
		return ^uint32(0)
	}).Export("__syscall_unlinkat")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a uint32) uint32 {
		return ^uint32(0)
	}).Export("__syscall_rmdir")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b uint32) uint32 {
		return ^uint32(0)
	}).Export("__syscall_stat64")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c, d, e uint32, f uint64) uint32 {
		return 0
	}).Export("_munmap_js")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c, d, e uint32, f uint64) uint32 {
		return 0
	}).Export("_msync_js")
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, a, b, c, d uint32, e uint64, f, g uint32) uint32 {
		return ^uint32(0)
	}).Export("_mmap_js")
	// 堆需要增长时由 Emscripten 运行时调用；尝试直接增长 wasm 内存（按页）。
	env.NewFunctionBuilder().WithFunc(func(ctx context.Context, mod api.Module, requested uint32) uint32 {
		mem := mod.Memory()
		curPages, _ := mem.Grow(0) // 不改变内存，返回当前页数
		curBytes := uint64(curPages) * 65536
		if uint64(requested) <= curBytes {
			return 1
		}
		deltaPages := uint32((uint64(requested) - curBytes + 65535) / 65536)
		if _, ok := mem.Grow(deltaPages); !ok {
			return 0
		}
		return 1
	}).Export("emscripten_resize_heap")

	if _, err := env.Instantiate(ctx); err != nil {
		return nil, fmt.Errorf("env instantiate: %w", err)
	}

	// 3) 实例化主模块（start 段会自动执行 __wasm_call_ctors）。
	//    stdout/stderr 通过 ModuleConfig 注入，供 WASI fd_write 使用。
	compiled, err := rt.CompileModule(ctx, wasmBytes)
	if err != nil {
		return nil, fmt.Errorf("compile jinwo.wasm: %w", err)
	}
	mod, err := rt.InstantiateModule(ctx, compiled,
		wazero.NewModuleConfig().WithStdout(os.Stdout).WithStderr(os.Stderr))
	if err != nil {
		return nil, fmt.Errorf("instantiate jinwo.wasm: %w", err)
	}

	r := &runtime{rt: rt, mod: mod}
	if err := r.bind(); err != nil {
		return nil, err
	}
	return r, nil
}

// bind 缓存导出的 wasm 函数句柄。
func (r *runtime) bind() error {
	get := func(name string) (api.Function, error) {
		fn := r.mod.ExportedFunction(name)
		if fn == nil {
			return nil, fmt.Errorf("jinwo.wasm missing export: %s", name)
		}
		return fn, nil
	}
	var err error
	if r.fnVersion, err = get("jw_vecdb_version"); err != nil {
		return err
	}
	if r.fnOpen, err = get("jw_vecdb_open"); err != nil {
		return err
	}
	if r.fnClose, err = get("jw_vecdb_close"); err != nil {
		return err
	}
	if r.fnCreateColl, err = get("jw_vecdb_create_collection"); err != nil {
		return err
	}
	if r.fnGetColl, err = get("jw_vecdb_get_collection"); err != nil {
		return err
	}
	if r.fnInsert, err = get("jw_vecdb_insert"); err != nil {
		return err
	}
	if r.fnCollInsert, err = get("jw_collection_insert"); err != nil {
		return err
	}
	if r.fnSearch, err = get("jw_vecdb_search"); err != nil {
		return err
	}
	if r.fnCollDelete, err = get("jw_collection_delete"); err != nil {
		return err
	}
	if r.fnCollBuildIndex, err = get("jw_collection_build_index"); err != nil {
		return err
	}
	if r.fnMalloc, err = get("malloc"); err != nil {
		return err
	}
	if r.fnFree, err = get("free"); err != nil {
		return err
	}
	return nil
}

// ---- 内存工具：在 wasm 线性内存中读写 C 结构体 / 向量 ----

func (r *runtime) malloc(size uint32) uint32 {
	if size == 0 {
		return 0
	}
	res, err := r.fnMalloc.Call(context.Background(), uint64(size))
	if err != nil {
		panic(fmt.Sprintf("jinwo_vecdb_wasm: malloc(%d) failed: %v", size, err))
	}
	return uint32(res[0])
}

func (r *runtime) free(ptr uint32) {
	if ptr == 0 {
		return
	}
	_, _ = r.fnFree.Call(context.Background(), uint64(ptr))
}

// writeJwStr 在 wasm 内存中构造 jw_str_t: [struct(8)] + [utf8 data] + [null]
// 返回 struct 的指针；调用方负责 free 该指针。
func (r *runtime) writeJwStr(s string) uint32 {
	ptr := r.malloc(szJwStrT + uint32(len(s)) + 1)
	mem := r.mod.Memory()
	_ = mem.WriteString(ptr+szJwStrT, s)
	_ = mem.WriteByte(ptr+szJwStrT+uint32(len(s)), 0)
	_ = mem.WriteUint32Le(ptr, ptr+szJwStrT) // ptr 字段
	_ = mem.WriteUint32Le(ptr+4, uint32(len(s))) // slen 字段
	return ptr
}

func (r *runtime) freeJwStr(ptr uint32) { r.free(ptr) }

// writeFloats 把 float32 切片写入 wasm 内存，返回数据指针。
func (r *runtime) writeFloats(v []float32) uint32 {
	if len(v) == 0 {
		return 0
	}
	buf := make([]byte, len(v)*4)
	for i, f := range v {
		putU32le(buf[i*4:], math.Float32bits(f))
	}
	ptr := r.malloc(uint32(len(buf)))
	_ = r.mod.Memory().Write(ptr, buf)
	return ptr
}

func putU32le(b []byte, v uint32) {
	b[0] = byte(v)
	b[1] = byte(v >> 8)
	b[2] = byte(v >> 16)
	b[3] = byte(v >> 24)
}
