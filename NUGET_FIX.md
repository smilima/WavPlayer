# NuGet Package Issue Fix

## Problem
The build is failing with errors related to the GoogleTest NuGet package:

**Error 1 - Missing Package:**
```
error : This project references NuGet package(s) that are missing on this computer.
The missing file is packages\Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8\build\native\Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.targets.
```

**Error 2 - Runtime Library Mismatch (if you restored packages):**
```
error LNK2038: mismatch detected for 'RuntimeLibrary': value 'MTd_StaticDebug' doesn't match value 'MDd_DynamicDebug' in Application.obj
```

## Root Cause
Your local `WavPlayer.vcxproj` file has references to the GoogleTest NuGet package. This is incorrect because:
1. **This project doesn't have any tests** - it's a WAV player application
2. **GoogleTest is not needed** for this project
3. The clean repository version doesn't include these references

## IMPORTANT: DO NOT Restore NuGet Packages
**DO NOT** use "Restore NuGet Packages" in Visual Studio - this will install GoogleTest but cause runtime library conflicts. Instead, **REMOVE** the GoogleTest references entirely.

## Solution - Complete Removal of GoogleTest

### Step 1: Close Visual Studio
Close Visual Studio completely before making any changes.

### Step 2: Remove GoogleTest from .vcxproj File

Open `WavPlayer.vcxproj` in a text editor (Notepad, VS Code, etc.) and remove the following:

1. **Remove Import statements** - Look for lines like these near the bottom of the file and DELETE them:
   ```xml
   <Import Project="packages\Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8\build\native\Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.targets" Condition="Exists('packages\...')" />
   ```

2. **Remove package reference ItemGroup** - Look for and DELETE:
   ```xml
   <ItemGroup>
     <None Include="packages.config" />
   </ItemGroup>
   ```

3. **Remove Error checking** - Look for and DELETE any lines checking for GoogleTest packages:
   ```xml
   <Error Condition="!Exists('packages\Microsoft.googletest...')" Text="..." />
   ```

4. **Remove NuGet targets import** - Look for and DELETE:
   ```xml
   <Import Project="$(VCTargetsPath)\...\NuGet.targets" />
   ```

### Step 3: Delete NuGet-Related Files

In your WavPlayer project directory, delete these files/folders if they exist:
- `packages` folder
- `packages.config` file
- Any `.nuget` folders

### Step 4: Clean Visual Studio Cache

1. Delete the `.vs` folder in your project directory (it's hidden)
2. Delete the `x64` and `Debug` folders if they exist

### Step 5: Rebuild

1. Open Visual Studio
2. Build → Clean Solution
3. Build → Rebuild Solution

## Quick Fix Using Git (Recommended)

If you haven't made other important changes to `WavPlayer.vcxproj`:

```bash
# Discard local changes to the project file
git checkout WavPlayer.vcxproj

# Delete the packages folder and config
rm -rf packages
rm -f packages.config

# Clean and rebuild in Visual Studio
```

## Alternative: Use the Clean Repository Version

1. Commit or stash any important code changes (NOT the .vcxproj changes)
2. Pull the clean version from this branch:
   ```bash
   git fetch origin claude/fix-nuget-packages-P7Iij
   git checkout WavPlayer.vcxproj
   ```
3. Delete packages folder: `rmdir /s /q packages` (Windows) or `rm -rf packages` (Git Bash)
4. Reopen Visual Studio and rebuild

## Prevention
- This project is a **WAV player** application, not a test project
- **Do not add GoogleTest** or other test frameworks to this project
- If you want to add tests in the future, create a **separate test project**
- Only use "Restore NuGet Packages" when packages are actually needed

## Verification
After applying the fix, you should see:
```
Build started...
1>------ Build started: Project: WavPlayer, Configuration: Debug x64 ------
1>WavPlayer.vcxproj -> C:\Users\...\WavPlayer\x64\Debug\WavPlayer.exe
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
```

## Still Having Issues?

If you're still seeing errors after following these steps:
1. Make sure Visual Studio is completely closed
2. Delete `.vs`, `x64`, `Debug`, and `packages` folders
3. Open `WavPlayer.vcxproj` in a text editor and verify there are NO references to "googletest" or "packages"
4. Reopen Visual Studio and rebuild
