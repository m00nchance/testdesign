#ifndef AWWW_H
#define AWWW_H

#include <string>
#include <vector>
#include "database.h"

struct WallpaperEntry {
    std::string path;
    std::string name;
    std::vector<std::string> tags;
    int width;
    int height;
    bool is_favorite;
    int rating;
};

class AwwwWin {
public:
    AwwwWin();
    ~AwwwWin();
    
    // Initialize the application
    bool initialize(const std::string& db_path);
    
    // Add wallpaper to database
    bool add_wallpaper(const std::string& image_path, const std::vector<std::string>& tags = {});
    
    // Remove wallpaper from database
    bool remove_wallpaper(int id);
    
    // Set wallpaper (apply to desktop)
    bool set_wallpaper(const std::string& image_path, int monitor_index = 0);
    
    // Get all wallpapers
    std::vector<WallpaperEntry> get_all_wallpapers();
    
    // Search wallpapers
    std::vector<WallpaperEntry> search(const std::string& query);
    
    // Get wallpapers by tag
    std::vector<WallpaperEntry> get_by_tag(const std::string& tag);
    
    // Get favorite wallpapers
    std::vector<WallpaperEntry> get_favorites();
    
    // Toggle favorite status
    bool toggle_favorite(int id);
    
    // Set rating
    bool set_rating(int id, int rating);
    
    // Get all unique tags
    std::vector<std::string> get_all_tags();
    
    // Import multiple wallpapers from directory
    int import_from_directory(const std::string& dir_path, bool recursive = false);
    
    // Get database instance
    Database* get_database();
    
private:
    Database db;
    bool initialized;
    std::string db_path;
    
    // Helper to extract image dimensions
    bool get_image_dimensions(const std::string& path, int& width, int& height);
    
    // Helper to extract filename from path
    std::string extract_filename(const std::string& path);
};

#endif // AWWW_H
