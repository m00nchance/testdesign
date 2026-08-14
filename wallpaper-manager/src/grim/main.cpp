// grim-win.cpp - Screenshot utility for Windows
// Usage: grim-win [options] [output_file]
// Options:
//   -o FILE    Output file (default: screenshot_TIMESTAMP.bmp)
//   -m INDEX   Monitor index (default: 0)
//   -r X,Y WxH Capture region
//   -h         Show help

#include "grim.h"
#include "utils.h"
#include <iostream>
#include <cstring>

void print_help() {
    std::cout << "grim-win - Screenshot utility for Windows\n\n";
    std::cout << "Usage: grim-win [options] [output_file]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o FILE    Output file path (default: screenshot_TIMESTAMP.bmp)\n";
    std::cout << "  -m INDEX   Monitor index (0 = primary, default: 0)\n";
    std::cout << "  -r X,Y WxH Capture specific region\n";
    std::cout << "  -l         List available monitors\n";
    std::cout << "  -h         Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  grim-win                    # Capture primary monitor\n";
    std::cout << "  grim-win -o myshot.bmp      # Save to specific file\n";
    std::cout << "  grim-win -m 1               # Capture second monitor\n";
    std::cout << "  grim-win -r 100,100 800x600 # Capture region at (100,100) with size 800x600\n";
    std::cout << "  grim-win -l                 # List monitors\n";
}

int main(int argc, char* argv[]) {
    std::string output_file;
    int monitor_index = 0;
    bool list_monitors = false;
    bool region_mode = false;
    int region_x = 0, region_y = 0, region_w = 0, region_h = 0;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            list_monitors = true;
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            monitor_index = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-r") == 0 && i + 3 < argc) {
            region_mode = true;
            // Parse "X,Y" format
            std::string coords = argv[++i];
            size_t comma_pos = coords.find(',');
            region_x = std::stoi(coords.substr(0, comma_pos));
            region_y = std::stoi(coords.substr(comma_pos + 1));
            
            // Parse "WxH" format
            std::string dims = argv[++i];
            size_t x_pos = dims.find('x');
            region_w = std::stoi(dims.substr(0, x_pos));
            region_h = std::stoi(dims.substr(x_pos + 1));
        }
        else if (argv[i][0] != '-') {
            output_file = argv[i];
        }
    }
    
    GrimWin grim;
    
    // List monitors
    if (list_monitors) {
        auto monitors = grim.get_monitors();
        if (monitors.empty()) {
            std::cout << "No monitors found.\n";
            return 1;
        }
        
        std::cout << "Available monitors:\n";
        for (size_t i = 0; i < monitors.size(); i++) {
            std::cout << "  [" << i << "] " 
                      << monitors[i].name 
                      << " (" << monitors[i].width << "x" << monitors[i].height << ")"
                      << (monitors[i].is_primary ? " [PRIMARY]" : "")
                      << "\n";
        }
        return 0;
    }
    
    // Generate output filename if not provided
    if (output_file.empty()) {
        output_file = utils::get_pictures_dir() + "\\" + 
                     utils::generate_filename("screenshot", "bmp");
    }
    
    // Ensure output directory exists
    std::filesystem::path out_path(output_file);
    utils::ensure_directory_exists(out_path.parent_path().string());
    
    // Capture
    bool success;
    if (region_mode) {
        std::cout << "Capturing region (" << region_x << "," << region_y 
                  << ") " << region_w << "x" << region_h << "...\n";
        success = grim.capture_region(output_file, region_x, region_y, region_w, region_h);
    } else {
        std::cout << "Capturing monitor " << monitor_index << "...\n";
        success = grim.capture_screen(output_file, monitor_index);
    }
    
    if (success) {
        std::cout << "Screenshot saved to: " << output_file << "\n";
        return 0;
    } else {
        std::cerr << "Error: " << grim.get_last_error() << "\n";
        return 1;
    }
}
