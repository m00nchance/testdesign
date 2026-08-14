#include "utils.h"
#include <windows.h>
#include <shlobj.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

#pragma comment(lib, "shell32.lib")

namespace utils {

std::string get_executable_path() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::filesystem::path exe_path(path);
    return exe_path.parent_path().string();
}

std::string get_pictures_dir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, 0, path))) {
        return wstring_to_string(path);
    }
    return get_executable_path() + "\\Pictures";
}

std::string get_app_data_dir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring app_data(path);
        return wstring_to_string(app_data) + "\\WallpaperManager";
    }
    return get_executable_path() + "\\AppData";
}

bool ensure_directory_exists(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

std::string generate_filename(const std::string& prefix, const std::string& extension) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << prefix << "_";
    
    // Add timestamp
    struct tm buf;
    localtime_s(&buf, &time_t_now);
    ss << std::put_time(&buf, "%Y%m%d_%H%M%S");
    
    // Add random suffix to avoid collisions
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1000, 9999);
    ss << "_" << distrib(gen);
    
    ss << "." << extension;
    return ss.str();
}

std::string get_timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    
    struct tm buf;
    localtime_s(&buf, &time_t_now);
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    
    return ss.str();
}

std::string wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) return L"";
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

uint64_t get_file_size(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (...) {
        return 0;
    }
}

bool delete_file(const std::string& path) {
    try {
        return std::filesystem::remove(path);
    } catch (...) {
        return false;
    }
}

bool copy_file(const std::string& src, const std::string& dst) {
    try {
        return std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> get_files_in_directory(const std::string& dir, const std::string& extension) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                if (extension.empty() || entry.path().extension().string() == extension) {
                    files.push_back(entry.path().string());
                }
            }
        }
    } catch (...) {
        // Directory doesn't exist or access error
    }
    return files;
}

} // namespace utils
