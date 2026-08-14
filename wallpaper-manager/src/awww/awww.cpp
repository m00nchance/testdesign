#include "awww.h"
#include "wallpaper_engine.h"
#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <sstream>

AwwwWin::AwwwWin() : initialized(false) {}

AwwwWin::~AwwwWin() {
    db.close();
}

bool AwwwWin::initialize(const std::string& path) {
    db_path = path;
    if (!db.initialize(path)) {
        return false;
    }
    initialized = true;
    return true;
}

bool AwwwWin::add_wallpaper(const std::string& image_path, const std::vector<std::string>& tags) {
    if (!initialized) return false;
    
    // Check if file exists
    if (!utils::file_exists(image_path)) {
        return false;
    }
    
    // Get dimensions
    int width = 0, height = 0;
    get_image_dimensions(image_path, width, height);
    
    // Create wallpaper entry
    Wallpaper wp;
    wp.path = image_path;
    wp.name = extract_filename(image_path);
    wp.width = width;
    wp.height = height;
    wp.file_size = utils::get_file_size(image_path);
    wp.is_favorite = false;
    wp.rating = 0;
    
    // Join tags with commas
    std::ostringstream tags_ss;
    for (size_t i = 0; i < tags.size(); i++) {
        if (i > 0) tags_ss << ",";
        tags_ss << tags[i];
    }
    wp.tags = tags_ss.str();
    
    int id = db.add_wallpaper(wp);
    return id != -1;
}

bool AwwwWin::remove_wallpaper(int id) {
    if (!initialized) return false;
    return db.delete_wallpaper(id);
}

bool AwwwWin::set_wallpaper(const std::string& image_path, int monitor_index) {
    if (!utils::file_exists(image_path)) {
        return false;
    }
    
    WallpaperEngine engine;
    if (monitor_index >= 0) {
        return engine.set_wallpaper_for_monitor(image_path, monitor_index);
    } else {
        return engine.set_wallpaper(image_path);
    }
}

std::vector<WallpaperEntry> AwwwWin::get_all_wallpapers() {
    std::vector<WallpaperEntry> entries;
    
    if (!initialized) return entries;
    
    auto wallpapers = db.get_all_wallpapers();
    for (const auto& wp : wallpapers) {
        WallpaperEntry entry;
        entry.path = wp.path;
        entry.name = wp.name;
        entry.width = wp.width;
        entry.height = wp.height;
        entry.is_favorite = wp.is_favorite;
        entry.rating = wp.rating;
        
        // Parse tags
        if (!wp.tags.empty()) {
            std::stringstream ss(wp.tags);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                // Trim whitespace
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    entry.tags.push_back(tag.substr(start, end - start + 1));
                }
            }
        }
        
        entries.push_back(entry);
    }
    
    return entries;
}

std::vector<WallpaperEntry> AwwwWin::search(const std::string& query) {
    std::vector<WallpaperEntry> entries;
    
    if (!initialized) return entries;
    
    auto wallpapers = db.search_wallpapers(query);
    for (const auto& wp : wallpapers) {
        WallpaperEntry entry;
        entry.path = wp.path;
        entry.name = wp.name;
        entry.width = wp.width;
        entry.height = wp.height;
        entry.is_favorite = wp.is_favorite;
        entry.rating = wp.rating;
        
        if (!wp.tags.empty()) {
            std::stringstream ss(wp.tags);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    entry.tags.push_back(tag.substr(start, end - start + 1));
                }
            }
        }
        
        entries.push_back(entry);
    }
    
    return entries;
}

std::vector<WallpaperEntry> AwwwWin::get_by_tag(const std::string& tag) {
    std::vector<WallpaperEntry> entries;
    
    if (!initialized) return entries;
    
    auto wallpapers = db.get_wallpapers_by_tag(tag);
    for (const auto& wp : wallpapers) {
        WallpaperEntry entry;
        entry.path = wp.path;
        entry.name = wp.name;
        entry.width = wp.width;
        entry.height = wp.height;
        entry.is_favorite = wp.is_favorite;
        entry.rating = wp.rating;
        
        if (!wp.tags.empty()) {
            std::stringstream ss(wp.tags);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    entry.tags.push_back(tag.substr(start, end - start + 1));
                }
            }
        }
        
        entries.push_back(entry);
    }
    
    return entries;
}

std::vector<WallpaperEntry> AwwwWin::get_favorites() {
    std::vector<WallpaperEntry> entries;
    
    if (!initialized) return entries;
    
    auto wallpapers = db.get_favorite_wallpapers();
    for (const auto& wp : wallpapers) {
        WallpaperEntry entry;
        entry.path = wp.path;
        entry.name = wp.name;
        entry.width = wp.width;
        entry.height = wp.height;
        entry.is_favorite = wp.is_favorite;
        entry.rating = wp.rating;
        
        if (!wp.tags.empty()) {
            std::stringstream ss(wp.tags);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    entry.tags.push_back(tag.substr(start, end - start + 1));
                }
            }
        }
        
        entries.push_back(entry);
    }
    
    return entries;
}

bool AwwwWin::toggle_favorite(int id) {
    if (!initialized) return false;
    
    Wallpaper wp = db.get_wallpaper(id);
    if (wp.id == -1) return false;
    
    wp.is_favorite = !wp.is_favorite;
    return db.update_wallpaper(wp);
}

bool AwwwWin::set_rating(int id, int rating) {
    if (!initialized) return false;
    if (rating < 0 || rating > 5) return false;
    
    Wallpaper wp = db.get_wallpaper(id);
    if (wp.id == -1) return false;
    
    wp.rating = rating;
    return db.update_wallpaper(wp);
}

std::vector<std::string> AwwwWin::get_all_tags() {
    if (!initialized) return {};
    return db.get_all_tags();
}

int AwwwWin::import_from_directory(const std::string& dir_path, bool recursive) {
    if (!initialized) return 0;
    
    int count = 0;
    std::vector<std::string> extensions = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp"};
    
    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                        if (add_wallpaper(entry.path().string())) {
                            count++;
                        }
                    }
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                        if (add_wallpaper(entry.path().string())) {
                            count++;
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Directory access error
    }
    
    return count;
}

Database* AwwwWin::get_database() {
    return &db;
}

bool AwwwWin::get_image_dimensions(const std::string& path, int& width, int& height) {
    // Simple BMP header reading
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    char header[54];
    file.read(header, 54);
    
    // Check BMP signature
    if (header[0] != 'B' || header[1] != 'M') {
        // Not a BMP, try other formats or return default
        // For production, would use GDI+ or stb_image.h
        width = 1920;
        height = 1080;
        return true;
    }
    
    // Read width and height from BMP header
    width = *reinterpret_cast<int32_t*>(&header[18]);
    height = *reinterpret_cast<int32_t*>(&header[22]);
    
    return true;
}

std::string AwwwWin::extract_filename(const std::string& path) {
    std::filesystem::path p(path);
    return p.filename().string();
}
