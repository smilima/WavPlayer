cmake_minimum_required(VERSION 3.20)
project(SimpleDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/D2DWindow.cpp
    src/MainWindow.cpp
    src/TransportBar.cpp
    src/TimelineView.cpp
    src/AudioEngine.cpp
    src/Track.cpp
)

set(HEADERS
    src/Application.h
    src/D2DWindow.h
    src/MainWindow.h
    src/TransportBar.h
    src/TimelineView.h
    src/AudioEngine.h
    src/Track.h
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
