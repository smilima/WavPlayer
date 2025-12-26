# Building and Running Tests

## Building with CMake (Recommended)

The tests are configured to build using CMake. Here's how to build them:

### Using CMake GUI (Easiest for Visual Studio users)

1. Open CMake GUI
2. Set "Where is the source code" to your WavPlayer directory
3. Set "Where to build the binaries" to `WavPlayer/build` (or any directory you prefer)
4. Click "Configure"
   - Select your Visual Studio version as the generator
   - Click "Finish"
5. Click "Generate"
6. Click "Open Project" to open in Visual Studio

OR simply click "Build" in CMake GUI

### Using Command Line

```cmd
# From the WavPlayer directory
mkdir build
cd build

# Configure with Visual Studio 2022 (adjust version as needed)
cmake .. -G "Visual Studio 17 2022" -A x64

# Build both the main project and tests
cmake --build . --config Debug

# Or build just the tests
cmake --build . --config Debug --target WavPlayerTests

# Run the tests
Debug\WavPlayerTests.exe

# Or use CTest
ctest -C Debug --verbose
```

### Using Visual Studio Code

If you have CMake Tools extension:
1. Open the WavPlayer folder in VS Code
2. CMake will auto-configure
3. Select build target: `WavPlayerTests`
4. Press F7 to build

## Option 2: Building from Visual Studio Solution

If you want to continue using the existing Visual Studio solution:

1. Open the generated solution from the CMake build:
   - After running CMake (steps above), open `build/SimpleDAW.sln`
   - This solution includes both the main project and the test project

2. Build the `WavPlayerTests` project
3. Run the tests

## Running Tests

After building successfully:

```cmd
# From the build directory
Debug\WavPlayerTests.exe

# Run with Google Test flags
Debug\WavPlayerTests.exe --gtest_filter=TrackTest.*
Debug\WavPlayerTests.exe --gtest_verbose
```

## Why Not the Original .vcxproj?

The original `WavPlayer.vcxproj` was created manually and doesn't include the Google Test framework or test configuration. The CMake build system handles:
- Downloading Google Test automatically
- Configuring runtime library settings correctly
- Setting up proper include paths and linking
- Creating the test executable

You can continue using the original `.vcxproj` for the main application, but use the CMake-generated solution for building and running tests.
