package main

/*
#cgo CFLAGS: -I../../include
#cgo LDFLAGS: -L../../build -ljw_vecdb -lm -pthread

#include <jw_vecdb.h>
#include <stdlib.h>

// Helper function to free char**
void free_char_array(char** array, size_t size) {
    if (array) {
        for (size_t i = 0; i < size; i++) {
            if (array[i]) {
                free(array[i]);
            }
        }
        free(array);
    }
}
*/
import "C"
import (
	"fmt"
	"math/rand"
	"unsafe"
)

// VecDBError represents an error from the VecDB library
type VecDBError struct {
	Code    int
	Message string
}

func (e *VecDBError) Error() string {
	return e.Message
}

// getErrorMessage returns the error message for a given error code
func getErrorMessage(code int) string {
	errorMessages := map[int]string{
		0:  "Success",
		-1: "Invalid parameter",
		-2: "Out of memory",
		-3: "File system error",
		-4: "Collection already exists",
		-5: "Collection does not exist",
		-6: "Vector does not exist",
		-7: "Index creation failed",
		-8: "Dimension mismatch",
		-9: "Internal error",
	}
	if msg, ok := errorMessages[code]; ok {
		return msg
	}
	return fmt.Sprintf("Unknown error: %d", code)
}

// VecDB represents a JinWo VecDB database
type VecDB struct {
	db *C.void
}

// NewVecDB creates or opens a database
func NewVecDB(path string, create bool) (*VecDB, error) {
	var db *C.void
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))

	result := C.jw_vecdb_open(&db, cPath, C.int(boolToInt(create)))
	if result != 0 {
		return nil, &VecDBError{
			Code:    int(result),
			Message: getErrorMessage(int(result)),
		}
	}

	return &VecDB{db: db}, nil
}

// Close closes the database
func (v *VecDB) Close() error {
	if v.db != nil {
		result := C.jw_vecdb_close(v.db)
		v.db = nil
		if result != 0 {
			return &VecDBError{
				Code:    int(result),
				Message: getErrorMessage(int(result)),
			}
		}
	}
	return nil
}

// GetVersion returns the JinWo VecDB version
func GetVersion() string {
	version := C.jw_version()
	return C.GoString(version)
}

// CreateCollection creates a new collection
func (v *VecDB) CreateCollection(name string, dimension int) (*Collection, error) {
	var col *C.void
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	result := C.jw_collection_create(&col, v.db, cName, C.int(dimension))
	if result != 0 {
		return nil, &VecDBError{
			Code:    int(result),
			Message: getErrorMessage(int(result)),
		}
	}

	return &Collection{col: col}, nil
}

// ListCollections returns a list of all collections
func (v *VecDB) ListCollections() ([]string, error) {
	var names **C.char
	var count C.size_t

	result := C.jw_collection_list(v.db, &names, &count)
	if result != 0 {
		return nil, &VecDBError{
			Code:    int(result),
			Message: getErrorMessage(int(result)),
		}
	}

	defer C.free_char_array(names, count)

	collections := make([]string, 0, count)
	for i := C.size_t(0); i < count; i++ {
		name := C.GoString(names[i])
		collections = append(collections, name)
	}

	return collections, nil
}

// Collection represents a JinWo VecDB collection
type Collection struct {
	col *C.void
}

// Close closes the collection
func (c *Collection) Close() error {
	if c.col != nil {
		result := C.jw_collection_close(c.col)
		c.col = nil
		if result != 0 {
			return &VecDBError{
				Code:    int(result),
				Message: getErrorMessage(int(result)),
			}
		}
	}
	return nil
}

// Insert inserts a vector into the collection
func (c *Collection) Insert(vector []float32) (uint64, error) {
	var id C.uint64_t
	
	// Convert Go slice to C array
	cVector := (*C.float)(unsafe.Pointer(&vector[0]))

	result := C.jw_collection_insert(c.col, cVector, &id)
	if result != 0 {
		return 0, &VecDBError{
			Code:    int(result),
			Message: getErrorMessage(int(result)),
		}
	}

	return uint64(id), nil
}

