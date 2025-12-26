# Quick Start Guide - WavPlayer Tests

## 5-Minute Setup

### 1. Open in Visual Studio 2022

Double-click `WavPlayer.sln` to open both the main project and test project.

### 2. Restore NuGet Packages

When you first open the solution:
- Visual Studio will prompt to restore NuGet packages
- Click **Restore** or right-click solution → **Restore NuGet Packages**
- This downloads Google Test automatically

### 3. Build Tests

- Right-click `WavPlayerTests` project
- Click **Build**
- Wait for build to complete (~30 seconds)

### 4. Run Tests

- Open **Test Explorer**: `Test → Test Explorer` (or press `Ctrl+E, T`)
- Click **Run All Tests** (green play button)
- Watch tests pass! ✓

## What Gets Tested?

✓ **Track Controls** - Volume, pan, EQ, mute, solo
✓ **Settings** - Window positions, preferences
✓ **Audio Math** - dB conversions, accuracy tests

## Quick Test Commands

### Run all tests
```
Test Explorer → Run All
```

### Run just Track tests
```
Test Explorer → Right-click "TrackTests" → Run
```

### Debug a failing test
```
Test Explorer → Right-click test → Debug
```

## Expected Results

You should see **~45 tests pass** in about 1 second:

```
[==========] Running 45 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 12 tests from TrackTests
...
[  PASSED  ] 45 tests.
```

## Common Issues

**"Cannot find gtest/gtest.h"**
→ Restore NuGet packages (right-click solution → Restore NuGet Packages)

**Tests don't show in Test Explorer**
→ Clean solution and rebuild: Build → Clean Solution, then Build → Build Solution

**Build errors about main.cpp**
→ This is normal - test project intentionally excludes main.cpp

## Next Steps

1. Run tests before committing code
2. Add tests when fixing bugs
3. Write tests for new features
4. Keep test coverage growing!

## Adding Your First Test

Edit `tests/TrackTests.cpp` and add:

```cpp
TEST(TrackTests, YourTestName) {
    Track track(L"My Test");

    // Your test code here
    EXPECT_TRUE(true);
}
```

Save, rebuild, and it appears in Test Explorer!

---

**Need help?** See `README.md` for detailed documentation.
