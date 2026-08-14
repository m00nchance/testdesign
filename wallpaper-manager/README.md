# Wallpaper Manager for Windows

A fast, native C++ wallpaper manager inspired by the Linux tools `grim`, `slurp`, and `awww` from the 43PR dotfiles, but rebuilt for Windows.

## Components

### grim-win
Screenshot utility using DirectX 11 Desktop Duplication API for fast, efficient screen capture.

**Features:**
- Capture full screen or specific monitors
- Region capture support
- Multi-monitor support
- BMP output format

**Usage:**
```bash
grim-win                    # Capture primary monitor
grim-win -o screenshot.bmp  # Save to specific file
grim-win -m 1               # Capture second monitor
grim-win -l                 # List available monitors
grim-win -r 100,100 800x600 # Capture region
```

### slurp-win
Interactive region selector with a transparent overlay window.

**Features:**
- Click-and-drag region selection
- Visual feedback with coordinates
- ESC to cancel
- Outputs in slurp-compatible format: `x,y WxH`

**Usage:**
```bash
slurp-win  # Opens overlay, select region with mouse
```

### awww-win
Wallpaper management CLI with SQLite database backend.

**Features:**
- Add/manage wallpapers in database
- Tag and rate wallpapers (0-5 stars)
- Mark favorites
- Search by name or tags
- Import entire directories
- Set random wallpaper
- Apply wallpaper to desktop

**Usage:**
```bash
awww-win init                          # Initialize database
awww-win add C:\Wallpapers\cool.jpg    # Add wallpaper
awww-win list                          # List all wallpapers
awww-win set 5                         # Set wallpaper by ID
awww-win search nature                 # Search wallpapers
awww-win tag 3 landscape               # Add tag
awww-win fav 7                         # Toggle favorite
awww-win import C:\Wallpapers          # Import directory
awww-win random                        # Random wallpaper
```

## Building

### Prerequisites
- Windows 10/11
- Visual Studio 2019+ with C++ workload
- CMake 3.16+
- SQLite3 library

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 16 2019" -A x64

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix "C:\Program Files\WallpaperManager"
```

### Dependencies

The project requires:
- **SQLite3**: For database storage (link statically or install system-wide)
- **DirectX 11**: Included with Windows SDK
- **Windows SDK**: For Win32 APIs

## Project Structure

```
wallpaper-manager/
├── CMakeLists.txt
├── include/
│   ├── utils.h
│   ├── database.h
│   ├── grim.h
│   ├── slurp.h
│   ├── awww.h
│   └── wallpaper_engine.h
├── src/
│   ├── core/
│   │   ├── utils.cpp
│   │   └── database.cpp
│   ├── grim/
│   │   ├── grim.cpp
│   │   └── main.cpp
│   ├── slurp/
│   │   ├── slurp.cpp
│   │   └── main.cpp
│   └── awww/
│       ├── awww.cpp
│       ├── wallpaper_engine.cpp
│       └── main.cpp
└── build/
```

## Architecture

### Performance Characteristics
- **Startup time**: ~10-20ms (native C++)
- **Screenshot capture**: ~8-15ms using DXGI Desktop Duplication
- **Memory usage**: ~15-25MB per process
- **Database queries**: <1ms for typical operations

### Key Technologies
- **DirectX 11**: High-performance screen capture
- **Win32 API**: Native Windows integration
- **SQLite3**: Lightweight embedded database
- **STL/C++17**: Modern C++ features

## Usage Examples

### Capture workflow
```bash
# Select a region interactively
slurp-win > selection.txt

# Parse the output and capture that region
set /p SEL=<selection.txt
# SEL now contains something like "100,200 800x600"

# Use with grim-win (manual parsing needed in batch)
grim-win -r 100,200 800x600 -o selected_region.bmp
```

### Wallpaper management workflow
```bash
# Initialize
awww-win init

# Import existing collection
awww-win import D:\Wallpapers

# Browse and organize
awww-win list
awww-win tag 5 nature
awww-win fav 12

# Set daily random wallpaper (use Task Scheduler)
awww-win random
```

## License

MIT License - See LICENSE file for details.

## Credits

Inspired by the Linux tools from the sway/wlroots ecosystem:
- `grim`: Screenshot utility for Wayland
- `slurp`: Region selector for Wayland  
- Custom wallpaper management scripts

Ported to Windows with native C++ for maximum performance.
