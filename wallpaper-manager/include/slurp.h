#ifndef SLURP_H
#define SLURP_H

#include <string>

#ifdef _WIN32
    #include <windows.h>
#endif

struct Selection {
    int x;
    int y;
    int width;
    int height;
    bool valid;
};

class SlurpWin {
public:
    SlurpWin();
    ~SlurpWin();
    
    // Show region selector and return selection
    Selection select_region();
    
    // Get selection as slurp-compatible output string: "x,y widthxheight"
    std::string get_slurp_output(const Selection& selection);
    
    // Parse slurp output string to Selection
    Selection parse_slurp_output(const std::string& output);
    
private:
#ifdef _WIN32
    // Internal window procedure handler
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Create overlay window for selection
    bool create_overlay_window();
    
    // Handle mouse events
    void on_mouse_down(int x, int y);
    void on_mouse_move(int x, int y);
    void on_mouse_up(int x, int y);
    
    // Render the selection rectangle
    void render_selection(HDC hdc);
    
    HWND overlay_hwnd;
#endif
    bool selecting;
    int start_x, start_y;
    int current_x, current_y;
    Selection result;
    bool captured;
};

#endif // SLURP_H
