# JinWo VecDB iOS Example

**Version**: v1.0.0
**Last Updated**: 2026-04-26

## Overview

This directory contains an iOS example for JinWo VecDB, demonstrating how to use Swift to call the C API for the vector database.

## Prerequisites

- Xcode 15.0+
- iOS 13.0+
- Built JinWo VecDB static library (`libjw_vecdb.a`)

## Quick Start

### 1. Build JinWo VecDB Library

```bash
# First build JinWo VecDB library
cd ../../
mkdir -p build && cd build
cmake .. && make
```

### 2. Open Xcode Project

```bash
# Open Xcode project
cd ../examples/ios
open JinWoVecDBiOSDemo.xcodeproj
```

### 3. Build and Run

- In Xcode, select target device (simulator or device)
- Click Run or press Cmd+R
- Or build using command line:
  ```bash
  xcodebuild -project JinWoVecDBiOSDemo.xcodeproj -scheme JinWoVecDBiOSDemo -destination 'platform=iOS Simulator,name=iPhone 15' build
  ```

## Feature Demonstration

The iOS example demonstrates the following features:

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

- `JinWoVecDBiOSDemo/Sources/iOSDemo.swift` - Swift demonstration program
- `JinWoVecDBiOSDemo/Sources/ViewController.swift` - View controller
- `JinWoVecDBiOSDemo/Sources/AppDelegate.swift` - App delegate
- `JinWoVecDBiOSDemo.xcodeproj` - Xcode project file

## Technical Implementation

- Uses Swift C bridging to call native library
- Wraps C API, provides Swift-callable interface
- Directly calls JinWo VecDB C API

## Troubleshooting

### Static Library Not Found
- **Issue**: Cannot find `libjw_vecdb.a`
- **Solution**: Ensure the library is properly built and integrated into the Xcode project

### Signing Issues
- **Issue**: Code signing failure
- **Solution**: Ensure developer certificates are properly configured in Xcode

### iOS Version Issues
- **Issue**: iOS version incompatibility
- **Solution**: Ensure target device is running iOS 13.0 or higher

## Related Documentation

- [API Reference](../../docs/en_us/api_reference.en_us.md)
- [iOS Integration Guide](../../docs/en_us/ios_integration.en_us.md)
