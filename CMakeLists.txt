cmake_minimum_required(VERSION 3.20)
project(SimpleDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Source files
set(SOURCES
    main.cpp
    Application.cpp
    D2DWindow.cpp
    MainWindow.cpp
    TransportBar.cpp
    TimelineView.cpp
    AudioEngine.cpp
    Track.cpp
    Project.cpp
    SpectrumWindow.cpp
)

set(HEADERS
    Application.h
    D2DWindow.h
    MainWindow.h
    TransportBar.h
    TimelineView.h
    AudioEngine.h
    Track.h
    Project.h
    SpectrumWindow.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1
    dwrite
    winmm
    Shlwapi
    Comdlg32
    Shell32
    Ole32
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Group source files in IDE
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

# Google Test setup
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Test sources (exclude main.cpp and Windows-specific UI files)
set(TEST_SOURCES
    tests/track_test.cpp
    tests/project_test.cpp
    tests/trackregion_test.cpp
    Track.cpp
    Project.cpp
    AudioEngine.cpp
    Settings.cpp
)

# Create test executable
add_executable(WavPlayerTests ${TEST_SOURCES})

# Link Google Test and Windows libraries
target_link_libraries(WavPlayerTests PRIVATE
    GTest::gtest_main
    winmm
    Shlwapi
)

# Add compiler definitions for tests
target_compile_definitions(WavPlayerTests PRIVATE UNICODE _UNICODE NOMINMAX)

# Include Google Test module
include(GoogleTest)
gtest_discover_tests(WavPlayerTests)
