// gui.cpp - Wallpaper GUI Selector using Dear ImGui + Win32/DX11
#include "gui.h"
#include "wallpaper_engine.h"
#include "utils.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <algorithm>
#include <filesystem>
#include <sstream>

// Dear ImGui headers (will be included from vendor)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// stb_image for loading images
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")

using Microsoft::WRL::ComPtr;

namespace gui {

// Global pointers for window procedure
static WallpaperGUI* g_gui_instance = nullptr;

// Window procedure callback
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_gui_instance && wParam != SIZE_MINIMIZED) {
            // Handle resize if needed
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

WallpaperGUI::WallpaperGUI() 
    : initialized(false)
    , window_width(1280)
    , window_height(720)
    , thumbnail_size(200.0f)
    , grid_spacing(15.0f)
    , columns(4)
    , scroll_y(0.0f)
    , selected_id(-1)
    , hovered_id(-1)
    , show_context_menu(false)
    , context_menu_id(-1)
    , hwnd(nullptr)
{
    search_buffer[0] = '\0';
    show_favorites_only = false;
}

WallpaperGUI::~WallpaperGUI() {
    shutdown();
}

bool WallpaperGUI::initialize(const std::string& path) {
    db_path = path;
    if (!db.initialize(path)) {
        return false;
    }
    initialized = true;

    // Initialize search buffer
    memset(search_buffer, 0, sizeof(search_buffer));

    // Register window class
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"awww-win GUI", nullptr };
    ::RegisterClassExW(&wc);

    // Create window
    hwnd = ::CreateWindowW(wc.lpszClassName, L"awww-win - Wallpaper Selector", WS_OVERLAPPEDWINDOW,
                           100, 100, window_width, window_height, nullptr, nullptr, wc.hInstance, nullptr);

    // Store instance pointer for window procedure
    g_gui_instance = this;

    // Initialize Direct3D
    if (!init_d3d(hwnd)) {
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d_device.Get(), d3d_context.Get());

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ItemSpacing = ImVec2(8, 8);

    // Show window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    return true;
}

int WallpaperGUI::run() {
    if (!initialized) return -1;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (true) {
        // Process Windows messages
        if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }

        // Start new frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render the UI
        render_frame();

        // Render
        ImGui::Render();
        d3d_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        swap_chain->Present(1, 0);
    }

    return 0;
}

void WallpaperGUI::shutdown() {
    // Cleanup ImGui
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Cleanup thumbnails
    unload_all_thumbnails();

    // Cleanup D3D
    cleanup_d3d();

    // Destroy window
    if (hwnd) {
        ::DestroyWindow(hwnd);
        hwnd = nullptr;
    }

    // Close database
    db.close();

    initialized = false;
}

