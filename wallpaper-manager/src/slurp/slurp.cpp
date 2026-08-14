#include "slurp.h"
#include <windows.h>
#include <sstream>
#include <algorithm>

// Define GET_X_LPARAM and GET_Y_LPARAM if not available
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// Global pointer for window procedure
static SlurpWin* g_slurp_instance = nullptr;

SlurpWin::SlurpWin()
    : overlay_hwnd(nullptr)
    , selecting(false)
    , start_x(0)
    , start_y(0)
    , current_x(0)
    , current_y(0)
    , captured(false)
{
    result.valid = false;
    result.x = 0;
    result.y = 0;
    result.width = 0;
    result.height = 0;
}

SlurpWin::~SlurpWin() {
    if (overlay_hwnd) {
        DestroyWindow(overlay_hwnd);
    }
}

Selection SlurpWin::select_region() {
    result.valid = false;
    captured = false;
    
    if (!create_overlay_window()) {
        return result;
    }
    
    // Show the window
    ShowWindow(overlay_hwnd, SW_SHOW);
    SetForegroundWindow(overlay_hwnd);
    SetCapture(overlay_hwnd);
    
    // Set global instance for window procedure
    g_slurp_instance = this;
    
    // Message loop
    MSG msg;
    while (!captured && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    ReleaseCapture();
    DestroyWindow(overlay_hwnd);
    overlay_hwnd = nullptr;
    g_slurp_instance = nullptr;
    
    return result;
}

bool SlurpWin::create_overlay_window() {
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"SlurpWinOverlay";
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    // Get full screen dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    // Create fullscreen transparent overlay
    overlay_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"SlurpWinOverlay",
        L"Select Region",
        WS_POPUP,
        0, 0, screen_width, screen_height,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    if (!overlay_hwnd) {
        return false;
    }
    
    // Set layered window attributes for transparency
    SetLayeredWindowAttributes(overlay_hwnd, 0, 200, LWA_ALPHA);
    
    return true;
}

LRESULT CALLBACK SlurpWin::window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!g_slurp_instance) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    switch (msg) {
        case WM_LBUTTONDOWN: {
            g_slurp_instance->on_mouse_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (wParam & MK_LBUTTON) {
                g_slurp_instance->on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            g_slurp_instance->on_mouse_up(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                g_slurp_instance->captured = true;
                g_slurp_instance->result.valid = false;
            }
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            g_slurp_instance->render_selection(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND: {
            // Fill with semi-transparent black
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
            
            return 1;
        }
        
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SlurpWin::on_mouse_down(int x, int y) {
    selecting = true;
    start_x = x;
    start_y = y;
    current_x = x;
    current_y = y;
    
    // Invalidate to trigger repaint
    InvalidateRect(overlay_hwnd, nullptr, TRUE);
}

void SlurpWin::on_mouse_move(int x, int y) {
    if (!selecting) return;
    
    current_x = x;
    current_y = y;
    
    // Invalidate to trigger repaint
    InvalidateRect(overlay_hwnd, nullptr, TRUE);
}

void SlurpWin::on_mouse_up(int x, int y) {
    if (!selecting) return;
    
    selecting = false;
    current_x = x;
    current_y = y;
    
    // Calculate final selection
    result.x = std::min(start_x, current_x);
    result.y = std::min(start_y, current_y);
    result.width = std::abs(current_x - start_x);
    result.height = std::abs(current_y - start_y);
    
    // Validate minimum size
    if (result.width < 10 || result.height < 10) {
        result.valid = false;
    } else {
        result.valid = true;
    }
    
    captured = true;
}

void SlurpWin::render_selection(HDC hdc) {
    if (!selecting && !result.valid) return;
    
    int x1 = selecting ? start_x : result.x;
    int y1 = selecting ? start_y : result.y;
    int x2 = selecting ? current_x : result.x + result.width;
    int y2 = selecting ? current_y : result.y + result.height;
    
    int width = std::abs(x2 - x1);
    int height = std::abs(y2 - y1);
    
    // Draw selection rectangle
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    SelectObject(hdc, pen);
    
    HBRUSH brush = CreateSolidBrush(RGB(0, 255, 0));
    SelectObject(hdc, brush);
    
    Rectangle(hdc, x1, y1, x2, y2);
    
    // Draw coordinates text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    
    std::wstring coord_text = std::to_wstring(width) + L"x" + std::to_wstring(height);
    TextOutW(hdc, x1 + 5, y1 + 5, coord_text.c_str(), coord_text.length());
    
    DeleteObject(brush);
    DeleteObject(pen);
}

std::string SlurpWin::get_slurp_output(const Selection& selection) {
    if (!selection.valid) {
        return "";
    }
    
    std::ostringstream oss;
    oss << selection.x << "," << selection.y << " " 
        << selection.width << "x" << selection.height;
    return oss.str();
}

Selection SlurpWin::parse_slurp_output(const std::string& output) {
    Selection selection;
    selection.valid = false;
    selection.x = 0;
    selection.y = 0;
    selection.width = 0;
    selection.height = 0;
    
    // Expected format: "x,y widthxheight"
    int x, y, w, h;
    char sep1, sep2, sep3;
    
    std::istringstream iss(output);
    if (iss >> x >> sep1 >> y >> sep2 >> w >> sep3 >> h) {
        if (sep1 == ',' && sep2 == ' ' && sep3 == 'x') {
            selection.x = x;
            selection.y = y;
            selection.width = w;
            selection.height = h;
            selection.valid = true;
        }
    }
    
    return selection;
}
