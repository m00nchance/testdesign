#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>

struct Wallpaper {
    int id;
    std::string path;
    std::string name;
    std::string tags;
    int width;
    int height;
    uint64_t file_size;
    std::string created_at;
    std::string updated_at;
    bool is_favorite;
    int rating; // 0-5
};

class Database {
public:
    Database();
    ~Database();
    
    bool initialize(const std::string& db_path);
    void close();
    
    // CRUD operations for wallpapers
    int add_wallpaper(const Wallpaper& wallpaper);
    bool update_wallpaper(const Wallpaper& wallpaper);
    bool delete_wallpaper(int id);
    Wallpaper get_wallpaper(int id);
    std::vector<Wallpaper> get_all_wallpapers();
    std::vector<Wallpaper> search_wallpapers(const std::string& query);
    std::vector<Wallpaper> get_wallpapers_by_tag(const std::string& tag);
    std::vector<Wallpaper> get_favorite_wallpapers();
    
    // Tag operations
    std::vector<std::string> get_all_tags();
    bool add_tag_to_wallpaper(int wallpaper_id, const std::string& tag);
    bool remove_tag_from_wallpaper(int wallpaper_id, const std::string& tag);
    
    // Settings
    bool set_setting(const std::string& key, const std::string& value);
    std::string get_setting(const std::string& key, const std::string& default_value = "");
    
private:
    sqlite3* db;
    bool create_tables();
};

#endif // DATABASE_H
