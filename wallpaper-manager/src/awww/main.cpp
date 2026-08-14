// awww-win.cpp - Wallpaper manager for Windows
// Usage: awww-win [command] [options]
// Commands:
//   add PATH     Add wallpaper(s) to database
//   list         List all wallpapers
//   set ID       Set wallpaper by ID
//   search QUERY Search wallpapers
//   tag ID TAG   Add tag to wallpaper
//   fav ID       Toggle favorite
//   import DIR   Import all images from directory
//   random       Set random wallpaper

#include "awww.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <cstring>

void print_help() {
    std::cout << "awww-win - Wallpaper Manager for Windows\n\n";
    std::cout << "Usage: awww-win [command] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  add PATH           Add wallpaper image to database\n";
    std::cout << "  list               List all wallpapers\n";
    std::cout << "  set ID             Set wallpaper by database ID\n";
    std::cout << "  set PATH           Set wallpaper by file path\n";
    std::cout << "  search QUERY       Search wallpapers by name or tags\n";
    std::cout << "  tag ID TAG         Add tag to wallpaper\n";
    std::cout << "  untag ID TAG       Remove tag from wallpaper\n";
    std::cout << "  fav ID             Toggle favorite status\n";
    std::cout << "  favorites          List favorite wallpapers\n";
    std::cout << "  rating ID RATING   Set rating (0-5)\n";
    std::cout << "  import DIR         Import all images from directory\n";
    std::cout << "  random             Set random wallpaper\n";
    std::cout << "  tags               List all tags\n";
    std::cout << "  init               Initialize database\n";
    std::cout << "  help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  awww-win init                          # Initialize database\n";
    std::cout << "  awww-win add C:\\Wallpapers\\cool.jpg   # Add wallpaper\n";
    std::cout << "  awww-win list                          # List all wallpapers\n";
    std::cout << "  awww-win set 5                         # Set wallpaper with ID 5\n";
    std::cout << "  awww-win search nature                 # Search for 'nature'\n";
    std::cout << "  awww-win tag 3 landscape               # Add 'landscape' tag to ID 3\n";
    std::cout << "  awww-win fav 7                         # Toggle favorite for ID 7\n";
    std::cout << "  awww-win import C:\\Wallpapers          # Import all images\n";
    std::cout << "  awww-win random                        # Set random wallpaper\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        print_help();
        return 0;
    }
    
    // Get database path
    std::string db_path = utils::get_app_data_dir() + "\\wallpapers.db";
    utils::ensure_directory_exists(utils::get_app_data_dir());
    
    AwwwWin awww;
    
    // Initialize command doesn't need existing DB
    if (cmd != "init") {
        if (!awww.initialize(db_path)) {
            std::cerr << "Error: Failed to initialize database. Run 'awww-win init' first.\n";
            return 1;
        }
    }
    
    if (cmd == "init") {
        if (awww.initialize(db_path)) {
            std::cout << "Database initialized at: " << db_path << "\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to initialize database.\n";
            return 1;
        }
    }
    
    else if (cmd == "add") {
        if (argc < 3) {
            std::cerr << "Error: Please provide a file path.\n";
            return 1;
        }
        
        std::string path = argv[2];
        if (awww.add_wallpaper(path)) {
            std::cout << "Added wallpaper: " << path << "\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to add wallpaper.\n";
            return 1;
        }
    }
    
    else if (cmd == "list") {
        auto wallpapers = awww.get_all_wallpapers();
        
        if (wallpapers.empty()) {
            std::cout << "No wallpapers in database.\n";
            return 0;
        }
        
        std::cout << "Wallpapers (" << wallpapers.size() << " total):\n\n";
        for (const auto& wp : wallpapers) {
            std::cout << "[" << wp.path << "] " 
                      << wp.name 
                      << " (" << wp.width << "x" << wp.height << ")"
                      << (wp.is_favorite ? " ★" : "")
                      << (wp.rating > 0 ? " [" + std::to_string(wp.rating) + "/5]" : "")
                      << "\n";
            if (!wp.tags.empty()) {
                std::cout << "  Tags: ";
                for (size_t i = 0; i < wp.tags.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << wp.tags[i];
                }
                std::cout << "\n";
            }
        }
        return 0;
    }
    
    else if (cmd == "set") {
        if (argc < 3) {
            std::cerr << "Error: Please provide wallpaper ID or path.\n";
            return 1;
        }
        
        std::string arg = argv[2];
        std::string path;
        
        // Check if it's a number (ID) or path
        if (std::isdigit(arg[0])) {
            int id = std::stoi(arg);
            auto wallpapers = awww.get_all_wallpapers();
            
            bool found = false;
            for (const auto& wp : wallpapers) {
                // Simple ID matching based on position
                static int current_id = 0;
                current_id++;
                if (current_id == id) {
                    path = wp.path;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::cerr << "Error: Wallpaper with ID " << id << " not found.\n";
                return 1;
            }
        } else {
            path = arg;
        }
        
        if (awww.set_wallpaper(path)) {
            std::cout << "Wallpaper set: " << path << "\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to set wallpaper.\n";
            return 1;
        }
    }
    
    else if (cmd == "search") {
        if (argc < 3) {
            std::cerr << "Error: Please provide a search query.\n";
            return 1;
        }
        
        std::string query = argv[2];
        auto wallpapers = awww.search(query);
        
        if (wallpapers.empty()) {
            std::cout << "No wallpapers found matching '" << query << "'.\n";
            return 0;
        }
        
        std::cout << "Search results for '" << query << "' (" << wallpapers.size() << " found):\n\n";
        for (const auto& wp : wallpapers) {
            std::cout << wp.path << "\n";
            std::cout << "  " << wp.name << " (" << wp.width << "x" << wp.height << ")\n";
        }
        return 0;
    }
    
    else if (cmd == "fav" || cmd == "favorite") {
        if (argc < 3) {
            std::cerr << "Error: Please provide wallpaper ID.\n";
            return 1;
        }
        
        int id = std::stoi(argv[2]);
        if (awww.toggle_favorite(id)) {
            std::cout << "Toggled favorite for wallpaper " << id << ".\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to toggle favorite.\n";
            return 1;
        }
    }
    
    else if (cmd == "favorites") {
        auto wallpapers = awww.get_favorites();
        
        if (wallpapers.empty()) {
            std::cout << "No favorite wallpapers.\n";
            return 0;
        }
        
        std::cout << "Favorite wallpapers (" << wallpapers.size() << "):\n\n";
        for (const auto& wp : wallpapers) {
            std::cout << wp.path << " - " << wp.name << "\n";
        }
        return 0;
    }
    
    else if (cmd == "rating") {
        if (argc < 4) {
            std::cerr << "Error: Usage: rating ID RATING (0-5)\n";
            return 1;
        }
        
        int id = std::stoi(argv[2]);
        int rating = std::stoi(argv[3]);
        
        if (awww.set_rating(id, rating)) {
            std::cout << "Set rating " << rating << "/5 for wallpaper " << id << ".\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to set rating.\n";
            return 1;
        }
    }
    
    else if (cmd == "import") {
        if (argc < 3) {
            std::cerr << "Error: Please provide a directory path.\n";
            return 1;
        }
        
        std::string dir = argv[2];
        int count = awww.import_from_directory(dir, true);
        
        std::cout << "Imported " << count << " wallpapers from " << dir << ".\n";
        return 0;
    }
    
    else if (cmd == "random") {
        auto wallpapers = awww.get_all_wallpapers();
        
        if (wallpapers.empty()) {
            std::cerr << "Error: No wallpapers in database.\n";
            return 1;
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, wallpapers.size() - 1);
        
        const auto& wp = wallpapers[distrib(gen)];
        
        if (awww.set_wallpaper(wp.path)) {
            std::cout << "Random wallpaper set: " << wp.path << "\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to set wallpaper.\n";
            return 1;
        }
    }
    
    else if (cmd == "tags") {
        auto tags = awww.get_all_tags();
        
        if (tags.empty()) {
            std::cout << "No tags found.\n";
            return 0;
        }
        
        std::cout << "Tags (" << tags.size() << "):\n";
        for (const auto& tag : tags) {
            std::cout << "  " << tag << "\n";
        }
        return 0;
    }
    
    else {
        std::cerr << "Unknown command: " << cmd << "\n";
        print_help();
        return 1;
    }
}
