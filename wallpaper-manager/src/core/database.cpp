#include "database.h"
#include "utils.h"
#include <sstream>
#include <algorithm>

Database::Database() : db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::initialize(const std::string& db_path) {
    // Ensure directory exists
    std::filesystem::path db_file(db_path);
    utils::ensure_directory_exists(db_file.parent_path().string());
    
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    return create_tables();
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::create_tables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS wallpapers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            tags TEXT DEFAULT '',
            width INTEGER DEFAULT 0,
            height INTEGER DEFAULT 0,
            file_size INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
            is_favorite INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0
        );
        
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
        
        CREATE INDEX IF NOT EXISTS idx_wallpapers_name ON wallpapers(name);
        CREATE INDEX IF NOT EXISTS idx_wallpapers_tags ON wallpapers(tags);
        CREATE INDEX IF NOT EXISTS idx_wallpapers_favorite ON wallpapers(is_favorite);
    )";
    
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        return false;
    }
    
    return true;
}

int Database::add_wallpaper(const Wallpaper& wallpaper) {
    const char* sql = R"(
        INSERT INTO wallpapers (path, name, tags, width, height, file_size, is_favorite, rating)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, wallpaper.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, wallpaper.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, wallpaper.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, wallpaper.width);
    sqlite3_bind_int(stmt, 5, wallpaper.height);
    sqlite3_bind_int64(stmt, 6, wallpaper.file_size);
    sqlite3_bind_int(stmt, 7, wallpaper.is_favorite ? 1 : 0);
    sqlite3_bind_int(stmt, 8, wallpaper.rating);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return id;
}

bool Database::update_wallpaper(const Wallpaper& wallpaper) {
    const char* sql = R"(
        UPDATE wallpapers 
        SET name = ?, tags = ?, width = ?, height = ?, file_size = ?, 
            is_favorite = ?, rating = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, wallpaper.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, wallpaper.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, wallpaper.width);
    sqlite3_bind_int(stmt, 4, wallpaper.height);
    sqlite3_bind_int64(stmt, 5, wallpaper.file_size);
    sqlite3_bind_int(stmt, 6, wallpaper.is_favorite ? 1 : 0);
    sqlite3_bind_int(stmt, 7, wallpaper.rating);
    sqlite3_bind_int(stmt, 8, wallpaper.id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::delete_wallpaper(int id) {
    const char* sql = "DELETE FROM wallpapers WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

Wallpaper Database::get_wallpaper(int id) {
    Wallpaper wallpaper;
    wallpaper.id = -1;
    
    const char* sql = "SELECT * FROM wallpapers WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return wallpaper;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        wallpaper.id = sqlite3_column_int(stmt, 0);
        wallpaper.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        wallpaper.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        wallpaper.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        wallpaper.width = sqlite3_column_int(stmt, 4);
        wallpaper.height = sqlite3_column_int(stmt, 5);
        wallpaper.file_size = sqlite3_column_int64(stmt, 6);
        wallpaper.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        wallpaper.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        wallpaper.is_favorite = sqlite3_column_int(stmt, 9) != 0;
        wallpaper.rating = sqlite3_column_int(stmt, 10);
    }
    
    sqlite3_finalize(stmt);
    return wallpaper;
}

std::vector<Wallpaper> Database::get_all_wallpapers() {
    std::vector<Wallpaper> wallpapers;
    
    const char* sql = "SELECT * FROM wallpapers ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return wallpapers;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Wallpaper wallpaper;
        wallpaper.id = sqlite3_column_int(stmt, 0);
        wallpaper.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        wallpaper.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        wallpaper.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        wallpaper.width = sqlite3_column_int(stmt, 4);
        wallpaper.height = sqlite3_column_int(stmt, 5);
        wallpaper.file_size = sqlite3_column_int64(stmt, 6);
        wallpaper.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        wallpaper.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        wallpaper.is_favorite = sqlite3_column_int(stmt, 9) != 0;
        wallpaper.rating = sqlite3_column_int(stmt, 10);
        wallpapers.push_back(wallpaper);
    }
    
    sqlite3_finalize(stmt);
    return wallpapers;
}

std::vector<Wallpaper> Database::search_wallpapers(const std::string& query) {
    std::vector<Wallpaper> wallpapers;
    
    const char* sql = "SELECT * FROM wallpapers WHERE name LIKE ? OR tags LIKE ? ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return wallpapers;
    }
    
    std::string search_pattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, search_pattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, search_pattern.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Wallpaper wallpaper;
        wallpaper.id = sqlite3_column_int(stmt, 0);
        wallpaper.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        wallpaper.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        wallpaper.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        wallpaper.width = sqlite3_column_int(stmt, 4);
        wallpaper.height = sqlite3_column_int(stmt, 5);
        wallpaper.file_size = sqlite3_column_int64(stmt, 6);
        wallpaper.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        wallpaper.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        wallpaper.is_favorite = sqlite3_column_int(stmt, 9) != 0;
        wallpaper.rating = sqlite3_column_int(stmt, 10);
        wallpapers.push_back(wallpaper);
    }
    
    sqlite3_finalize(stmt);
    return wallpapers;
}

