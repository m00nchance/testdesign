#ifndef GRIM_H
#define GRIM_H

#include <string>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>

struct MonitorInfo {
    std::string name;
    std::string device_name;
    int left;
    int top;
    int width;
    int height;
    bool is_primary;
};

class GrimWin {
public:
    GrimWin();
    ~GrimWin();
    
    // Initialize DirectX and desktop duplication
    bool initialize();
    
    // Get list of available monitors
    std::vector<MonitorInfo> get_monitors();
    
    // Capture full screen of specified monitor (0 = all monitors)
    bool capture_screen(const std::string& output_path, int monitor_index = 0);
    
    // Capture specific region
    bool capture_region(const std::string& output_path, int x, int y, int width, int height);
    
    // Capture to memory buffer
    bool capture_to_buffer(std::vector<uint8_t>& buffer, int& out_width, int& out_height, int monitor_index = 0);
    
    // Get last error message
    std::string get_last_error() const;
    
private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGIOutputDuplication* duplication;
    bool initialized;
    std::string last_error;
    
    bool init_d3d11();
    bool init_desktop_duplication(int monitor_index);
    void cleanup();
    bool save_texture_to_file(ID3D11Texture2D* texture, const std::string& path);
    bool crop_and_save(ID3D11Texture2D* texture, const std::string& path, int x, int y, int width, int height);
};

#endif // GRIM_H
