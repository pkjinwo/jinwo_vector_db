const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');
const path = require('path');

// Define types
const voidPtr = ref.types.void;
const voidPtrPtr = ref.refType(voidPtr);
const charPtr = ref.types.CString;
const charPtrPtr = ref.refType(charPtr);
const int = ref.types.int;
const size_t = ref.types.size_t;
const uint64 = ref.types.uint64;
const float = ref.types.float;
const floatPtr = ref.refType(float);

// Define search result structure
const JWSearchResult = Struct({
  id: uint64,
  distance: float
});
const JWSearchResultPtr = ref.refType(JWSearchResult);
const JWSearchResultPtrPtr = ref.refType(JWSearchResultPtr);

// Load the library
let libraryPath;
if (process.platform === 'win32') {
  libraryPath = path.join(__dirname, '../../build/jw_vecdb.dll');
} else if (process.platform === 'darwin') {
  libraryPath = path.join(__dirname, '../../build/libjw_vecdb.dylib');
} else {
  libraryPath = path.join(__dirname, '../../build/libjw_vecdb.so');
}

const jw_vecdb = ffi.Library(libraryPath, {
  'jw_vecdb_open': [int, [voidPtrPtr, charPtr, int]],
  'jw_vecdb_close': [int, [voidPtr]],
  'jw_version': [charPtr, []],
  'jw_collection_create': [int, [voidPtrPtr, voidPtr, charPtr, int]],
  'jw_collection_close': [int, [voidPtr]],
  'jw_collection_list': [int, [voidPtr, charPtrPtr, ref.refType(size_t)]],
  'jw_collection_insert': [int, [voidPtr, floatPtr, ref.refType(uint64)]],
  'jw_collection_search': [int, [voidPtr, floatPtr, size_t, JWSearchResultPtrPtr, ref.refType(size_t)]]
});

// Error handling
class VecDBError extends Error {
  constructor(code) {
    const message = getErrorMessage(code);
    super(message);
    this.code = code;
  }
}

function getErrorMessage(code) {
  const errorMessages = {
    0: 'Success',
    -1: 'Invalid parameter',
    -2: 'Out of memory',
    -3: 'File system error',
    -4: 'Collection already exists',
    -5: 'Collection does not exist',
    -6: 'Vector does not exist',
    -7: 'Index creation failed',
    -8: 'Dimension mismatch',
    -9: 'Internal error'
  };
  return errorMessages[code] || `Unknown error: ${code}`;
}

// VecDB class
class VecDB {
  constructor(dbPtr) {
    this.dbPtr = dbPtr;
  }

  static getVersion() {
    const versionPtr = jw_vecdb.jw_version();
    return ref.readCString(versionPtr);
  }

  static async open(path, create = true) {
    const dbPtr = ref.alloc(voidPtr);
    const result = jw_vecdb.jw_vecdb_open(dbPtr, path, create ? 1 : 0);
    
    if (result !== 0) {
      throw new VecDBError(result);
    }

    return new VecDB(dbPtr.deref());
  }

  close() {
    if (this.dbPtr) {
      const result = jw_vecdb.jw_vecdb_close(this.dbPtr);
      if (result !== 0) {
        throw new VecDBError(result);
      }
      this.dbPtr = null;
    }
  }

  async createCollection(name, dimension) {
    const colPtr = ref.alloc(voidPtr);
    const result = jw_vecdb.jw_collection_create(colPtr, this.dbPtr, name, dimension);
    
    if (result !== 0) {
      throw new VecDBError(result);
    }

    return new Collection(colPtr.deref());
  }

  async listCollections() {
    const namesPtr = ref.alloc(charPtrPtr);
    const count = ref.alloc(size_t);
    
    const result = jw_vecdb.jw_collection_list(this.dbPtr, namesPtr, count);
    if (result !== 0) {
      throw new VecDBError(result);
    }

    const collectionCount = count.deref();
    const collections = [];
    
    const names = namesPtr.deref();
    for (let i = 0; i < collectionCount; i++) {
      const namePtr = ref.readPointer(names, i * ref.sizeof.pointer);
      collections.push(ref.readCString(namePtr));
      // Free the string
      ffi.C.free(namePtr);
    }
    
    // Free the array
    ffi.C.free(names);
    
    return collections;
  }
}

