# JinWo VecDB Windows Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains a Windows example for JinWo VecDB, demonstrating how to use C# and P/Invoke to call the C API for the vector database.

## Prerequisites

- Visual Studio 2022
- .NET 8.0 SDK
- Built JinWo VecDB library (`jw_vecdb.dll`)

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && cmake --build . --config Release
```

### 2. Open Visual Studio Project

```bash
# Open Visual Studio project
cd ../examples/windows
# Open JinWoVecDBDemo.sln in Visual Studio
```

### 3. Build and Run

- In Visual Studio, select Release configuration
- Click Build > Build Solution or press F7
- Click Debug > Start Without Debugging or press Ctrl+F5
- Or build using command line:
  ```bash
  dotnet build JinWoVecDBDemo.csproj -c Release
  cd bin/Release/net8.0
  ./JinWoVecDBDemo.exe
  ```

## Feature Demonstration

The Windows example demonstrates the following features:

- **Database Operations**
  - Open/close database
  - Create new database or open existing database

- **Collection Operations**
  - Create collection
  - List all collections

- **Vector Operations**
  - Insert vectors
  - Search similar vectors (KNN search)

- **Version Information**
  - Get JinWo VecDB version number

## Code Structure

- `windows_demo.cs` - C# demonstration program
- `JinWoVecDBDemo.sln` - Visual Studio solution
- `JinWoVecDBDemo.csproj` - Visual Studio project file

## Technical Implementation

- Uses C# P/Invoke to call native DLL
- Wraps C API, provides C#-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### DLL Not Found
- **Issue**: Cannot find `jw_vecdb.dll`
- **Solution**: Ensure the DLL file is in the executable directory or system PATH

### .NET Version Issues
- **Issue**: .NET version incompatibility
- **Solution**: Ensure .NET 8.0 SDK is installed

### Platform Target Issues
- **Issue**: Platform target mismatch
- **Solution**: Ensure project is set to x64 platform

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [Windows Integration Guide](../../docs/en_us/windows_integration.en_us.md)
