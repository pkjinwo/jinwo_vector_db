module jinwo/vecdb/wasm_demo

go 1.25.0

require github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm v0.0.0

require (
	github.com/tetratelabs/wazero v1.12.0 // indirect
	golang.org/x/sys v0.44.0 // indirect
)

replace github.com/pkjinwo/jinwo_vector_db/bindings/jinwo_vecdb_wasm => ../../bindings/jinwo_vecdb_wasm
