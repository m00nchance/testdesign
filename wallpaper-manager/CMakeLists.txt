cmake_minimum_required(VERSION 3.16)
project(WallpaperManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Find required packages
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SQLite3_INCLUDE_DIRS})

# Core utilities
add_library(core_utils STATIC
    src/core/utils.cpp
    src/core/database.cpp
)
target_link_libraries(core_utils PRIVATE ${SQLite3_LIBRARIES})

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
target_link_libraries(awww-win PRIVATE ${SQLite3_LIBRARIES} ole32 shell32 user32 gdi32)

# Install targets
install(TARGETS grim-win slurp-win awww-win DESTINATION bin)