// SearchResult represents a search result
type SearchResult struct {
	ID       uint64
	Distance float32
}

// Search searches for similar vectors
func (c *Collection) Search(query []float32, k int) ([]SearchResult, error) {
	var results *C.jw_search_result_t
	var count C.size_t

	// Convert Go slice to C array
	cQuery := (*C.float)(unsafe.Pointer(&query[0]))

	result := C.jw_collection_search(c.col, cQuery, C.size_t(k), &results, &count)
	if result != 0 {
		return nil, &VecDBError{
			Code:    int(result),
			Message: getErrorMessage(int(result)),
		}
	}

	defer C.free(unsafe.Pointer(results))

	searchResults := make([]SearchResult, 0, count)
	for i := C.size_t(0); i < count; i++ {
		searchResults = append(searchResults, SearchResult{
			ID:       uint64(results[i].id),
			Distance: float32(results[i].distance),
		})
	}

	return searchResults, nil
}

// boolToInt converts a bool to int
func boolToInt(b bool) int {
	if b {
		return 1
	}
	return 0
}

// generateRandomVector generates a random vector
func generateRandomVector(dimension int) []float32 {
	vector := make([]float32, dimension)
	for i := range vector {
		vector[i] = float32(rand.Float64()*2 - 1) // Random value between -1 and 1
	}
	return vector
}

func main() {
	fmt.Println("========================================")
	fmt.Println("  JinWo VecDB Go Demo")
	fmt.Println("========================================")
	fmt.Println()

	// Get version
	version := GetVersion()
	fmt.Printf("Version: %s\n", version)
	fmt.Println()

	// Create database
	dbPath := "./vecdb_go"
	fmt.Printf("Opening database at: %s\n", dbPath)
	db, err := NewVecDB(dbPath, true)
	if err != nil {
		fmt.Printf("Error opening database: %v\n", err)
		return
	}
	defer db.Close()
	fmt.Println("Database opened successfully")
	fmt.Println()

	// Create collection
	fmt.Println("Creating collection...")
	collection, err := db.CreateCollection("test", 128)
	if err != nil {
		fmt.Printf("Error creating collection: %v\n", err)
		return
	}
	defer collection.Close()
	fmt.Println("Collection created successfully: test")
	fmt.Println()

	// Insert vectors
	fmt.Println("Inserting 10 vectors...")
	for i := 0; i < 10; i++ {
		vector := generateRandomVector(128)
		vectorID, err := collection.Insert(vector)
		if err != nil {
			fmt.Printf("Error inserting vector %d: %v\n", i, err)
			return
		}
		fmt.Printf("Inserted vector %d successfully, ID: %d\n", i, vectorID)
	}
	fmt.Println("Insertion completed")
	fmt.Println()

	// Search
	fmt.Println("Searching for similar vectors...")
	query := generateRandomVector(128)
	results, err := collection.Search(query, 5)
	if err != nil {
		fmt.Printf("Error searching: %v\n", err)
		return
	}

	if len(results) > 0 {
		fmt.Printf("Search results (top %d):\n", len(results))
		for i, result := range results {
			fmt.Printf("ID: %d, Distance: %.4f\n", result.ID, result.Distance)
		}
	} else {
		fmt.Println("Search failed, no results")
	}
	fmt.Println()

	// List collections
	fmt.Println("Listing all collections...")
	collections, err := db.ListCollections()
	if err != nil {
		fmt.Printf("Error listing collections: %v\n", err)
		return
	}

	fmt.Printf("Found %d collections:\n", len(collections))
	for _, name := range collections {
		fmt.Printf("  %s\n", name)
	}
	fmt.Println()

	fmt.Println("Demo completed successfully!")
}
