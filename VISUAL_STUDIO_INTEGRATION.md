# Integrating Google Tests with Visual Studio 2026

You have two excellent options for working with the tests in Visual Studio 2026:

## Option 1: Open CMake-Generated Solution (Recommended)

This is the easiest approach and gives you a fully integrated experience.

### Steps:

1. **Generate the solution** (if you haven't already):
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Visual Studio 18 2026" -A x64
   ```

2. **Open the solution**:
   - Navigate to `build/SimpleDAW.sln`
   - Double-click to open in Visual Studio 2026

   OR from Visual Studio:
   - File → Open → Project/Solution
   - Browse to `WavPlayer/build/SimpleDAW.sln`

3. **You'll see multiple projects**:
   - `SimpleDAW` - Your main WavPlayer application
   - `WavPlayerTests` - The test executable
   - `gtest` and `gtest_main` - Google Test libraries
   - `ALL_BUILD` - Builds everything
   - `RUN_TESTS` - Runs all tests

4. **Build and run tests**:
   - Right-click `WavPlayerTests` → Set as Startup Project
   - Press F5 to build and run tests with debugger
   - Or Ctrl+F5 to run without debugger

5. **Run tests via Test Explorer**:
   - Test → Test Explorer (or Ctrl+E, T)
   - Click "Run All Tests"
   - Individual tests will appear and you can run them separately

### Advantages:
✅ Full IntelliSense support
✅ Integrated debugging (set breakpoints in tests!)
✅ Test Explorer integration
✅ Solution Explorer shows all files
✅ Can build both main app and tests from one solution

---

## Option 2: Use Visual Studio's Native CMake Support

Visual Studio 2026 has excellent built-in CMake support, so you can work directly with CMakeLists.txt.

### Steps:

1. **Open the folder in Visual Studio**:
   - File → Open → Folder
   - Select your `WavPlayer` folder
   - Visual Studio will automatically detect `CMakeLists.txt`

2. **Wait for CMake configuration**:
   - Visual Studio will automatically run CMake
   - Watch the Output window (View → Output, select "CMake" from dropdown)
   - Wait for "CMake generation finished" message

3. **Select build target**:
   - In the toolbar, you'll see a dropdown showing build targets
   - Select `WavPlayerTests.exe` to build/run tests
   - Or select `SimpleDAW.exe` to build/run the main app

4. **Build and run**:
   - Press F5 to build and debug
   - Or use Build → Build All (Ctrl+Shift+B)

5. **Access Test Explorer**:
   - Test → Test Explorer
   - Tests will automatically appear
   - Click "Run All Tests" or run individual tests

### CMake Configuration:
   - Right-click `CMakeLists.txt` → "CMake Settings for SimpleDAW"
   - Customize build configurations if needed

### Advantages:
✅ No need to regenerate solution files
✅ Direct CMake integration
✅ Automatically picks up CMakeLists.txt changes
✅ Modern Visual Studio workflow
✅ Easy to switch between Debug/Release configurations

---

## Option 3: Keep Using Your Original .vcxproj

If you want to continue using your original `WavPlayer.vcxproj` for the main app and only use CMake for tests:

### Steps:

1. **For main app development**:
   - Use your existing `WavPlayer.vcxproj` as usual
   - Build and run normally

2. **For running tests**:
   - Use `build_and_test.bat`
   - Or open `build/SimpleDAW.sln` just for testing
   - Or use command line: `cd build && Debug\WavPlayerTests.exe`

### Advantages:
✅ Don't change your existing workflow
✅ Tests are separate and optional
✅ Simple to run tests when needed

---

## Recommended Test Explorer Workflow

Once you have tests showing in Test Explorer (works with both Option 1 and 2):

1. **Run all tests**: Click "Run All Tests" button
2. **Run specific test**: Click the test name
3. **Debug a test**: Right-click test → Debug
4. **Group tests**: Group by Project, Namespace, Class, or Traits
5. **Search tests**: Use the search box to filter tests

### Visual Studio Test Explorer Features:
- ✅ See test results with green ✓ or red ✗
- ✅ Click on failed tests to see error messages
- ✅ Set breakpoints in test code and debug
- ✅ Re-run only failed tests
- ✅ Tests automatically re-run on build (can be configured)

---

## Debugging Tests in Visual Studio

One of the biggest advantages of Visual Studio integration:

1. **Open a test file** (e.g., `tests/track_test.cpp`)
2. **Set a breakpoint** (click left margin or press F9)
3. **Right-click the test in Test Explorer** → Debug
4. **Debugger will stop at your breakpoint**
5. **Inspect variables**, step through code, etc.

This is incredibly helpful for debugging failing tests!

---

## Recommended Setup

**For most developers, I recommend Option 2** (CMake folder mode):
- Modern Visual Studio workflow
- No need to regenerate solution files
- Automatic CMake integration
- Full Test Explorer support
- Easy to add new test files (just edit CMakeLists.txt)

**If you prefer a traditional .sln workflow, use Option 1**:
- More familiar for classic Visual Studio users
- Clear project structure in Solution Explorer
- Just remember to re-run CMake if you change CMakeLists.txt

---

## Adding New Tests

When you add new test files:

### If using CMake folder mode (Option 2):
1. Create new test file in `tests/` folder
2. Add it to `TEST_SOURCES` in `CMakeLists.txt`
3. Right-click `CMakeLists.txt` → "Generate Cache"
4. Tests will automatically appear in Test Explorer

### If using generated solution (Option 1):
1. Create new test file in `tests/` folder
2. Add it to `TEST_SOURCES` in `CMakeLists.txt`
3. Re-run CMake: `cd build && cmake ..`
4. Reload solution in Visual Studio
5. Tests will appear in Test Explorer

---

## Troubleshooting

### Tests don't appear in Test Explorer
- Make sure you're using Visual Studio 2026 (has native Google Test support)
- Install "Test Adapter for Google Test" extension if needed
  - Extensions → Manage Extensions → Search "Google Test"
- Rebuild the solution/project
- Close and reopen Test Explorer

### CMake configuration fails
- Check Output window (View → Output, select CMake)
- Make sure CMake is installed (comes with Visual Studio)
- Delete `build/` folder and regenerate

### Can't debug tests
- Make sure you're building Debug configuration (not Release)
- Right-click test → Debug (not Run)
- Ensure `WavPlayerTests` project is built in Debug mode

---

## Summary

| Option | Best For | Pros | Cons |
|--------|----------|------|------|
| **Generated Solution** | Traditional VS users | Familiar, clear structure | Need to regenerate if CMakeLists.txt changes |
| **CMake Folder Mode** | Modern workflow | Auto-updates, clean | Slightly different UI |
| **Keep .vcxproj** | Minimal change | No workflow disruption | Less integrated testing |

All three options work great - choose what fits your workflow best!
