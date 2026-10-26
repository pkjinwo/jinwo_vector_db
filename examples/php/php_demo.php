<?php
/**
 * JinWo VecDB PHP Demo
 *
 * This script demonstrates how to use JinWo VecDB with PHP
 * using FFI to call the C API.
 */

// Check if FFI extension is available
if (!extension_loaded('ffi')) {
    die('Error: PHP FFI extension is not available. Please enable it in php.ini.');
}

// Define library path
$libraryPath = __DIR__ . '/../../build/libjw_vecdb.so';
if (!file_exists($libraryPath)) {
    // Try Windows DLL
    $libraryPath = __DIR__ . '/../../build/jw_vecdb.dll';
    if (!file_exists($libraryPath)) {
        die('Error: Could not find JinWo VecDB library. Please build the library first.');
    }
}

// Load the library
$ffi = FFI::cdef('    
    // Database operations
    int jw_vecdb_open(void **db, const char *path, int create);
    int jw_vecdb_close(void *db);
    
    // Version information
    const char *jw_version();
    
    // Collection operations
    int jw_collection_create(void **collection, void *db, const char *name, int dimension);
    int jw_collection_close(void *collection);
    int jw_collection_list(void *db, char ***names, size_t *count);
    
    // Vector operations
    int jw_collection_insert(void *collection, const float *vector, uint64_t *id);
    int jw_collection_search(void *collection, const float *query, size_t k, struct jw_search_result_t **results, size_t *count);
    
    // Search result structure
    struct jw_search_result_t {
        uint64_t id;
        float distance;
    };
    
    // Free memory
    void free(void *ptr);
', $libraryPath);

// Error handling class
class VecDBError extends Exception {
    private $code;
    
    public function __construct($code) {
        $this->code = $code;
        $message = $this->getErrorMessage($code);
        parent::__construct($message, $code);
    }
    
    private function getErrorMessage($code) {
        $errorMessages = [
            0 => 'Success',
            -1 => 'Invalid parameter',
            -2 => 'Out of memory',
            -3 => 'File system error',
            -4 => 'Collection already exists',
            -5 => 'Collection does not exist',
            -6 => 'Vector does not exist',
            -7 => 'Index creation failed',
            -8 => 'Dimension mismatch',
            -9 => 'Internal error'
        ];
        return $errorMessages[$code] ?? "Unknown error: $code";
    }
}

// VecDB class
class VecDB {
    private $db;
    private $ffi;
    
    public function __construct($path, $create = true, $ffi) {
        $this->ffi = $ffi;
        $dbPtr = FFI::new('void*');
        $result = $this->ffi->jw_vecdb_open(FFI::addr($dbPtr), $path, $create ? 1 : 0);
        if ($result !== 0) {
            throw new VecDBError($result);
        }
        $this->db = $dbPtr;
    }
    
    public function __destruct() {
        if ($this->db) {
            $this->ffi->jw_vecdb_close($this->db);
        }
    }
    
    public static function getVersion($ffi) {
        $versionPtr = $ffi->jw_version();
        return $ffi->string($versionPtr);
    }
    
    public function createCollection($name, $dimension) {
        $colPtr = FFI::new('void*');
        $result = $this->ffi->jw_collection_create(FFI::addr($colPtr), $this->db, $name, $dimension);
        if ($result !== 0) {
            throw new VecDBError($result);
        }
        return new Collection($colPtr, $this->ffi);
    }
    
    public function listCollections() {
        $namesPtr = FFI::new('char**');
        $count = FFI::new('size_t');
        $result = $this->ffi->jw_collection_list($this->db, FFI::addr($namesPtr), FFI::addr($count));
        if ($result !== 0) {
            throw new VecDBError($result);
        }
        
        $collections = [];
        for ($i = 0; $i < $count->cdata; $i++) {
            $namePtr = $namesPtr[$i];
            if ($namePtr) {
                $collections[] = $this->ffi->string($namePtr);
                // Free the string
                $this->ffi->free($namePtr);
            }
        }
        
        // Free the array
        $this->ffi->free($namesPtr);
        
        return $collections;
    }
}

// Collection class
class Collection {
    private $col;
    private $ffi;
    
    public function __construct($colPtr, $ffi) {
        $this->col = $colPtr;
        $this->ffi = $ffi;
    }
    
    public function __destruct() {
        if ($this->col) {
            $this->ffi->jw_collection_close($this->col);
        }
    }
    
    public function insert($vector) {
        // Create a float array
        $vectorArray = FFI::new('float[' . count($vector) . ']');
        foreach ($vector as $i => $value) {
            $vectorArray[$i] = $value;
        }
        
        $id = FFI::new('uint64_t');
        $result = $this->ffi->jw_collection_insert($this->col, $vectorArray, FFI::addr($id));
        if ($result !== 0) {
            throw new VecDBError($result);
        }
        
        return $id->cdata;
    }
    
    public function search($query, $k) {
        // Create a float array for query
        $queryArray = FFI::new('float[' . count($query) . ']');
        foreach ($query as $i => $value) {
            $queryArray[$i] = $value;
        }
        
        $resultsPtr = FFI::new('struct jw_search_result_t*');
        $count = FFI::new('size_t');
        
        $result = $this->ffi->jw_collection_search($this->col, $queryArray, $k, FFI::addr($resultsPtr), FFI::addr($count));
        if ($result !== 0) {
            throw new VecDBError($result);
        }
        
        $searchResults = [];
        for ($i = 0; $i < $count->cdata; $i++) {
            $result = $resultsPtr[$i];
            $searchResults[] = [
                'id' => $result->id,
                'distance' => $result->distance
            ];
        }
        
        // Free the results
        $this->ffi->free($resultsPtr);
        
        return $searchResults;
    }
}

// Helper function to generate random vector
function generateRandomVector($dimension) {
    $vector = [];
    for ($i = 0; $i < $dimension; $i++) {
        $vector[] = (float)(mt_rand() / mt_getrandmax() * 2 - 1); // Random value between -1 and 1
    }
    return $vector;
}

// Main demo function
function main() {
    global $ffi;
    
    echo "========================================\n";
    echo "  JinWo VecDB PHP Demo\n";
    echo "========================================\n\n";
    
    try {
        // Get version
        $version = VecDB::getVersion($ffi);
        echo "Version: $version\n\n";
        
        // Create database
        $dbPath = './vecdb_php';
        echo "Opening database at: $dbPath\n";
        $db = new VecDB($dbPath, true, $ffi);
        echo "Database opened successfully\n\n";
        
        // Create collection
        echo "Creating collection...\n";
        $collection = $db->createCollection('test', 128);
        echo "Collection created successfully: test\n\n";
        
        // Insert vectors
        echo "Inserting 10 vectors...\n";
        for ($i = 0; $i < 10; $i++) {
            $vector = generateRandomVector(128);
            $vectorId = $collection->insert($vector);
            echo "Inserted vector $i successfully, ID: $vectorId\n";
        }
        echo "Insertion completed\n\n";
        
        // Search
        echo "Searching for similar vectors...\n";
        $query = generateRandomVector(128);
        $results = $collection->search($query, 5);
        
        if (count($results) > 0) {
            echo "Search results (top " . count($results) . "):\n";
            foreach ($results as $i => $result) {
                echo "ID: {$result['id']}, Distance: " . number_format($result['distance'], 4) . "\n";
            }
        } else {
            echo "Search failed, no results\n";
        }
        echo "\n";
        
        // List collections
        echo "Listing all collections...\n";
        $collections = $db->listCollections();
        echo "Found " . count($collections) . " collections:\n";
        foreach ($collections as $name) {
            echo "  $name\n";
        }
        echo "\n";
        
        echo "Demo completed successfully!\n";
        
    } catch (VecDBError $e) {
        echo "Error: {$e->getMessage()}\n";
    } catch (Exception $e) {
        echo "Unexpected error: {$e->getMessage()}\n";
    }
}

// Run the demo
main();
