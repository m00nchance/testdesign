#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem>

namespace utils {
    // Get current executable path
    std::string get_executable_path();
    
    // Get user's pictures directory
    std::string get_pictures_dir();
    
    // Get user's app data directory
    std::string get_app_data_dir();
    
    // Create directory if it doesn't exist
    bool ensure_directory_exists(const std::string& path);
    
    // Generate unique filename
    std::string generate_filename(const std::string& prefix, const std::string& extension);
    
    // Get current timestamp as string
    std::string get_timestamp_string();
    
    // Convert wide string to narrow string
    std::string wstring_to_string(const std::wstring& wstr);
    
    // Convert narrow string to wide string
    std::wstring string_to_wstring(const std::string& str);
    
    // Check if file exists
    bool file_exists(const std::string& path);
    
    // Get file size in bytes
    uint64_t get_file_size(const std::string& path);
    
    // Delete file
    bool delete_file(const std::string& path);
    
    // Copy file
    bool copy_file(const std::string& src, const std::string& dst);
    
    // Get list of files in directory with extension filter
    std::vector<std::string> get_files_in_directory(const std::string& dir, const std::string& extension = "");
}

#endif // UTILS_H
