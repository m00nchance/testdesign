cmake_minimum_required(VERSION 3.16)
project(WallpaperManager VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Disable system SQLite3 search - use vendored version only
set(SQLite3_FOUND TRUE)
set(SQLite3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/vendor/sqlite)
set(SQLite3_LIBRARIES sqlite3)

# Include vendor directory for SQLite3
include_directories(${CMAKE_SOURCE_DIR}/vendor/sqlite)

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

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Core utilities
add_library(core_utils STATIC
    src/core/utils.cpp
    src/core/database.cpp
)
target_link_libraries(core_utils PRIVATE sqlite3)

# grim-win executable
add_executable(grim-win
    src/grim/grim.cpp
    src/grim/main.cpp
    src/core/utils.cpp
)
target_link_libraries(grim-win PRIVATE d3d11 dxgi)

# slurp-win executable
add_executable(slurp-win
    src/slurp/slurp.cpp
    src/slurp/main.cpp
)
target_link_libraries(slurp-win PRIVATE user32 gdi32)

# awww-win executable
add_executable(awww-win
    src/awww/awww.cpp
    src/awww/wallpaper_engine.cpp
    src/awww/main.cpp
    src/core/utils.cpp
    src/core/database.cpp
)
target_link_libraries(awww-win PRIVATE sqlite3 ole32 shell32 user32 gdi32)

# Install targets
install(TARGETS grim-win slurp-win awww-win DESTINATION bin)
