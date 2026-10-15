<?php
/**
 * JinWoVecDB PHP FFI binding.
 *
 * Loads the cross-platform shared library `libjinwo` (produced by the
 * `build_native_lib.yml` CI and published to the GitHub release) via PHP-FFI
 * and wraps the C convenience API.
 *
 * Prerequisites:
 *   - PHP >= 8.0 with the `ffi` extension enabled (php.ini: `ffi.enable=true`)
 *   - the native lib present (run `php bin/install.php`, or set
 *     JINWO_VECDB_LIB to an explicit path / put libjinwo on the system loader path)
 */

namespace JinWoVecDB;

final class JinWoVecDB
{
    /* jw_vecdb_flag_t bits (from jw_vecdb.h) */
    public const READONLY   = 0x01;
    public const READWRITE  = 0x02;
    public const CREATE     = 0x04;
    public const TRUNCATE   = 0x08;
    public const MEMORY     = 0x10;

    private static ?\FFI $ffi = null;

    /** @var \FFI\CData|null underlying jw_vecdb_t* handle */
    private ?object $db = null;

    /** C string buffers kept alive across the native call that uses them */
    private array $hold = [];

    private const CDEF = <<<'C'
typedef int jw_bool_t;
typedef int jw_status_t;
typedef unsigned long long jw_size_t;
typedef unsigned int jw_uint32_t;
typedef unsigned long long jw_uint64_t;
typedef float jw_score_t;
typedef unsigned long long jw_vec_id_t;
typedef unsigned long long jw_vid_t;
typedef unsigned int jw_dim_t;
typedef const float *jw_cvec_t;

typedef struct jw_str_t {
    char *ptr;
    jw_size_t slen;
} jw_str_t;

typedef struct jw_vecdb_t jw_vecdb_t;
typedef struct jw_collection_t jw_collection_t;

typedef struct jw_search_result {
    jw_vec_id_t id;
    jw_score_t score;
    jw_cvec_t vector;
    jw_size_t dimension;
    void *metadata;
    jw_size_t metadata_size;
} jw_search_result_t;

jw_str_t jw_vecdb_version(void);
jw_status_t jw_vecdb_open(const jw_str_t *path, jw_uint32_t flags, jw_vecdb_t **db);
jw_status_t jw_vecdb_close(jw_vecdb_t *db);
jw_status_t jw_vecdb_create_collection(jw_vecdb_t *db, const jw_str_t *name, jw_dim_t dim, jw_collection_t **coll);
jw_status_t jw_vecdb_insert(jw_vecdb_t *db, const jw_str_t *coll_name, jw_cvec_t vec, jw_dim_t dim, jw_vid_t *vid);
jw_size_t jw_vecdb_search(jw_vecdb_t *db, const jw_str_t *coll_name, jw_cvec_t query, jw_dim_t dim, jw_size_t k, jw_search_result_t *results);
jw_size_t jw_vecdb_list_collections(const jw_vecdb_t *db, jw_str_t *names, jw_size_t capacity);
const char *jw_vecdb_strerror(jw_status_t status);
C;

    /**
     * Library version string (e.g. "JinWo VecDB 0.1.34").
     */
    public static function version(): string
    {
        $ffi = self::ffi();
        $v = $ffi->jw_vecdb_version();
        return \FFI::string($v->ptr, $v->slen);
    }

    /**
     * Resolve the libjinwo binary path.
     * Priority: explicit arg > JINWO_VECDB_LIB env > packaged lib/ dir > system loader.
     */
    public static function resolveLibPath(?string $explicit = null): string
    {
        if ($explicit !== null) {
            return $explicit;
        }
        $env = getenv('JINWO_VECDB_LIB');
        if ($env !== false && $env !== '') {
            return $env;
        }
        $file = dirname(__DIR__) . '/lib/' . self::platformFile();
        if (file_exists($file)) {
            return $file;
        }
        // last resort: hope libjinwo is on the system loader path
        return 'libjinwo';
    }

    /**
     * Native artifact filename for the current platform.
     */
    public static function platformFile(): string
    {
        $os = PHP_OS_FAMILY;
        $machine = php_uname('m');
        if ($os === 'Darwin') {
            return 'libjinwo-darwin-universal.dylib';
        }
        if ($os === 'Windows') {
            return 'libjinwo-windows-x86_64.dll';
        }
        if ($os === 'Linux') {
            return ($machine === 'aarch64' || $machine === 'arm64')
                ? 'libjinwo-linux-aarch64.so'
                : 'libjinwo-linux-x86_64.so';
        }
        throw new \RuntimeException("Unsupported OS family: {$os}");
    }

    private static function ffi(): \FFI
    {
        if (self::$ffi === null) {
            if (!extension_loaded('ffi')) {
                throw new \RuntimeException(
                    'The PHP FFI extension is required. Enable it in php.ini (extension=ffi, ffi.enable=true).'
                );
            }
            self::$ffi = \FFI::cdef(self::CDEF, self::resolveLibPath());
        }
        return self::$ffi;
    }

