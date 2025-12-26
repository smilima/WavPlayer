# WavPlayer Unit Tests

This directory contains unit tests for the WavPlayer project using Google Test framework.

## Test Structure

- `track_test.cpp` - Tests for the Track class
- `project_test.cpp` - Tests for the Project class
- `trackregion_test.cpp` - Tests for the TrackRegion struct

## Building and Running Tests

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler
- Windows SDK (for Windows-specific dependencies)

### Building Tests

```bash
# Create build directory
mkdir build
cd build

# Configure CMake
cmake ..

# Build the project and tests
cmake --build .

# Or build only tests
cmake --build . --target WavPlayerTests
```

### Running Tests

```bash
# Run all tests
ctest

# Or run the test executable directly
./WavPlayerTests

# Run with verbose output
./WavPlayerTests --gtest_verbose

# Run specific test suite
./WavPlayerTests --gtest_filter=TrackTest.*

# Run specific test
./WavPlayerTests --gtest_filter=TrackTest.Volume
```

## Test Coverage

Current tests cover:

### Track Class
- Constructor and default values
- Property getters and setters (name, volume, pan, mute, solo, armed, height, color)
- Visibility logic (explicit and with regions)
- Region management (add/remove)
- Height constraints (minimum 60px)

### TrackRegion Struct
- Default construction
- End time calculation
- Property values
- Clip offset handling

### Project Class
- Constructor and default values
- Filename management
- Modified flag behavior
- BPM and sample rate settings
- Track management (add/remove/clear)
- Project name extraction
- Audio loaded detection
- File extension constants

## Adding New Tests

To add new tests:

1. Create a new test file in the `tests/` directory (e.g., `myclass_test.cpp`)
2. Include the necessary headers and use Google Test macros
3. Add the test file to `TEST_SOURCES` in the root `CMakeLists.txt`
4. Add any additional source files needed for the tests

Example test structure:

```cpp
#include <gtest/gtest.h>
#include "../MyClass.h"

TEST(MyClassTest, TestName) {
    MyClass obj;
    EXPECT_EQ(obj.getValue(), 42);
}
```

## Google Test Framework

The project uses Google Test v1.15.2, which is automatically downloaded via CMake's FetchContent.

For more information on writing tests, see the [Google Test documentation](https://google.github.io/googletest/).