// Collection class
class Collection {
  constructor(colPtr) {
    this.colPtr = colPtr;
  }

  close() {
    if (this.colPtr) {
      const result = jw_vecdb.jw_collection_close(this.colPtr);
      if (result !== 0) {
        throw new VecDBError(result);
      }
      this.colPtr = null;
    }
  }

  async insert(vector) {
    // Create a buffer for the vector
    const vectorBuffer = Buffer.alloc(vector.length * ref.sizeof.float);
    for (let i = 0; i < vector.length; i++) {
      vectorBuffer.writeFloatLE(vector[i], i * ref.sizeof.float);
    }
    
    const id = ref.alloc(uint64);
    const result = jw_vecdb.jw_collection_insert(this.colPtr, vectorBuffer, id);
    
    if (result !== 0) {
      throw new VecDBError(result);
    }
    
    return id.deref();
  }

  async search(query, k) {
    // Create a buffer for the query vector
    const queryBuffer = Buffer.alloc(query.length * ref.sizeof.float);
    for (let i = 0; i < query.length; i++) {
      queryBuffer.writeFloatLE(query[i], i * ref.sizeof.float);
    }
    
    const resultsPtr = ref.alloc(JWSearchResultPtr);
    const count = ref.alloc(size_t);
    
    const result = jw_vecdb.jw_collection_search(this.colPtr, queryBuffer, k, resultsPtr, count);
    if (result !== 0) {
      throw new VecDBError(result);
    }

    const searchCount = count.deref();
    const searchResults = [];
    
    const results = resultsPtr.deref();
    for (let i = 0; i < searchCount; i++) {
      const result = JWSearchResult.get(results, i * JWSearchResult.size);
      searchResults.push({
        id: result.id,
        distance: result.distance
      });
    }
    
    // Free the results
    ffi.C.free(results);
    
    return searchResults;
  }
}

// Helper function to generate random vector
function generateRandomVector(dimension) {
  const vector = [];
  for (let i = 0; i < dimension; i++) {
    vector.push(Math.random() * 2 - 1); // Random value between -1 and 1
  }
  return vector;
}

// Main demo function
async function main() {
  console.log('========================================');
  console.log('  JinWo VecDB Node.js Demo');
  console.log('========================================');
  console.log();
  
  try {
    // Get version
    const version = VecDB.getVersion();
    console.log(`Version: ${version}`);
    console.log();
    
    // Create database
    const dbPath = './vecdb_nodejs';
    console.log(`Opening database at: ${dbPath}`);
    const db = await VecDB.open(dbPath, true);
    console.log('Database opened successfully');
    console.log();
    
    // Create collection
    console.log('Creating collection...');
    const collection = await db.createCollection('test', 128);
    console.log('Collection created successfully: test');
    console.log();
    
    // Insert vectors
    console.log('Inserting 10 vectors...');
    for (let i = 0; i < 10; i++) {
      const vector = generateRandomVector(128);
      const vectorId = await collection.insert(vector);
      console.log(`Inserted vector ${i} successfully, ID: ${vectorId}`);
    }
    console.log('Insertion completed');
    console.log();
    
    // Search
    console.log('Searching for similar vectors...');
    const query = generateRandomVector(128);
    const results = await collection.search(query, 5);
    
    if (results.length > 0) {
      console.log(`Search results (top ${results.length}):`);
      results.forEach((result, index) => {
        console.log(`ID: ${result.id}, Distance: ${result.distance.toFixed(4)}`);
      });
    } else {
      console.log('Search failed, no results');
    }
    console.log();
    
    // List collections
    console.log('Listing all collections...');
    const collections = await db.listCollections();
    console.log(`Found ${collections.length} collections:`);
    collections.forEach(name => {
      console.log(`  ${name}`);
    });
    console.log();
    
    // Cleanup
    collection.close();
    db.close();
    
    console.log('Demo completed successfully!');
    
  } catch (error) {
    console.error(`Error: ${error.message}`);
  }
}

// Run the demo
main();
