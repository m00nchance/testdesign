#ifndef WALLPAPER_ENGINE_H
#define WALLPAPER_ENGINE_H

#include <string>

#ifdef _WIN32
// Windows-specific implementation
class WallpaperEngine {
public:
    WallpaperEngine();
    ~WallpaperEngine();
    
    // Set wallpaper for all monitors
    bool set_wallpaper(const std::string& image_path);
    
    // Set wallpaper for specific monitor
    bool set_wallpaper_for_monitor(const std::string& image_path, int monitor_index);
    
    // Set wallpaper style (0 = fill, 1 = fit, 2 = stretch, 3 = tile, 4 = center, 5 = span)
    bool set_wallpaper_style(int style);
    
    // Get current wallpaper path
    std::string get_current_wallpaper();
    
    // Refresh desktop
    void refresh_desktop();
    
private:
    // Windows Registry methods
    bool update_registry(const std::string& path);
    bool update_registry_style(int style);
    
    // Notify system of change
    void notify_change();
};
#else
// Stub implementation for non-Windows platforms
class WallpaperEngine {
public:
    WallpaperEngine() {}
    ~WallpaperEngine() {}
    
    bool set_wallpaper(const std::string& image_path) { 
        (void)image_path;
        return false; 
    }
    
    bool set_wallpaper_for_monitor(const std::string& image_path, int monitor_index) {
        (void)image_path;
        (void)monitor_index;
        return false;
    }
    
    bool set_wallpaper_style(int style) {
        (void)style;
        return false;
    }
    
    std::string get_current_wallpaper() {
        return "";
    }
    
    void refresh_desktop() {}
};
#endif

#endif // WALLPAPER_ENGINE_H
