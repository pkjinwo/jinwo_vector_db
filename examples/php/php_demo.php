<?php
/**
 * JinWo VecDB PHP Demo (uses the jinwo/vecdb FFI binding).
 *
 * Prerequisites:
 *   - PHP >= 8.0 with ext-ffi enabled (php.ini: extension=ffi, ffi.enable=true)
 *   - fetch the native lib once:  php ../../bindings/php/bin/install.php
 *     (or set JINWO_VECDB_LIB to an existing libjinwo path)
 *
 * Run:  php php_demo.php
 */

$autoload = __DIR__ . '/../../bindings/php/vendor/autoload.php';
if (file_exists($autoload)) {
    require $autoload;
} else {
    require __DIR__ . '/../../bindings/php/src/JinWoVecDB.php';
}

use JinWoVecDB\JinWoVecDB;

function generateRandomVector(int $dim): array
{
    $v = [];
    for ($i = 0; $i < $dim; $i++) {
        $v[] = (float) (mt_rand() / mt_getrandmax() * 2 - 1);
    }
    return $v;
}

echo "========================================\n";
echo "  JinWo VecDB PHP Demo\n";
echo "========================================\n\n";

echo "Version: " . JinWoVecDB::version() . "\n\n";

$db = new JinWoVecDB('./vecdb_php', JinWoVecDB::CREATE | JinWoVecDB::READWRITE);
echo "Database opened successfully\n\n";

$dim = 128;
$db->createCollection('test', $dim);
echo "Collection 'test' (dim=$dim) created\n\n";

echo "Inserting 10 vectors...\n";
for ($i = 0; $i < 10; $i++) {
    $vid = $db->insert('test', generateRandomVector($dim));
    echo "  inserted vector $i, id=$vid\n";
}
echo "Insertion completed\n\n";

echo "Searching for similar vectors...\n";
$query = generateRandomVector($dim);
$results = $db->search('test', $query, 5);
if (count($results) > 0) {
    echo "Top " . count($results) . " results:\n";
    foreach ($results as $r) {
        echo "  id={$r['id']} score={$r['score']} dim={$r['dimension']}\n";
    }
} else {
    echo "No results\n";
}
echo "\n";

echo "Collections: " . implode(', ', $db->listCollections()) . "\n\n";
$db->close();
echo "Demo completed successfully!\n";
