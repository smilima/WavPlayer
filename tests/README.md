# WavPlayer Automated Tests

This directory contains automated unit tests for the WavPlayer project using Google Test framework.

## Setup Instructions for Visual Studio 2022

### Step 1: Add Test Project to Solution

1. Open your WavPlayer solution in Visual Studio 2022
2. Right-click on the solution in Solution Explorer
3. Select **Add → Existing Project**
4. Navigate to the `tests` folder and select `WavPlayerTests.vcxproj`

### Step 2: Install Google Test via NuGet

The project is configured to use NuGet for Google Test. When you first build:

1. Right-click on the `WavPlayerTests` project
2. Select **Manage NuGet Packages**
3. If prompted, restore packages (or they may restore automatically)
4. The Google Test package `Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn` will be installed

**Alternative Manual Installation:**
1. Right-click on `WavPlayerTests` project
2. **Manage NuGet Packages**
3. Click **Browse** tab
4. Search for "google test"
5. Install `Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn`

### Step 3: Build the Test Project

1. Set the build configuration (Debug or Release)
2. Right-click on `WavPlayerTests` project
3. Select **Build**

The tests will be compiled to `bin\Debug\tests\WavPlayerTests.exe` or `bin\Release\tests\WavPlayerTests.exe`

### Step 4: Run Tests

#### Option A: Using Test Explorer (Recommended)

1. Open **Test Explorer** (Test → Test Explorer or Ctrl+E, T)
2. Click **Run All** to run all tests
3. Tests will appear in the explorer with pass/fail status
4. Click individual tests to see details
5. Right-click tests to run, debug, or view source

#### Option B: Using Command Line

```cmd
cd bin\Debug\tests
WavPlayerTests.exe
```

Or for specific tests:
```cmd
WavPlayerTests.exe --gtest_filter=TrackTests.*
```

#### Option C: Using Visual Studio Debugger

1. Right-click on `WavPlayerTests` project
2. **Set as Startup Project**
3. Press F5 to run with debugging
4. Tests will run in console and show results

## Test Organization

### Current Test Files

- **TrackTests.cpp** - Tests for Track class functionality
  - Track creation and properties
  - Volume, pan, and EQ controls
  - Mute/solo functionality
  - Peak level tracking
  - Color and visibility properties

- **SettingsTests.cpp** - Tests for Settings class
  - Window position and size persistence
  - Mixer window settings
  - Timeline settings
  - Project path storage

- **AudioUtilsTests.cpp** - Tests for audio utility functions
  - dB to linear conversion
  - Linear to dB conversion
  - Round-trip conversion accuracy
  - Range limiting and clamping

## Writing New Tests

### Test File Template

```cpp
#include "gtest/gtest.h"
#include "../YourClass.h"

// Simple test
TEST(TestSuiteName, TestName) {
    // Arrange
    YourClass obj;

    // Act
    int result = obj.someMethod();

    // Assert
    EXPECT_EQ(result, expectedValue);
}

// Test with fixture (for setup/teardown)
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Runs before each test
    }

    void TearDown() override {
        // Runs after each test
    }
};

TEST_F(MyTestFixture, TestWithFixture) {
    // Your test code
}
```

### Common Assertions

```cpp
EXPECT_EQ(a, b);        // a == b
EXPECT_NE(a, b);        // a != b
EXPECT_LT(a, b);        // a < b
EXPECT_GT(a, b);        // a > b
EXPECT_LE(a, b);        // a <= b
EXPECT_GE(a, b);        // a >= b

EXPECT_FLOAT_EQ(a, b);  // Floating point equality
EXPECT_NEAR(a, b, epsilon);  // Within epsilon

EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

EXPECT_STREQ(s1, s2);   // C-string equality
```

## Running Specific Tests

### Filter by test name
```cmd
WavPlayerTests.exe --gtest_filter=TrackTests.VolumeControl
```

### Filter by test suite
```cmd
WavPlayerTests.exe --gtest_filter=TrackTests.*
```

### Run tests matching pattern
```cmd
WavPlayerTests.exe --gtest_filter=*Volume*
```

### Exclude tests
```cmd
WavPlayerTests.exe --gtest_filter=-SettingsTests.*
```

## Continuous Integration

To integrate with CI/CD:

1. Build test project: `msbuild tests\WavPlayerTests.vcxproj /p:Configuration=Release`
2. Run tests: `bin\Release\tests\WavPlayerTests.exe --gtest_output=xml:test_results.xml`
3. Parse XML output for results

## Troubleshooting

### "Cannot find Google Test headers"
- Ensure NuGet packages are restored
- Check that the Google Test NuGet package is installed
- Verify `packages.config` is present

### "LNK2001 unresolved external symbol"
- Ensure all required source files are included in the test project
- Check that Windows libraries (d2d1.lib, dwrite.lib, winmm.lib) are linked
- Verify main.cpp is NOT included (it has WinMain which conflicts)

### Tests don't appear in Test Explorer
- Clean and rebuild the solution
- Close and reopen Visual Studio
- Check **Test → Test Explorer** is showing "All Tests"
- Verify test project built successfully

### Build errors about missing files
- Ensure all source files from main project are accessible
- Check relative paths in .vcxproj are correct
- Verify you're building x64 configuration (x86 not configured)

## Adding More Tests

To add new test files:

1. Create new `.cpp` file in `tests/` directory
2. Add `#include "gtest/gtest.h"` and your class headers
3. Write TEST() or TEST_F() macros
4. Add the file to `WavPlayerTests.vcxproj` in the `<ItemGroup>` with test files
5. Rebuild and run

## Best Practices

1. **Test Naming**: Use descriptive names - `TEST(AudioEngine, ProcessesMonoClipCorrectly)`
2. **Arrange-Act-Assert**: Structure tests clearly
3. **One Assertion Per Test**: Keep tests focused (guideline, not rule)
4. **Fast Tests**: Unit tests should run in milliseconds
5. **Isolated Tests**: Each test should be independent
6. **Mock External Dependencies**: For UI/IO, consider mocking

## Future Test Coverage

Areas to expand test coverage:

- [ ] AudioEngine playback logic
- [ ] AudioClip loading and processing
- [ ] Project save/load functionality
- [ ] Timeline view calculations
- [ ] Audio region manipulation
- [ ] Real-time audio mixing
- [ ] EQ filter processing
- [ ] Transport controls

## Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Advanced Google Test Topics](https://google.github.io/googletest/advanced.html)