bool WallpaperGUI::init_d3d(HWND hwnd) {
    // Create device and context
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                   D3D11_SDK_VERSION, &d3d_device, &featureLevel, &d3d_context);
    if (FAILED(hr)) {
        return false;
    }

    // Get DXGI device
    ComPtr<IDXGIDevice> dxgi_device;
    hr = d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    // Get adapter
    ComPtr<IDXGIAdapter> dxgi_adapter;
    hr = dxgi_device->GetAdapter(dxgi_adapter.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Get output (monitor)
    ComPtr<IDXGIOutput> dxgi_output;
    hr = dxgi_adapter->EnumOutputs(0, dxgi_output.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Get output description for dimensions
    DXGI_OUTPUT_DESC output_desc;
    dxgi_output->GetDesc(&output_desc);

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = window_width;
    sd.BufferDesc.Height = window_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<IDXGIFactory> dxgi_factory;
    hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgi_factory.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgi_factory->CreateSwapChain(d3d_device, &sd, swap_chain.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Create render target view
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = d3d_device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void WallpaperGUI::cleanup_d3d() {
    render_target_view.Reset();
    swap_chain.Reset();
    d3d_context.Reset();
    d3d_device.Reset();
}

std::vector<Wallpaper> WallpaperGUI::get_filtered_wallpapers() {
    auto all = db.get_all_wallpapers();
    std::vector<Wallpaper> filtered;

    for (auto& wp : all) {
        bool include = true;

        // Filter by favorites
        if (show_favorites_only && !wp.is_favorite) {
            include = false;
        }

        // Filter by search query
        if (include && search_buffer[0] != '\0') {
            std::string query = search_buffer;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            
            std::string name_lower = wp.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            
            std::string tags_lower = wp.tags;
            std::transform(tags_lower.begin(), tags_lower.end(), tags_lower.begin(), ::tolower);

            if (name_lower.find(query) == std::string::npos && 
                tags_lower.find(query) == std::string::npos) {
                include = false;
            }
        }

        // Filter by tag
        if (include && !current_tag_filter.empty()) {
            if (wp.tags.find(current_tag_filter) == std::string::npos) {
                include = false;
            }
        }

        if (include) {
            filtered.push_back(wp);
        }
    }

    return filtered;
}

void WallpaperGUI::render_frame() {
    // Main window fills the screen
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("Wallpaper Browser", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_MenuBar);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Directory")) {
                // Could open folder dialog here
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Favorites Only", nullptr, &show_favorites_only);
            ImGui::Separator();
            
            // Thumbnail size slider
            float size = thumbnail_size;
            if (ImGui::SliderFloat("Thumbnail Size", &size, 100.0f, 400.0f)) {
                thumbnail_size = size;
            }
            
            // Columns slider
            int cols = columns;
            if (ImGui::SliderInt("Columns", &cols, 2, 10)) {
                columns = cols;
            }
            
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Split into sidebar and main content
    ImGui::BeginChild("Sidebar", ImVec2(250, 0), true);
    render_sidebar();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("MainContent", ImVec2(0, 0), true);
    render_wallpaper_grid();
    ImGui::EndChild();

    ImGui::End();

    // Render context menu if needed
    if (show_context_menu) {
        render_context_menu();
    }
}

void WallpaperGUI::render_sidebar() {
    ImGui::Text("Filters");
    ImGui::Separator();
    ImGui::Spacing();

    // Search box
    ImGui::Text("Search:");
    if (ImGui::InputText("##Search", search_buffer, sizeof(search_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Search on enter
    }
    if (ImGui::Button("Clear Search", ImVec2(-1, 0))) {
        search_buffer[0] = '\0';
    }
    ImGui::Spacing();

    // Quick filters
    ImGui::Checkbox("Favorites Only", &show_favorites_only);
    ImGui::Spacing();

    // Tags
    ImGui::Text("Tags:");
    ImGui::Separator();
    
    auto tags = db.get_all_tags();
    if (tags.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No tags");
    } else {
        // Clear filter button
        if (ImGui::Button("Clear", ImVec2(-1, 25))) {
            current_tag_filter.clear();
        }
        
        for (const auto& tag : tags) {
            bool is_selected = (current_tag_filter == tag);
            if (ImGui::Selectable(tag.c_str(), is_selected)) {
                if (is_selected) {
                    current_tag_filter.clear();
                } else {
                    current_tag_filter = tag;
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Stats
    auto wallpapers = get_filtered_wallpapers();
    ImGui::Text("Showing: %d wallpapers", (int)wallpapers.size());
    
    auto all = db.get_all_wallpapers();
    ImGui::Text("Total: %d wallpapers", (int)all.size());
}

void WallpaperGUI::render_wallpaper_grid() {
    auto wallpapers = get_filtered_wallpapers();

    if (wallpapers.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No wallpapers found.");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Use 'awww-win import <directory>' to add wallpapers.");
        return;
    }

    float available_width = ImGui::GetContentRegionAvail().x;
    int actual_columns = std::max(1, (int)((available_width + grid_spacing) / (thumbnail_size + grid_spacing)));
    
    float cell_width = (available_width - (actual_columns + 1) * grid_spacing) / actual_columns;
    float cell_height = cell_width * 0.75f + 40.0f; // 4:3 aspect ratio + space for info

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(grid_spacing, grid_spacing));

    for (size_t i = 0; i < wallpapers.size(); i++) {
        if (i > 0 && i % actual_columns != 0) {
            ImGui::SameLine();
        }

        const auto& wp = wallpapers[i];
        
        // Get or load thumbnail
        ThumbnailEntry* thumb = get_or_load_thumbnail(wp.id);

        // Calculate item size
        ImVec2 item_size(cell_width, cell_height);
        
        // Begin child for each wallpaper
        ImGui::BeginChild(("##thumb_" + std::to_string(wp.id)).c_str(), item_size, true, 
                          ImGuiWindowFlags_NoScrollbar);

        // Check for clicks
        bool hovered = ImGui::IsItemHovered();
        if (hovered) {
            hovered_id = wp.id;
            
            // Right-click for context menu
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                context_menu_id = wp.id;
                show_context_menu = true;
            }
        }

        // Double-click to set wallpaper
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered) {
            set_wallpaper(wp.path);
        }

        // Render thumbnail or placeholder
        if (thumb && thumb->loaded && thumb->srv) {
            // Render image
            ImVec2 img_size(cell_width, cell_width * 0.75f);
            ImGui::Image(thumb->srv, img_size);
        } else {
            // Placeholder
            ImVec2 img_size(cell_width, cell_width * 0.75f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            
            // Background rectangle
            draw_list->AddRectFilled(pos, ImVec2(pos.x + img_size.x, pos.y + img_size.y), 
                                     IM_COL32(40, 40, 40, 255));
            
            // Loading indicator or error icon
            if (thumb && thumb->load_failed) {
                // Error icon (simple X)
                float cx = pos.x + img_size.x / 2;
                float cy = pos.y + img_size.y / 2;
                float s = img_size.x * 0.3f;
                draw_list->AddLine(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), IM_COL32(200, 50, 50, 255), 2.0f);
                draw_list->AddLine(ImVec2(cx + s, cy - s), ImVec2(cx - s, cy + s), IM_COL32(200, 50, 50, 255), 2.0f);
            } else {
                // Loading spinner or simple text
                float cx = pos.x + img_size.x / 2;
                float cy = pos.y + img_size.y / 2;
                draw_list->AddText(ImVec2(cx - 30, cy - 10), IM_COL32(150, 150, 150, 255), "Loading...");
            }
            
            ImGui::Dummy(img_size);
        }

        // Info section
        ImGui::Spacing();
        
        // Filename (truncated)
        std::string display_name = wp.name;
        if (display_name.length() > 20) {
            display_name = display_name.substr(0, 17) + "...";
        }
        ImGui::Text("%s", display_name.c_str());

        // Dimensions
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%dx%d", wp.width, wp.height);

        // Rating stars
        if (wp.rating > 0) {
            std::string stars(wp.rating, '*');
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", stars.c_str());
        }

        // Favorite indicator
        if (wp.is_favorite) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "★");
        }

        ImGui::EndChild();
    }

    ImGui::PopStyleVar();
}

void WallpaperGUI::render_context_menu() {
    if (!show_context_menu || context_menu_id == -1) return;

    auto wallpapers = get_filtered_wallpapers();
    Wallpaper* wp = nullptr;
    for (auto& w : wallpapers) {
        if (w.id == context_menu_id) {
            wp = &w;
            break;
        }
    }

    if (!wp) {
        show_context_menu = false;
        return;
    }

    ImGui::OpenPopup("WallpaperContextMenu");
    
    if (ImGui::BeginPopup("WallpaperContextMenu")) {
        ImGui::Text("'%s'", wp->name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Set as Wallpaper")) {
            set_wallpaper(wp->path);
            show_context_menu = false;
        }

        if (ImGui::MenuItem(wp->is_favorite ? "Remove from Favorites" : "Add to Favorites")) {
            toggle_favorite(context_menu_id);
            show_context_menu = false;
        }

        // Rating submenu
        if (ImGui::BeginMenu("Rating")) {
            for (int r = 0; r <= 5; r++) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d %s", r, r == 1 ? "Star" : "Stars");
                if (ImGui::MenuItem(buf, nullptr, wp->rating == r)) {
                    // Set rating would need DB method
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Add Tag...")) {
            add_tag_dialog(context_menu_id);
            show_context_menu = false;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete", nullptr, false, true)) {
            delete_wallpaper(context_menu_id);
            show_context_menu = false;
        }

        ImGui::EndPopup();
    }

    // Close context menu if clicked outside
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        show_context_menu = false;
    }
}

ThumbnailEntry* WallpaperGUI::get_or_load_thumbnail(int wallpaper_id) {
    // Check if already cached
    for (auto& thumb : thumbnails) {
        if (thumb->wallpaper_id == wallpaper_id) {
            return thumb.get();
        }
    }

    // Find wallpaper in DB
    auto wallpapers = db.get_all_wallpapers();
    Wallpaper* wp = nullptr;
    for (auto& w : wallpapers) {
        if (w.id == wallpaper_id) {
            wp = &w;
            break;
        }
    }

    if (!wp) return nullptr;

    // Create new thumbnail entry
    auto thumb = std::make_unique<ThumbnailEntry>();
    thumb->wallpaper_id = wallpaper_id;
    thumb->path = wp->path;
    thumb->srv = nullptr;
    thumb->width = 0;
    thumb->height = 0;
    thumb->loaded = false;
    thumb->load_failed = false;

    // Load texture synchronously (for simplicity - could be async)
    thumb->srv = create_texture_from_file(wp->path, thumb->width, thumb->height);
    thumb->loaded = (thumb->srv != nullptr);
    thumb->load_failed = !thumb->loaded;

    ThumbnailEntry* result = thumb.get();
    thumbnails.push_back(std::move(thumb));
    
    return result;
}

ID3D11ShaderResourceView* WallpaperGUI::create_texture_from_file(const std::string& path, int& out_width, int& out_height) {
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    // Load image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        return nullptr;
    }

    out_width = width;
    out_height = height;

    // Create texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subresource = {};
    subresource.pSysMem = data;
    subresource.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = d3d_device->CreateTexture2D(&desc, &subresource, texture.GetAddressOf());
    
    stbi_image_free(data);

    if (FAILED(hr)) {
        return nullptr;
    }

    // Create shader resource view
    ID3D11ShaderResourceView* srv = nullptr;
    hr = d3d_device->CreateShaderResourceView(texture.Get(), nullptr, &srv);
    if (FAILED(hr)) {
        return nullptr;
    }

    return srv;
}

void WallpaperGUI::set_wallpaper(const std::string& path) {
    WallpaperEngine engine;
    if (engine.set_wallpaper(path)) {
        // Show success notification (could use ImGui toast)
    }
}

void WallpaperGUI::delete_wallpaper(int id) {
    // Get wallpaper path first
    auto wp = db.get_wallpaper(id);
    if (wp.id != -1) {
        // Remove from database
        db.delete_wallpaper(id);
        
        // Remove thumbnail from cache
        for (auto it = thumbnails.begin(); it != thumbnails.end(); ) {
            if ((*it)->wallpaper_id == id) {
                if ((*it)->srv) {
                    (*it)->srv->Release();
                }
                it = thumbnails.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void WallpaperGUI::toggle_favorite(int id) {
    auto wp = db.get_wallpaper(id);
    if (wp.id != -1) {
        wp.is_favorite = !wp.is_favorite;
        db.update_wallpaper(wp);
    }
}

void WallpaperGUI::add_tag_dialog(int id) {
    // Simple implementation - in production would use ImGui input popup
    // For now, just a placeholder
}

void WallpaperGUI::unload_all_thumbnails() {
    for (auto& thumb : thumbnails) {
        if (thumb->srv) {
            thumb->srv->Release();
            thumb->srv = nullptr;
        }
    }
    thumbnails.clear();
}

} // namespace gui
