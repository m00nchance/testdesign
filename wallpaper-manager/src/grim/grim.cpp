#include "grim.h"
#include "utils.h"

#ifdef _WIN32
    #include <windows.h>
    #include <d3d11.h>
    #include <dxgi1_2.h>
    #include <wrl/client.h>
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "dxgi.lib")
    using Microsoft::WRL::ComPtr;
#endif

#include <fstream>
#include <sstream>

GrimWin::GrimWin() 
    : initialized(false)
#ifdef _WIN32
    , device(nullptr)
    , context(nullptr)
    , duplication(nullptr)
#endif
{}

GrimWin::~GrimWin() {
    cleanup();
}

bool GrimWin::initialize() {
#ifdef _WIN32
    if (initialized) return true;
    
    if (!init_d3d11()) {
        last_error = "Failed to initialize Direct3D 11";
        return false;
    }
    
    initialized = true;
    return true;
#else
    last_error = "GrimWin is only supported on Windows";
    return false;
#endif
}

#ifdef _WIN32
bool GrimWin::init_d3d11() {
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    
    D3D_FEATURE_LEVEL feature_level;
    
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        &device,
        &feature_level,
        &context
    );
    
    if (FAILED(hr)) {
        last_error = "D3D11CreateDevice failed: " + std::to_string(hr);
        return false;
    }
    
    return true;
}

bool GrimWin::init_desktop_duplication(int monitor_index) {
    if (!device) {
        last_error = "Direct3D device not initialized";
        return false;
    }
    
    ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.GetAddressOf()));
    if (FAILED(hr)) {
        last_error = "Failed to get DXGI device";
        return false;
    }
    
    ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(&adapter);
    if (FAILED(hr)) {
        last_error = "Failed to get DXGI adapter";
        return false;
    }
    
    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(monitor_index, &output);
    if (FAILED(hr)) {
        last_error = "Failed to enumerate outputs for monitor " + std::to_string(monitor_index);
        return false;
    }
    
    ComPtr<IDXGIOutput1> output1;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(output1.GetAddressOf()));
    if (FAILED(hr)) {
        last_error = "Failed to get IDXGIOutput1";
        return false;
    }
    
    hr = output1->DuplicateOutput(device, &duplication);
    if (FAILED(hr)) {
        last_error = "Desktop duplication not supported or already in use";
        return false;
    }
    
    return true;
}

void GrimWin::cleanup() {
    if (duplication) {
        duplication->ReleaseFrame();
        duplication->Release();
        duplication = nullptr;
    }
    
    if (context) {
        context->Release();
        context = nullptr;
    }
    
    if (device) {
        device->Release();
        device = nullptr;
    }
    
    initialized = false;
}

std::vector<MonitorInfo> GrimWin::get_monitors() {
    std::vector<MonitorInfo> monitors;
    
    if (!initialized && !initialize()) {
        return monitors;
    }
    
    ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.GetAddressOf()));
    if (FAILED(hr)) {
        return monitors;
    }
    
    ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(&adapter);
    if (FAILED(hr)) {
        return monitors;
    }
    
    ComPtr<IDXGIOutput> output;
    UINT output_index = 0;
    
    while (adapter->EnumOutputs(output_index, &output) != DXGI_ERROR_NOT_FOUND) {
        DXGI_OUTPUT_DESC desc;
        hr = output->GetDesc(&desc);
        
        if (SUCCEEDED(hr)) {
            MonitorInfo info;
            info.name = utils::wstring_to_string(desc.DeviceName);
            info.device_name = utils::wstring_to_string(desc.DeviceName);
            info.left = desc.DesktopCoordinates.left;
            info.top = desc.DesktopCoordinates.top;
            info.width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
            info.height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
            info.is_primary = (output_index == 0);
            
            monitors.push_back(info);
        }
        
        output.Reset();
        output_index++;
    }
    
    // If no monitors found via DXGI, fall back to EnumDisplayMonitors
    if (monitors.empty()) {
        EnumDisplayMonitors(NULL, NULL, [](HMONITOR hmon, HDC, LPRECT, LPARAM lParam) -> BOOL {
            auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(lParam);
            
            MONITORINFOEXW mi = {};
            mi.cbSize = sizeof(MONITORINFOEXW);
            
            if (GetMonitorInfoW(hmon, &mi)) {
                MonitorInfo info;
                info.device_name = utils::wstring_to_string(mi.szDevice);
                info.name = info.device_name;
                info.left = mi.rcMonitor.left;
                info.top = mi.rcMonitor.top;
                info.width = mi.rcMonitor.right - mi.rcMonitor.left;
                info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
                info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
                
                monitors->push_back(info);
            }
            
            return TRUE;
        }, reinterpret_cast<LPARAM>(&monitors));
    }
    
    return monitors;
}

