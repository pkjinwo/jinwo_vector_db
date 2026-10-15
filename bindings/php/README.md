# jinwo/vecdb — PHP binding (FFI)

PHP wrapper for **JinWo VecDB**, an embedded vector database. It talks to the
cross-platform native library **`libjinwo`** through PHP-FFI (no C extension
to compile).

> The native lib name is **`libjinwo`** everywhere (no second name). The PHP
> package name `jinwo/vecdb` is just the Composer identifier — not the lib name.

## Requirements

- PHP >= 8.0
- `ext-ffi` enabled (`php.ini`: `extension=ffi` and `ffi.enable=true`)
- the `libjinwo` shared library for your platform

## Install

```bash
composer require jinwo/vecdb
php vendor/jinwo/vecdb/bin/install.php   # downloads libjinwo into the package lib/
```

`bin/install.php` picks the right artifact from the GitHub release `v0.1.34`
(`libjinwo-linux-x86_64.so` / `libjinwo-linux-aarch64.so` /
`libjinwo-darwin-universal.dylib` / `libjinwo-windows-x86_64.dll`) and extracts
it locally — no `apt`/`brew` needed.

Alternatively, point the loader at an existing lib:

```bash
export JINWO_VECDB_LIB=/usr/local/lib/libjinwo.so
```

## Usage

```php
use JinWoVecDB\JinWoVecDB;

echo JinWoVecDB::version() . PHP_EOL;

$db = new JinWoVecDB('docs.jwv', JinWoVecDB::CREATE | JinWoVecDB::READWRITE);
$db->createCollection('docs', 4);
$vid = $db->insert('docs', [0.1, 0.2, 0.3, 0.4]);
$results = $db->search('docs', [0.1, 0.2, 0.3, 0.4], 5);
print_r($results);          // [{id, score, dimension}, ...]
$db->close();
```

## How it works

`src/JinWoVecDB.php` builds an FFI `cdef` from the real C API in
`include/jw_vecdb.h` and loads `libjinwo` (resolved via `JINWO_VECDB_LIB`,
then the packaged `lib/`, then the system loader). Vectors are passed as
`float[]`; results come back as `jw_search_result_t` structs read for `id` /
`score` / `dimension`.
