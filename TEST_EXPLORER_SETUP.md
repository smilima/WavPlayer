# Getting Test Explorer Working in Visual Studio 2026

## Quick Checklist

Follow these steps in order:

### ✅ Step 1: Install Google Test Adapter Extension

1. **Extensions → Manage Extensions**
2. Search for **"Test Adapter for Google Test"**
3. Install it (you may need to close VS and wait for installation)
4. Restart Visual Studio

**Alternative Extension:**
- If the above doesn't work, try **"Google Test Adapter"** by Christian Soltenborn

### ✅ Step 2: Open Project in Visual Studio

Choose ONE method:

**Method A: CMake Folder (Recommended)**
1. File → Open → Folder
2. Select your `WavPlayer` folder
3. Wait for CMake to finish configuring (watch Output window)

**Method B: Solution File**
1. File → Open → Project/Solution
2. Navigate to `build/SimpleDAW.sln`
3. Open it

### ✅ Step 3: Build the Test Project

**This is critical!** Tests won't appear until the executable is built.

1. In toolbar, make sure configuration is **Debug** (not Release)
2. Build → Build Solution (Ctrl+Shift+B)
3. Wait for build to complete successfully
4. Verify output shows: `WavPlayerTests.vcxproj -> ...\Debug\WavPlayerTests.exe`

### ✅ Step 4: Open Test Explorer

1. **Test → Test Explorer** (or press Ctrl+E, T)
2. You should see tests appear automatically

If tests don't appear, try:
- Click the **Refresh** button (circular arrow icon) at top of Test Explorer
- Close and reopen Test Explorer
- Build → Rebuild Solution

### ✅ Step 5: Enable Test Discovery (If Still Not Working)

1. **Tools → Options**
2. Navigate to: **Test → General**
3. Make sure these are checked:
   - ☑️ "Automatically detect run settings files"
   - ☑️ "Discover tests in real time from C++ test sources"
4. Click OK

### ✅ Step 6: Configure Google Test Adapter (If Still Not Working)

1. **Tools → Options**
2. Navigate to: **Test Adapter for Google Test** (or **Google Test Adapter**)
3. Check these settings:
   - **Test discovery:** Set to "Executable"
   - **Test discovery regex:** Leave default
   - **Working directory:** Leave default
4. Click OK
5. Restart Visual Studio

---

## Still Not Working? Try These:

### Option 1: Manual Test Discovery

1. **Test Explorer** window
2. Click the **Settings** icon (gear) in toolbar
3. Click **Run Tests After Build**
4. Build the solution again
5. Tests should appear

### Option 2: Check Test Executable Path

1. Open Output window (View → Output)
2. Select "Tests" from dropdown
3. Look for messages about test discovery
4. Verify it's looking in the right path for `WavPlayerTests.exe`

The path should be something like:
- `WavPlayer/build/Debug/WavPlayerTests.exe` (if using generated solution)
- `WavPlayer/out/build/x64-Debug/Debug/WavPlayerTests.exe` (if using CMake folder)

### Option 3: Run Tests Manually to Verify They Work

Open a command prompt and run:

```cmd
cd WavPlayer\build\Debug
WavPlayerTests.exe
```

If this shows test results, your tests work - it's just a VS discovery issue.

### Option 4: Force Test Adapter Settings

Create a file called `.runsettings` in your WavPlayer root directory:

```xml
<?xml version="1.0" encoding="utf-8"?>
<RunSettings>
  <RunConfiguration>
    <MaxCpuCount>1</MaxCpuCount>
    <ResultsDirectory>.\TestResults</ResultsDirectory>
    <TargetPlatform>x64</TargetPlatform>
  </RunConfiguration>
  <GoogleTestAdapterSettings>
    <SolutionSettings>
      <Settings>
        <TestDiscoveryRegex>.*</TestDiscoveryRegex>
        <TestDiscoveryTimeoutInSeconds>30</TestDiscoveryTimeoutInSeconds>
        <WorkingDir>$(ExecutableDir)</WorkingDir>
        <PathExtension>$(ExecutableDir)</PathExtension>
      </Settings>
    </SolutionSettings>
  </GoogleTestAdapterSettings>
</RunSettings>
```

Then:
1. **Test → Configure Run Settings → Select Solution Wide runsettings File**
2. Select the `.runsettings` file you just created
3. Rebuild solution

---

## Expected Result

Once working, you should see in Test Explorer:

```
📁 WavPlayerTests
  📁 TrackTest
    ✓ Constructor
    ✓ DefaultConstructor
    ✓ SetName
    ✓ Volume
    ✓ Pan
    ✓ Mute
    ✓ Solo
    ✓ Armed
    ✓ Visibility
    ✓ VisibilityWithRegions
    ✓ Height
    ✓ Color
    ✓ Regions
  📁 TrackRegionTest
    ✓ DefaultConstructor
    ✓ EndTime
    ✓ CustomValues
    ✓ ZeroDuration
    ✓ ClipOffset
  📁 ProjectTest
    ✓ Constructor
    ✓ Filename
    ✓ ModifiedFlag
    ✓ BPM
    ✓ SampleRate
    ✓ TrackManagement
    ✓ Clear
    ✓ ProjectName
    ✓ HasAudioLoaded
    ✓ FileExtension
```

Total: **35 tests**

---

## Visual Studio Version Note

Visual Studio 2026 is very new. If you're having persistent issues:

1. Make sure Visual Studio is fully updated:
   - Help → Check for Updates

2. Verify you have the latest components:
   - Tools → Get Tools and Features
   - Ensure "Desktop development with C++" is installed
   - Ensure "Test Adapter for Google Test" is included

3. Check if there's a compatibility issue:
   - Try Visual Studio 2022 if available (has mature Google Test support)

---

## Debugging Discovery Issues

### View Test Output:

1. **View → Output**
2. In the "Show output from:" dropdown, select **Tests**
3. This will show test discovery process and any errors

### Enable Diagnostic Logging:

1. **Tools → Options → Test Adapter for Google Test**
2. Set **Logging level** to **Trace** or **Debug**
3. Rebuild solution
4. Check Output window (Tests) for detailed discovery logs

---

## Working Alternative: Use CTest

If Test Explorer still doesn't work, you can use CTest (built into CMake):

```cmd
cd build
ctest -C Debug --verbose
```

This will run all tests and show results.

---

## Report Back

After trying these steps, let me know:
1. Which method you used to open the project (CMake folder or .sln)?
2. Did the build succeed? (Check Output window)
3. Is the Google Test Adapter extension installed?
4. What does the Output window (Tests) show?

I'll help you troubleshoot further!