bool GrimWin::capture_screen(const std::string& output_path, int monitor_index) {
    if (!initialized && !initialize()) {
        return false;
    }
    
    if (!init_desktop_duplication(monitor_index)) {
        return false;
    }
    
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    ComPtr<IDXGIResource> desktop_resource;
    
    HRESULT hr = duplication->AcquireNextFrame(5000, &frame_info, &desktop_resource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        last_error = "Frame acquisition timeout";
        return false;
    }
    if (FAILED(hr)) {
        last_error = "AcquireNextFrame failed: " + std::to_string(hr);
        return false;
    }
    
    ComPtr<ID3D11Texture2D> desktop_texture;
    hr = desktop_resource.As(&desktop_texture);
    if (FAILED(hr)) {
        last_error = "Failed to get texture from resource";
        duplication->ReleaseFrame();
        return false;
    }
    
    bool success = save_texture_to_file(desktop_texture.Get(), output_path);
    
    duplication->ReleaseFrame();
    return success;
}

bool GrimWin::capture_region(const std::string& output_path, int x, int y, int width, int height) {
    if (!initialized && !initialize()) {
        return false;
    }
    
    // Capture full screen first
    std::vector<uint8_t> buffer;
    int screen_width, screen_height;
    
    if (!capture_to_buffer(buffer, screen_width, screen_height, 0)) {
        return false;
    }
    
    // Validate region
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + width > screen_width) width = screen_width - x;
    if (y + height > screen_height) height = screen_height - y;
    
    if (width <= 0 || height <= 0) {
        last_error = "Invalid region dimensions";
        return false;
    }
    
    // Create cropped buffer (BGRA format)
    std::vector<uint8_t> cropped_buffer(width * height * 4);
    
    for (int row = 0; row < height; row++) {
        memcpy(
            &cropped_buffer[row * width * 4],
            &buffer[(y + row) * screen_width * 4 + x * 4],
            width * 4
        );
    }
    
    // Save as BMP
    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        last_error = "Cannot open output file: " + output_path;
        return false;
    }
    
    // BMP Header
    struct BMPHeader {
        uint16_t signature = 0x4D42; // 'BM'
        uint32_t file_size;
        uint16_t reserved1 = 0;
        uint16_t reserved2 = 0;
        uint32_t data_offset = 54;
    };
    
    struct BMPInfoHeader {
        uint32_t size = 40;
        int32_t img_width;
        int32_t img_height;
        uint16_t planes = 1;
        uint16_t bpp = 32;
        uint32_t compression = 0;
        uint32_t img_size;
        int32_t x_pixels_per_meter = 0;
        int32_t y_pixels_per_meter = 0;
        uint32_t colors_used = 0;
        uint32_t colors_important = 0;
    };
    
    BMPHeader bmp_header;
    BMPInfoHeader info_header;
    
    info_header.img_width = width;
    info_header.img_height = height;
    info_header.img_size = width * height * 4;
    
    bmp_header.file_size = 54 + info_header.img_size;
    
    file.write(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
    file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    file.write(reinterpret_cast<char*>(cropped_buffer.data()), cropped_buffer.size());
    
    return true;
}