std::vector<Wallpaper> Database::get_wallpapers_by_tag(const std::string& tag) {
    std::vector<Wallpaper> wallpapers;
    
    const char* sql = "SELECT * FROM wallpapers WHERE tags LIKE ? ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return wallpapers;
    }
    
    std::string search_pattern = "%" + tag + "%";
    sqlite3_bind_text(stmt, 1, search_pattern.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Wallpaper wallpaper;
        wallpaper.id = sqlite3_column_int(stmt, 0);
        wallpaper.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        wallpaper.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        wallpaper.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        wallpaper.width = sqlite3_column_int(stmt, 4);
        wallpaper.height = sqlite3_column_int(stmt, 5);
        wallpaper.file_size = sqlite3_column_int64(stmt, 6);
        wallpaper.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        wallpaper.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        wallpaper.is_favorite = sqlite3_column_int(stmt, 9) != 0;
        wallpaper.rating = sqlite3_column_int(stmt, 10);
        wallpapers.push_back(wallpaper);
    }
    
    sqlite3_finalize(stmt);
    return wallpapers;
}

std::vector<Wallpaper> Database::get_favorite_wallpapers() {
    std::vector<Wallpaper> wallpapers;
    
    const char* sql = "SELECT * FROM wallpapers WHERE is_favorite = 1 ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return wallpapers;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Wallpaper wallpaper;
        wallpaper.id = sqlite3_column_int(stmt, 0);
        wallpaper.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        wallpaper.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        wallpaper.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        wallpaper.width = sqlite3_column_int(stmt, 4);
        wallpaper.height = sqlite3_column_int(stmt, 5);
        wallpaper.file_size = sqlite3_column_int64(stmt, 6);
        wallpaper.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        wallpaper.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        wallpaper.is_favorite = sqlite3_column_int(stmt, 9) != 0;
        wallpaper.rating = sqlite3_column_int(stmt, 10);
        wallpapers.push_back(wallpaper);
    }
    
    sqlite3_finalize(stmt);
    return wallpapers;
}

std::vector<std::string> Database::get_all_tags() {
    std::vector<std::string> tags;
    std::vector<Wallpaper> wallpapers = get_all_wallpapers();
    
    for (const auto& wp : wallpapers) {
        if (!wp.tags.empty()) {
            std::stringstream ss(wp.tags);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                // Trim whitespace
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    tag = tag.substr(start, end - start + 1);
                    if (!tag.empty()) {
                        tags.push_back(tag);
                    }
                }
            }
        }
    }
    
    // Remove duplicates
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    
    return tags;
}

bool Database::add_tag_to_wallpaper(int wallpaper_id, const std::string& tag) {
    Wallpaper wp = get_wallpaper(wallpaper_id);
    if (wp.id == -1) return false;
    
    // Check if tag already exists
    if (wp.tags.find(tag) != std::string::npos) {
        return true; // Already exists
    }
    
    // Add tag
    if (wp.tags.empty()) {
        wp.tags = tag;
    } else {
        wp.tags += "," + tag;
    }
    
    return update_wallpaper(wp);
}

bool Database::remove_tag_from_wallpaper(int wallpaper_id, const std::string& tag) {
    Wallpaper wp = get_wallpaper(wallpaper_id);
    if (wp.id == -1) return false;
    
    // Remove tag
    size_t pos = wp.tags.find(tag);
    if (pos == std::string::npos) {
        return true; // Tag doesn't exist
    }
    
    // Remove the tag and any surrounding commas
    size_t start = pos;
    size_t length = tag.length();
    
    // Check for preceding comma
    if (start > 0 && wp.tags[start - 1] == ',') {
        start--;
        length++;
    }
    // Check for trailing comma
    else if (start + tag.length() < wp.tags.length() && wp.tags[start + tag.length()] == ',') {
        length++;
    }
    
    wp.tags.erase(start, length);
    
    // Trim any leading/trailing commas
    while (!wp.tags.empty() && wp.tags[0] == ',') wp.tags.erase(0, 1);
    while (!wp.tags.empty() && wp.tags.back() == ',') wp.tags.pop_back();
    
    return update_wallpaper(wp);
}

bool Database::set_setting(const std::string& key, const std::string& value) {
    const char* sql = R"(
        INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::string Database::get_setting(const std::string& key, const std::string& default_value) {
    const char* sql = "SELECT value FROM settings WHERE key = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return default_value;
    }
    
    sqlite3_bind_int(stmt, 1, 1);
    
    std::string value = default_value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    
    sqlite3_finalize(stmt);
    return value;
}
