#!/usr/bin/env php
<?php
/**
 * Download the correct libjinwo native binary for this platform from the
 * GitHub release and extract it into this package's lib/ directory.
 *
 * Usage:  php bin/install.php
 * (or:    composer jinwo:install-lib   — when used as a dependency)
 */

$root = dirname(__DIR__);
$release = 'v0.1.34';

// allow \JinWoVecDB\JinWoVecDB::platformFile() used below (standalone or via composer)
$classFile = $root . '/src/JinWoVecDB.php';
if (file_exists($classFile)) {
    require_once $classFile;
}

$os = PHP_OS_FAMILY;
$machine = php_uname('m');

switch ($os) {
    case 'Darwin':
        $asset = 'libjinwo-darwin-universal.tar.gz';
        break;
    case 'Windows':
        $asset = 'libjinwo-windows-x86_64.tar.gz';
        break;
    case 'Linux':
        $asset = ($machine === 'aarch64' || $machine === 'arm64')
            ? 'libjinwo-linux-aarch64.tar.gz'
            : 'libjinwo-linux-x86_64.tar.gz';
        break;
    default:
        fwrite(STDERR, "Unsupported OS family: {$os}\n");
        exit(1);
}

$url = "https://github.com/pkjinwo/jinwo_vector_db/releases/download/{$release}/{$asset}";
$libDir = $root . '/lib';
@mkdir($libDir, 0777, true);

echo "Downloading {$url}\n";
$data = @file_get_contents($url);
if ($data === false) {
    fwrite(STDERR, "Failed to download {$asset}. Check network access and that the release asset exists.\n");
    exit(1);
}

$tmp = tempnam(sys_get_temp_dir(), 'jwlib') . '.tar.gz';
file_put_contents($tmp, $data);

$tmpDir = sys_get_temp_dir() . '/jwlib_' . uniqid('', true);
mkdir($tmpDir);

$ok = false;

// Prefer the system tar — it handles the archive's "." entries robustly
// (PharData::extractTo throws "Cannot extract '.', internal error" on them).
$tarBin = trim(@shell_exec('command -v tar 2>/dev/null') ?: '');
if ($tarBin !== '' && is_executable($tarBin)) {
    exec(escapeshellcmd($tarBin) . ' -xzf ' . escapeshellarg($tmp) . ' -C ' . escapeshellarg($tmpDir) . ' 2>/dev/null', $_, $rc);
    $ok = ($rc === 0);
}
// Fallback to PharData if tar is unavailable.
if (!$ok) {
    try {
        $phar = new PharData($tmp);
        $phar->extractTo($tmpDir);
        $ok = true;
    } catch (\Throwable $e) {
        fwrite(STDERR, "Extract failed: {$e->getMessage()}\n");
        @unlink($tmp);
        exit(1);
    }
}

// The tarball contains lib/ and include/ at its root; flatten lib/* into $libDir.
foreach (glob($tmpDir . '/lib/*') as $src) {
    if (is_file($src)) {
        copy($src, $libDir . '/' . basename($src));
    }
}

// cleanup (recursive, tolerant of nested dirs)
function jw_rmtree(string $dir): void
{
    if (!is_dir($dir)) return;
    foreach (scandir($dir) ?: [] as $it) {
        if ($it === '.' || $it === '..') continue;
        $p = $dir . '/' . $it;
        if (is_dir($p)) jw_rmtree($p); else @unlink($p);
    }
    @rmdir($dir);
}
jw_rmtree($tmpDir);
@unlink($tmp);

if (class_exists(\JinWoVecDB\JinWoVecDB::class)) {
    $expected = $libDir . '/' . \JinWoVecDB\JinWoVecDB::platformFile();
    if (file_exists($expected)) {
        echo "OK: extracted libjinwo into {$libDir}\n  -> {$expected}\n";
    } else {
        echo "WARNING: expected {$expected} not found after extraction. Contents of {$libDir}:\n";
        foreach (glob($libDir . '/*') as $f) {
            echo "  " . basename($f) . "\n";
        }
    }
} else {
    echo "OK: extracted files into {$libDir}\n";
}
