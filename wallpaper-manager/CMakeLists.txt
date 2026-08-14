cmake_minimum_required(VERSION 3.16)
project(WallpaperManager VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Disable system SQLite3 search - use vendored version only
set(SQLite3_FOUND TRUE)
set(SQLite3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/vendor/sqlite)
set(SQLite3_LIBRARIES sqlite3)

# Include vendor directory for SQLite3 and other libs
include_directories(${CMAKE_SOURCE_DIR}/vendor/sqlite)
include_directories(${CMAKE_SOURCE_DIR}/vendor/imgui)
include_directories(${CMAKE_SOURCE_DIR}/vendor/stb)

# SQLite3 amalgamation (vendored) - compile directly
add_library(sqlite3 STATIC 
    ${CMAKE_SOURCE_DIR}/vendor/sqlite/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC ${CMAKE_SOURCE_DIR}/vendor/sqlite)
target_compile_definitions(sqlite3 PUBLIC 
    _CRT_SECURE_NO_WARNINGS
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_JSON1
)

# Dear ImGui library (vendored)
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
)

# Only build platform-specific backends on Windows
if(WIN32)
    list(APPEND IMGUI_SOURCES
        ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_impl_win32.cpp
        ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_impl_dx11.cpp
    )
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui)
target_compile_definitions(imgui PUBLIC _CRT_SECURE_NO_WARNINGS)
if(WIN32)
    target_link_libraries(imgui PRIVATE d3d11 dxgi user32)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Core utilities
add_library(core_utils STATIC
    src/core/utils.cpp
    src/core/database.cpp
)
target_link_libraries(core_utils PRIVATE sqlite3)

# awww-win executable (CLI only)
add_executable(awww-win
    src/awww/awww.cpp
    src/awww/wallpaper_engine.cpp
    src/awww/main.cpp
    src/core/utils.cpp
    src/core/database.cpp
)
if(WIN32)
    target_link_libraries(awww-win PRIVATE sqlite3 ole32 shell32 user32 gdi32)
else()
    target_link_libraries(awww-win PRIVATE sqlite3)
endif()

# awww-win-gui executable (GUI version with wallpaper picker)
# Only build on Windows
if(WIN32)
    add_executable(awww-win-gui
        src/awww/awww.cpp
        src/awww/wallpaper_engine.cpp
        src/awww/gui.cpp
        src/core/utils.cpp
        src/core/database.cpp
    )
    target_link_libraries(awww-win-gui PRIVATE imgui sqlite3 ole32 shell32 user32 gdi32 d3d11 dxgi)
    target_compile_definitions(awww-win-gui PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # GUI not supported on non-Windows platforms - skip building
endif()

# Install targets
install(TARGETS awww-win DESTINATION bin)
if(WIN32)
    install(TARGETS awww-win-gui DESTINATION bin)
endif()
