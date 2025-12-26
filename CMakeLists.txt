cmake_minimum_required(VERSION 3.20)
project(SimpleDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
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

# Find your test file list, likely looks like:
set(TEST_SOURCES
    tests/trackregion_test.cpp
    tests/track_test.cpp        # Remove or comment out
    tests/project_test.cpp      # Remove or comment out
)
