#include "wallpaper_engine.h"
#include <windows.h>
#include <sstream>

#pragma comment(lib, "user32.lib")

WallpaperEngine::WallpaperEngine() {}

WallpaperEngine::~WallpaperEngine() {}

bool WallpaperEngine::set_wallpaper(const std::string& image_path) {
    if (!update_registry(image_path)) {
        return false;
    }
    
    // Notify all windows of the change
    notify_change();
    
    return true;
}

bool WallpaperEngine::set_wallpaper_for_monitor(const std::string& image_path, int monitor_index) {
    // For per-monitor wallpaper, we need to use the undocumented SetDisplayConfig
    // or modify the registry for each monitor separately
    
    // Simple approach: set for all monitors (Windows 10+)
    return set_wallpaper(image_path);
}

bool WallpaperEngine::set_wallpaper_style(int style) {
    return update_registry_style(style);
}

std::string WallpaperEngine::get_current_wallpaper() {
    char path[MAX_PATH];
    DWORD size = sizeof(path);
    
    HKEY hkey;
    LONG result = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Control Panel\\Desktop",
        0,
        KEY_READ,
        &hkey
    );
    
    if (result != ERROR_SUCCESS) {
        return "";
    }
    
    result = RegQueryValueExA(hkey, "WallPaper", nullptr, nullptr, reinterpret_cast<LPBYTE>(path), &size);
    RegCloseKey(hkey);
    
    if (result != ERROR_SUCCESS) {
        return "";
    }
    
    return std::string(path);
}

void WallpaperEngine::refresh_desktop() {
    notify_change();
}

bool WallpaperEngine::update_registry(const std::string& path) {
    HKEY hkey;
    LONG result = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Control Panel\\Desktop",
        0,
        KEY_WRITE,
        &hkey
    );
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    // Convert to wide string for registry
    std::wstring wpath(path.begin(), path.end());
    
    result = RegSetValueExW(
        hkey,
        L"WallPaper",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(wpath.c_str()),
        (wpath.length() + 1) * sizeof(wchar_t)
    );
    
    RegCloseKey(hkey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    // Also set the wallpaper style to fill (10)
    update_registry_style(10);
    
    return true;
}

bool WallpaperEngine::update_registry_style(int style) {
    HKEY hkey;
    LONG result = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Control Panel\\Desktop",
        0,
        KEY_WRITE,
        &hkey
    );
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    // Style mapping:
    // 0 = Tile, 1 = Center, 2 = Stretch, 3 = Fit, 4 = Fill, 5 = Span
    // Registry uses different values:
    // WallpaperStyle: 0 = Tile, 1 = Center, 10 = Stretch, 6 = Fit, 10 = Fill, 22 = Span
    // TileWallpaper: 1 = Tile, 0 = Not tiled
    
    std::string style_str, tile_str;
    
    switch (style) {
        case 0: // Fill
            style_str = "10";
            tile_str = "0";
            break;
        case 1: // Fit
            style_str = "6";
            tile_str = "0";
            break;
        case 2: // Stretch
            style_str = "2";
            tile_str = "0";
            break;
        case 3: // Tile
            style_str = "0";
            tile_str = "1";
            break;
        case 4: // Center
            style_str = "1";
            tile_str = "0";
            break;
        case 5: // Span
            style_str = "22";
            tile_str = "0";
            break;
        default:
            style_str = "10";
            tile_str = "0";
    }
    
    RegSetValueExA(hkey, "WallpaperStyle", 0, REG_SZ, 
                   reinterpret_cast<const BYTE*>(style_str.c_str()), style_str.length() + 1);
    RegSetValueExA(hkey, "TileWallpaper", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(tile_str.c_str()), tile_str.length() + 1);
    
    RegCloseKey(hkey);
    return true;
}

void WallpaperEngine::notify_change() {
    // Send WM_SETTINGCHANGE to all top-level windows
    SendMessageTimeoutA(
        HWND_BROADCAST,
        WM_SETTINGCHANGE,
        SPI_SETDESKWALLPAPER,
        0,
        SMTO_NORMAL,
        1000,
        nullptr
    );
    
    // Force refresh
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, nullptr, 
                         SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
}
