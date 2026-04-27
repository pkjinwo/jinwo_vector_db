# examples/CMakeLists.txt
# Main CMakeLists.txt for JinWo VecDB examples
# Each platform has its own subdirectory with platform-specific demos

# Platform-specific demos are organized in subdirectories:
# - windows/   : Windows C# demo (Visual Studio solution)
# - linux/     : Linux C demo (CMake/Make)
# - android/   : Android Java demo (Android Studio)
# - ios/       : iOS Swift demo (Xcode)
# - python/    : Python demo (native Python)
# - go/        : Go demo (cgo)
# - nodejs/    : Node.js demo (ffi-napi)
# - php/       : PHP demo (FFI)

# Note: The main CMakeLists.txt only builds the base demo applications
# Platform-specific builds should use the CMakeLists.txt in each platform directory

add_executable(jw_vecdb_demo demo.c)
target_link_libraries(jw_vecdb_demo PRIVATE jw_vecdb_static)

add_executable(jw_vector_bench vector_bench.c)
target_link_libraries(jw_vector_bench PRIVATE jw_vecdb_static)

# Language-specific examples (non-CMake builds):
# - Python: Run with 'python python/python_demo.py'
# - Go: Run with 'go run go/go_demo.go'
# - Node.js: Run with 'npm start' in nodejs directory
# - PHP: Run with 'php php/php_demo.php'
