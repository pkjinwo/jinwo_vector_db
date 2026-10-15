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

try {
    $phar = new PharData($tmp);
    $phar->extractTo($tmpDir);
    // The tarball contains lib/ and include/ at its root; flatten lib/* into $libDir.
    foreach (glob($tmpDir . '/lib/*') as $src) {
        if (is_file($src)) {
            copy($src, $libDir . '/' . basename($src));
        }
    }
} catch (\Throwable $e) {
    fwrite(STDERR, "Extract failed: {$e->getMessage()}\n");
    @unlink($tmp);
    exit(1);
}

// cleanup
array_map('unlink', glob($tmpDir . '/lib/*'));
@rmdir($tmpDir . '/lib');
@rmdir($tmpDir . '/include');
@rmdir($tmpDir);
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
