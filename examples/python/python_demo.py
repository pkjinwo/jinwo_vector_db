#!/usr/bin/env python3
"""
JinWo VecDB Python Demo

This script demonstrates how to use JinWo VecDB with Python
using ctypes to call the C API.
"""

import ctypes
import os
import random
from ctypes import c_void_p, c_char_p, c_int, c_size_t, c_uint64, c_float, byref

# Load the JinWo VecDB library
library_path = os.path.join(os.path.dirname(__file__), '../../build/libjw_vecdb.so')
try:
    jw_vecdb = ctypes.CDLL(library_path)
except OSError:
    # Try Windows DLL
    library_path = os.path.join(os.path.dirname(__file__), '../../build/jw_vecdb.dll')
    try:
        jw_vecdb = ctypes.CDLL(library_path)
    except OSError:
        print(f"Error: Could not load JinWo VecDB library. Please build the library first.")
        exit(1)

# Define function prototypes
jw_vecdb.jw_vecdb_open.argtypes = [ctypes.POINTER(c_void_p), c_char_p, c_int]
jw_vecdb.jw_vecdb_open.restype = c_int

jw_vecdb.jw_vecdb_close.argtypes = [c_void_p]
jw_vecdb.jw_vecdb_close.restype = c_int

jw_vecdb.jw_version.argtypes = []
jw_vecdb.jw_version.restype = c_char_p

jw_vecdb.jw_collection_create.argtypes = [ctypes.POINTER(c_void_p), c_void_p, c_char_p, c_int]
jw_vecdb.jw_collection_create.restype = c_int

jw_vecdb.jw_collection_close.argtypes = [c_void_p]
jw_vecdb.jw_collection_close.restype = c_int

jw_vecdb.jw_collection_list.argtypes = [c_void_p, ctypes.POINTER(ctypes.POINTER(c_char_p)), ctypes.POINTER(c_size_t)]
jw_vecdb.jw_collection_list.restype = c_int

jw_vecdb.jw_collection_insert.argtypes = [c_void_p, ctypes.POINTER(c_float), ctypes.POINTER(c_uint64)]
jw_vecdb.jw_collection_insert.restype = c_int

jw_vecdb.jw_collection_search.argtypes = [c_void_p, ctypes.POINTER(c_float), c_size_t, ctypes.POINTER(ctypes.POINTER(c_void_p)), ctypes.POINTER(c_size_t)]
jw_vecdb.jw_collection_search.restype = c_int

# Define search result structure
class JWSearchResult(ctypes.Structure):
    _fields_ = [
        ('id', c_uint64),
        ('distance', c_float)
    ]

# Set search result pointer type
SearchResultPtr = ctypes.POINTER(JWSearchResult)

class VecDBError(Exception):
    """VecDB error exception"""
    def __init__(self, code):
        self.code = code
        self.message = self._get_error_message(code)
        super().__init__(self.message)
    
    def _get_error_message(self, code):
        error_messages = {
            0: "Success",
            -1: "Invalid parameter",
            -2: "Out of memory",
            -3: "File system error",
            -4: "Collection already exists",
            -5: "Collection does not exist",
            -6: "Vector does not exist",
            -7: "Index creation failed",
            -8: "Dimension mismatch",
            -9: "Internal error"
        }
        return error_messages.get(code, f"Unknown error: {code}")

