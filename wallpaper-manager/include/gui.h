#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include "database.h"

// Forward declare ImGui and DX11 types
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace gui {

// Thumbnail cache entry
struct ThumbnailEntry {
    int wallpaper_id;
    std::string path;
    ID3D11ShaderResourceView* srv;
    int width;
    int height;
    bool loaded;
    bool load_failed;
};

// Wallpaper GUI Application
class WallpaperGUI {
public:
    WallpaperGUI();
    ~WallpaperGUI();

    // Initialize the GUI application
    bool initialize(const std::string& db_path);

    // Run the main loop
    int run();

    // Cleanup
    void shutdown();

private:
    // Database
    Database db;
    bool initialized;
    std::string db_path;

    // Window dimensions
    int window_width;
    int window_height;

    // Grid layout settings
    float thumbnail_size;
    float grid_spacing;
    int columns;

    // Scroll position
    float scroll_y;

    // Selected wallpaper
    int selected_id;
    int hovered_id;

    // Context menu state
    bool show_context_menu;
    int context_menu_id;

    // Search filter
    char search_buffer[256];
    bool show_favorites_only;
    bool show_tags_filter;
    std::string current_tag_filter;

    // Thumbnails cache (lazy loaded)
    std::vector<std::unique_ptr<ThumbnailEntry>> thumbnails;

    // DirectX 11 resources (using ComPtr for automatic reference counting)
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;

    // Window handles
    HWND hwnd;

    // Methods
    bool init_d3d(HWND hwnd);
    void cleanup_d3d();
    void render_frame();
    void render_wallpaper_grid();
    void render_thumbnail(const ThumbnailEntry& thumb, int index);
    void render_sidebar();
    void render_context_menu();
    void load_thumbnails();
    void unload_all_thumbnails();
    ThumbnailEntry* get_or_load_thumbnail(int wallpaper_id);
    ID3D11ShaderResourceView* create_texture_from_file(const std::string& path, int& out_width, int& out_height);
    void set_wallpaper(const std::string& path);
    void delete_wallpaper(int id);
    void toggle_favorite(int id);
    void add_tag_dialog(int id);
    std::vector<Wallpaper> get_filtered_wallpapers();
};

} // namespace gui

#endif // GUI_H
