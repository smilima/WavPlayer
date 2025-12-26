# Quick Start: Tests in Visual Studio 2026

## Fastest Way to Get Started (30 seconds)

### Method 1: Open CMake Folder (Recommended)

```
Visual Studio 2026
    ↓
File → Open → Folder
    ↓
Select: WavPlayer folder
    ↓
Wait for "CMake generation finished" (watch Output window)
    ↓
Toolbar: Select "WavPlayerTests.exe"
    ↓
Press F5 to run tests!
```

**Test Explorer:**
- Test → Test Explorer (Ctrl+E, T)
- Click "Run All Tests"

---

### Method 2: Open Generated Solution

```
build_and_test.bat
    ↓
Open: build/SimpleDAW.sln
    ↓
Solution Explorer: Right-click "WavPlayerTests"
    ↓
Set as Startup Project
    ↓
Press F5 to run tests!
```

---

## Your Options at a Glance

### 🎯 Option 1: CMake Folder Mode (Modern)
**Open:** File → Open → Folder → Select `WavPlayer`

**Pros:**
- ✅ Zero setup
- ✅ Auto-updates when CMakeLists.txt changes
- ✅ Test Explorer works perfectly
- ✅ Modern Visual Studio workflow

**Best for:** Daily development

---

### 🎯 Option 2: Generated Solution (Traditional)
**Open:** `build/SimpleDAW.sln`

**Pros:**
- ✅ Familiar .sln workflow
- ✅ Clear project structure
- ✅ All projects in Solution Explorer

**Best for:** Traditional Visual Studio users

---

## Running Tests

### From Visual Studio UI:
1. **Build:** Ctrl+Shift+B
2. **Run:** F5 (with debugging) or Ctrl+F5 (without)
3. **Test Explorer:** Ctrl+E, T → "Run All Tests"

### From Command Line:
```cmd
build_and_test.bat
```

---

## Debugging a Test

1. Open `tests/track_test.cpp` (or any test file)
2. Set breakpoint (click left margin or F9)
3. Test Explorer → Right-click test → Debug
4. Debugger stops at breakpoint!
5. Inspect variables, step through, etc.

---

## Adding a New Test

1. Create file: `tests/my_new_test.cpp`
2. Edit `CMakeLists.txt`:
   ```cmake
   set(TEST_SOURCES
       tests/track_test.cpp
       tests/project_test.cpp
       tests/trackregion_test.cpp
       tests/my_new_test.cpp  # Add this line
       Track.cpp
       Project.cpp
       ...
   )
   ```
3. If using CMake folder: Right-click CMakeLists.txt → Generate Cache
4. If using .sln: Re-run `cmake ..` in build folder, reload solution
5. New tests appear in Test Explorer!

---

## Example Test File Template

```cpp
#include <gtest/gtest.h>
#include "../YourClass.h"

TEST(YourClassTest, TestName) {
    YourClass obj;
    EXPECT_EQ(obj.getValue(), 42);
}

TEST(YourClassTest, AnotherTest) {
    YourClass obj;
    obj.setValue(100);
    EXPECT_EQ(obj.getValue(), 100);
}
```

---

## Common Google Test Assertions

```cpp
// Equality
EXPECT_EQ(actual, expected);
EXPECT_NE(actual, not_expected);

// Comparison
EXPECT_LT(val1, val2);  // Less than
EXPECT_LE(val1, val2);  // Less or equal
EXPECT_GT(val1, val2);  // Greater than
EXPECT_GE(val1, val2);  // Greater or equal

// Boolean
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Floating point
EXPECT_FLOAT_EQ(val1, val2);
EXPECT_DOUBLE_EQ(val1, val2);

// Strings
EXPECT_STREQ(str1, str2);

// Fatal assertions (stop test on failure)
ASSERT_EQ(actual, expected);
ASSERT_TRUE(condition);
```

---

## Test Explorer Tips

- **Filter tests:** Use search box at top
- **Group tests:** Click group icon → Group by Class/Namespace
- **Re-run failed tests:** After fixing, click "Run Failed Tests"
- **Playlist:** Right-click tests → Add to Playlist (create test suites)
- **Code Coverage:** Right-click → Analyze Code Coverage

---

## Need Help?

- **Full guide:** See `VISUAL_STUDIO_INTEGRATION.md`
- **Build instructions:** See `BUILD_TESTS.md`
- **Test docs:** See `tests/README.md`
- **Google Test docs:** https://google.github.io/googletest/

---

## Current Test Coverage

✅ **Track class** (17 tests)
- Properties, regions, visibility, constraints

✅ **TrackRegion struct** (6 tests)
- Construction, time calculations

✅ **Project class** (12 tests)
- File management, tracks, settings

**Total: 35 tests** across 3 test files