class VecDB:
    """JinWo VecDB wrapper class"""
    
    def __init__(self, path, create=True):
        """Open or create a database"""
        self.db_ptr = c_void_p()
        result = jw_vecdb.jw_vecdb_open(byref(self.db_ptr), path.encode('utf-8'), 1 if create else 0)
        if result != 0:
            raise VecDBError(result)
    
    def __del__(self):
        """Close the database"""
        if hasattr(self, 'db_ptr') and self.db_ptr:
            jw_vecdb.jw_vecdb_close(self.db_ptr)
    
    @staticmethod
    def get_version():
        """Get JinWo VecDB version"""
        version_ptr = jw_vecdb.jw_version()
        return version_ptr.decode('utf-8')
    
    def create_collection(self, name, dimension):
        """Create a collection"""
        collection_ptr = c_void_p()
        result = jw_vecdb.jw_collection_create(
            byref(collection_ptr),
            self.db_ptr,
            name.encode('utf-8'),
            dimension
        )
        if result != 0:
            raise VecDBError(result)
        return Collection(collection_ptr)
    
    def list_collections(self):
        """List all collections"""
        names_ptr = ctypes.POINTER(c_char_p)()
        count = c_size_t(0)
        result = jw_vecdb.jw_collection_list(
            self.db_ptr,
            byref(names_ptr),
            byref(count)
        )
        if result != 0:
            raise VecDBError(result)
        
        collections = []
        for i in range(count.value):
            name_ptr = names_ptr[i]
            if name_ptr:
                collections.append(name_ptr.decode('utf-8'))
        
        # Free the memory
        for i in range(count.value):
            if names_ptr[i]:
                ctypes.free(names_ptr[i])
        ctypes.free(names_ptr)
        
        return collections

class Collection:
    """Collection wrapper class"""
    
    def __init__(self, collection_ptr):
        """Initialize collection"""
        self.collection_ptr = collection_ptr
    
    def __del__(self):
        """Close the collection"""
        if hasattr(self, 'collection_ptr') and self.collection_ptr:
            jw_vecdb.jw_collection_close(self.collection_ptr)
    
    def insert(self, vector):
        """Insert a vector"""
        vector_array = (c_float * len(vector))(*vector)
        vector_ptr = ctypes.cast(vector_array, ctypes.POINTER(c_float))
        vector_id = c_uint64(0)
        
        result = jw_vecdb.jw_collection_insert(
            self.collection_ptr,
            vector_ptr,
            byref(vector_id)
        )
        if result != 0:
            raise VecDBError(result)
        
        return vector_id.value
    
    def search(self, query, k):
        """Search for similar vectors"""
        query_array = (c_float * len(query))(*query)
        query_ptr = ctypes.cast(query_array, ctypes.POINTER(c_float))
        
        results_ptr = SearchResultPtr()
        count = c_size_t(0)
        
        result = jw_vecdb.jw_collection_search(
            self.collection_ptr,
            query_ptr,
            k,
            byref(results_ptr),
            byref(count)
        )
        if result != 0:
            raise VecDBError(result)
        
        search_results = []
        for i in range(count.value):
            search_results.append({
                'id': results_ptr[i].id,
                'distance': results_ptr[i].distance
            })
        
        # Free the memory
        ctypes.free(results_ptr)
        
        return search_results

def generate_random_vector(dimension):
    """Generate a random vector"""
    return [random.uniform(-1.0, 1.0) for _ in range(dimension)]

def main():
    """Main demo function"""
    print("========================================")
    print("  JinWo VecDB Python Demo")
    print("========================================")
    print()
    
    try:
        # Get version
        version = VecDB.get_version()
        print(f"Version: {version}")
        print()
        
        # Create database
        db_path = "./vecdb_python"
        print(f"Opening database at: {db_path}")
        db = VecDB(db_path, create=True)
        print("Database opened successfully")
        print()
        
        # Create collection
        print("Creating collection...")
        collection = db.create_collection("test", 128)
        print("Collection created successfully: test")
        print()
        
        # Insert vectors
        print("Inserting 10 vectors...")
        for i in range(10):
            vector = generate_random_vector(128)
            vector_id = collection.insert(vector)
            print(f"Inserted vector {i} successfully, ID: {vector_id}")
        print("Insertion completed")
        print()
        
        # Search
        print("Searching for similar vectors...")
        query = generate_random_vector(128)
        results = collection.search(query, 5)
        if results:
            print(f"Search results (top {len(results)}):")
            for i, result in enumerate(results):
                print(f"ID: {result['id']}, Distance: {result['distance']:.4f}")
        else:
            print("Search failed, no results")
        print()
        
        # List collections
        print("Listing all collections...")
        collections = db.list_collections()
        print(f"Found {len(collections)} collections:")
        for name in collections:
            print(f"  {name}")
        print()
        
        print("Demo completed successfully!")
        
    except VecDBError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()