bool GrimWin::capture_to_buffer(std::vector<uint8_t>& buffer, int& out_width, int& out_height, int monitor_index) {
    if (!initialized && !initialize()) {
        return false;
    }
    
    if (!init_desktop_duplication(monitor_index)) {
        return false;
    }
    
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    ComPtr<IDXGIResource> desktop_resource;
    
    HRESULT hr = duplication->AcquireNextFrame(5000, &frame_info, &desktop_resource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        last_error = "Frame acquisition timeout";
        return false;
    }
    if (FAILED(hr)) {
        last_error = "AcquireNextFrame failed: " + std::to_string(hr);
        return false;
    }
    
    ComPtr<ID3D11Texture2D> desktop_texture;
    hr = desktop_resource.As(&desktop_texture);
    if (FAILED(hr)) {
        last_error = "Failed to get texture from resource";
        duplication->ReleaseFrame();
        return false;
    }
    
    // Get texture description
    D3D11_TEXTURE2D_DESC desc;
    desktop_texture->GetDesc(&desc);
    
    out_width = desc.Width;
    out_height = desc.Height;
    
    // Create staging texture for CPU access
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = desc.Width;
    staging_desc.Height = desc.Height;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.Format = desc.Format;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    ComPtr<ID3D11Texture2D> staging_texture;
    hr = device->CreateTexture2D(&staging_desc, nullptr, &staging_texture);
    if (FAILED(hr)) {
        last_error = "Failed to create staging texture";
        duplication->ReleaseFrame();
        return false;
    }
    
    // Copy to staging texture
    context->CopyResource(staging_texture.Get(), desktop_texture.Get());
    
    // Map and read pixels
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        last_error = "Failed to map staging texture";
        duplication->ReleaseFrame();
        return false;
    }
    
    buffer.resize(desc.Width * desc.Height * 4);
    
    uint8_t* src = static_cast<uint8_t*>(mapped.pData);
    uint8_t* dst = buffer.data();
    
    for (UINT row = 0; row < desc.Height; row++) {
        memcpy(dst, src, desc.Width * 4);
        src += mapped.RowPitch;
        dst += desc.Width * 4;
    }
    
    context->Unmap(staging_texture.Get(), 0);
    duplication->ReleaseFrame();
    
    return true;
}

bool GrimWin::save_texture_to_file(ID3D11Texture2D* texture, const std::string& path) {
    if (!texture) {
        last_error = "Null texture provided";
        return false;
    }
    
    // Get texture description
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    
    // Create staging texture
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = desc.Width;
    staging_desc.Height = desc.Height;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.Format = desc.Format;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    ComPtr<ID3D11Texture2D> staging_texture;
    HRESULT hr = device->CreateTexture2D(&staging_desc, nullptr, &staging_texture);
    if (FAILED(hr)) {
        last_error = "Failed to create staging texture";
        return false;
    }
    
    // Copy texture to staging
    context->CopyResource(staging_texture.Get(), texture);
    
    // Map staging texture
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        last_error = "Failed to map staging texture";
        return false;
    }
    
    // Save as BMP
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        context->Unmap(staging_texture.Get(), 0);
        last_error = "Cannot open output file: " + path;
        return false;
    }
    
    // BMP Header
    struct BMPHeader {
        uint16_t signature = 0x4D42;
        uint32_t file_size;
        uint16_t reserved1 = 0;
        uint16_t reserved2 = 0;
        uint32_t data_offset = 54;
    };
    
    struct BMPInfoHeader {
        uint32_t size = 40;
        int32_t img_width;
        int32_t img_height;
        uint16_t planes = 1;
        uint16_t bpp = 32;
        uint32_t compression = 0;
        uint32_t img_size;
        int32_t x_pixels_per_meter = 0;
        int32_t y_pixels_per_meter = 0;
        uint32_t colors_used = 0;
        uint32_t colors_important = 0;
    };
    
    BMPHeader bmp_header;
    BMPInfoHeader info_header;
    
    info_header.img_width = desc.Width;
    info_header.img_height = desc.Height;
    info_header.img_size = desc.Width * desc.Height * 4;
    
    bmp_header.file_size = 54 + info_header.img_size;
    
    file.write(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
    file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    
    // Write pixel data (flip vertically for BMP)
    uint8_t* src = static_cast<uint8_t*>(mapped.pData);
    for (int row = desc.Height - 1; row >= 0; row--) {
        file.write(reinterpret_cast<char*>(src + row * mapped.RowPitch), desc.Width * 4);
    }
    
    context->Unmap(staging_texture.Get(), 0);
    
    return true;
}

std::string GrimWin::get_last_error() const {
    return last_error;
}

#endif // _WIN32