    /**
     * Open (or create) a database.
     *
     * @param string $path  Database file path; empty string => in-memory database.
     * @param int    $flags Bitwise combination of the class constants (default CREATE|READWRITE).
     */
    public function __construct(string $path = '', int $flags = self::CREATE | self::READWRITE)
    {
        $ffi = self::ffi();
        $dbp = $ffi->new('jw_vecdb_t*');
        $st = $ffi->jw_vecdb_open($this->str($path), $flags, \FFI::addr($dbp));
        if ($st !== 0) {
            throw new \RuntimeException('jw_vecdb_open failed: ' . $this->err($st));
        }
        $this->db = $dbp;
        $this->hold = [];
    }

    /**
     * Create a collection (auto-created on insert too, but explicit is clearer).
     */
    public function createCollection(string $name, int $dim): void
    {
        $ffi = self::ffi();
        $coll = $ffi->new('jw_collection_t*');
        $st = $ffi->jw_vecdb_create_collection($this->db, $this->str($name), $dim, \FFI::addr($coll));
        if ($st !== 0) {
            throw new \RuntimeException("create_collection('{$name}') failed: " . $this->err($st));
        }
        $this->hold = [];
    }

    /**
     * Insert a vector; returns the new vector id.
     *
     * @param string $collection Collection name.
     * @param float[] $vec       Vector values.
     */
    public function insert(string $collection, array $vec): int
    {
        $ffi = self::ffi();
        $dim = count($vec);
        $vid = $ffi->new('jw_vid_t');
        $st = $ffi->jw_vecdb_insert($this->db, $this->str($collection), $this->vec($vec, $dim), $dim, \FFI::addr($vid));
        if ($st !== 0) {
            throw new \RuntimeException("insert failed: " . $this->err($st));
        }
        $this->hold = [];
        return (int) $vid->cdata;
    }

    /**
     * Search a collection for the k nearest neighbours.
     *
     * @param string  $collection Collection name.
     * @param float[] $query     Query vector.
     * @param int     $k         Max number of results.
     * @return array<int,array{id:string,score:float,dimension:int}>
     */
    public function search(string $collection, array $query, int $k): array
    {
        $ffi = self::ffi();
        $dim = count($query);
        $out = $ffi->new("jw_search_result_t[{$k}]");
        $n = $ffi->jw_vecdb_search($this->db, $this->str($collection), $this->vec($query, $dim), $dim, $k, $out);
        $res = [];
        for ($i = 0; $i < $n; $i++) {
            $r = $out[$i];
            $res[] = [
                'id'        => (string) $r->id,
                'score'     => $r->score,
                'dimension' => (int) $r->dimension,
            ];
        }
        $this->hold = [];
        return $res;
    }

    /**
     * List all collection names.
     *
     * @param int $capacity Max collections to return.
     * @return string[]
     */
    public function listCollections(int $capacity = 64): array
    {
        $ffi = self::ffi();
        $names = $ffi->new("jw_str_t[{$capacity}]");
        $n = $ffi->jw_vecdb_list_collections($this->db, $names, $capacity);
        $list = [];
        for ($i = 0; $i < $n; $i++) {
            $list[] = \FFI::string($names[$i]->ptr, $names[$i]->slen);
        }
        $this->hold = [];
        return $list;
    }

    public function close(): void
    {
        if ($this->db !== null) {
            self::ffi()->jw_vecdb_close($this->db);
            $this->db = null;
        }
    }

    public function __destruct()
    {
        $this->close();
    }

    private function str(string $s): object
    {
        $ffi = self::ffi();
        $n = strlen($s);
        $buf = $ffi->new("char[" . ($n + 1) . "]");
        if ($n > 0) {
            \FFI::memcpy($buf, $s, $n);
        }
        $buf[$n] = "\0";
        $j = $ffi->new('jw_str_t');
        $j->ptr = \FFI::addr($buf[0]); // char* pointer (not an owned array) for the char* field
        $j->slen = $n;
        // keep both the buffer and the struct alive during the native call
        $this->hold[] = $buf;
        $this->hold[] = $j;
        return \FFI::addr($j); // jw_str_t* — matches the C API's const jw_str_t * parameter
    }

    /**
     * @param float[] $v
     */
    private function vec(array $v, int $dim): object
    {
        $c = self::ffi()->new("float[{$dim}]");
        foreach ($v as $i => $x) {
            $c[$i] = (float) $x;
        }
        return $c;
    }

    private function err(int $st): string
    {
        $msg = self::ffi()->jw_vecdb_strerror($st);
        return ($msg !== null) ? \FFI::string($msg) : "status={$st}";
    }
}
