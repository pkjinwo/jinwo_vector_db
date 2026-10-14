# JinWo VecDB Local Build Guide (C Development Library & Headers)

This guide explains how to compile JinWo VecDB's pure-C core (`src/*.c` +
`include/*.h`) into a local development library and headers, so you can
integrate it into your own C/C++ project or develop on top of the source.

> Chinese version: `docs/zh_cn/build_guide.zh_cn.md`
> The build entry point is the root `CMakeLists.txt`, which shares the same
> C source list with `build_python_pypi/CMakeLists.txt`. Do not maintain them
> separately.

---

## 1. Build Artifacts

| Type | File |
| --- | --- |
| Shared lib (Linux) | `libjinwo.so` |
| Shared lib (macOS) | `libjinwo.dylib` |
| Shared lib (Windows) | `libjinwo.dll` |
| Static lib (Unix/macOS) | `libjinwo.a` |
| Import lib (Windows) | `libjinwo.lib` |
| Headers | all `.h` under `include/` |

> ⚠️ **Naming transition (2026-10)**: the `libjinwo.*` above are what the already
> published languages (C/C++/Go/Python/Android) use today. The cross-platform FFI
> base for **not-yet-published** languages (C#/PHP, etc.) is named **`libjinwo_vecdb`**,
> built and published by the CI workflow `.github/workflows/build_native_lib.yml`
> (artifacts `libjinwo_vecdb-<platform>.*` + `include/*.h`) for PHP(FFI) / C#(DllImport).
> All languages are planned to unify on `libjinwo_vecdb` in **0.1.35**.

---

## 2. General Build Flow

```bash
# 1) Get the source
git clone https://gitee.com/pkjinwo/jinwo_vector_db.git   # China mirror
cd jinwo_vector_db

# 2) Configure (generates Makefile / Ninja by default)
cmake -S . -B build

# 3) Build (parallel)
cmake --build build -j$(nproc)

# 4) Install library + headers to a prefix
cmake --install build --prefix /your/install/path
```

Installed layout:

```
<your-install-path>/
├── lib/
│   ├── libjinwo.so              # or .dylib / .dll (published C/C++ local dev lib)
│   └── libjinwo.a
└── include/
    ├── jw_vecdb.h
    ├── jw_types.h
    └── ... (other .h)
```

In your C/C++ project:

```c
#include <jw_vecdb.h>
```

and link with `-ljinwo` (shared) or `libjinwo.a` (static).

---

## 3. Platform-Specific Builds

### 3.1 Unix / Linux

The default generator is Makefile; just run the general flow:

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local
```

### 3.2 macOS

**Option A: Command line (Makefile / Ninja)**

```bash
# Build a universal x86_64 + arm64 binary (optional)
cmake -S . -B build -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build -j$(sysctl -n hw.ncpu)
sudo cmake --install build --prefix /usr/local
```

**Option B: Generate an Xcode project (compile / debug in GUI)**

```bash
cmake -G Xcode -S . -B build-xcode
open build-xcode/jinwo_vecdb.xcodeproj
```

Select the `jinwo` / `jinwo_static` target and Build / Run as usual in Xcode.

### 3.3 Windows (Visual Studio)

```bat
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release
cmake --install build --prefix C:\jinwo_vecdb
```

Or simply open `build\jinwo_vecdb.sln` in Visual Studio.
`libjinwo.lib` (import lib) and `libjinwo.dll` (dynamic) are produced under
`build\Release\`.

---

## 4. FAQ

- **`cmake ..` says "does not appear to contain CMakeLists.txt"**
  Older docs told you to `mkdir build && cd build && cmake ..`, but the
  `CMakeLists.txt` lives at the repository root. Use `cmake -S . -B build`
  from the root instead of `cmake ..` from inside `build/`.

- **Library name has a `lib` prefix**
  On Windows / Linux the artifact is `libjinwo.*` on purpose, to avoid a
  clash between `jinwo.so` and the same-named `jinwo.py` (see the
  `PREFIX "lib"` setting in `CMakeLists.txt`).

---

## 5. Notes for Maintainers / AI Tools (Sync Constraint)

The root `CMakeLists.txt` and `build_python_pypi/CMakeLists.txt` share the
`JINWO_SOURCES` list and compile definitions. Whenever you change **either**
file, you MUST sync the **other**, otherwise you get "Python builds but the C
library doesn't" (or vice versa):

1. Add / remove `src/*.c` (update `JINWO_SOURCES`);
2. Change `JW_EXPORTS` etc. in `target_compile_definitions`;
3. Change platform-specific handling (WIN32 MSVC subdir, `PREFIX "lib"`,
   APPLE `-undefined dynamic_lookup`).

Both files carry a prominent "sync reminder" comment block at the top —
please read it before editing.
