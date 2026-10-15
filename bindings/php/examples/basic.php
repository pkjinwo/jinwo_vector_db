<?php
/**
 * Basic usage example for jinwo/vecdb (PHP FFI binding).
 *
 * 1) composer require jinwo/vecdb
 * 2) php bin/install.php        # fetch libjinwo for this platform
 * 3) php examples/basic.php
 *
 * (If not installed via Composer, run from the package dir:
 *    php -d ffi.enable=true examples/basic.php
 *  and ensure libjinwo was fetched with bin/install.php.)
 */

$autoload = __DIR__ . '/../vendor/autoload.php';
if (file_exists($autoload)) {
    require $autoload;
} else {
    require __DIR__ . '/../src/JinWoVecDB.php';
}

use JinWoVecDB\JinWoVecDB;

echo "JinWo VecDB version: " . JinWoVecDB::version() . PHP_EOL;

$db = new JinWoVecDB('php_demo.jwv', JinWoVecDB::CREATE | JinWoVecDB::READWRITE);
$db->createCollection('docs', 4);

$vid = $db->insert('docs', [0.1, 0.2, 0.3, 0.4]);
echo "inserted vid={$vid}" . PHP_EOL;

$results = $db->search('docs', [0.1, 0.2, 0.3, 0.4], 5);
echo "top results:" . PHP_EOL;
foreach ($results as $r) {
    echo "  id={$r['id']} score={$r['score']} dim={$r['dimension']}" . PHP_EOL;
}

echo "collections: " . implode(', ', $db->listCollections()) . PHP_EOL;
$db->close();
echo "OK" . PHP_EOL;
