// slurp-win.cpp - Region selector for Windows
// Usage: slurp-win
// Output: "x,y WxH" format (compatible with slurp from sway)

#include "slurp.h"
#include <iostream>

int main() {
    SlurpWin slurp;
    
    std::cout << "Select a region with your mouse...\n";
    std::cout << "Press ESC to cancel.\n";
    
    Selection selection = slurp.select_region();
    
    if (!selection.valid) {
        std::cout << "Selection cancelled.\n";
        return 1;
    }
    
    // Output in slurp-compatible format: "x,y WxH"
    std::string output = slurp.get_slurp_output(selection);
    std::cout << output << "\n";
    
    return 0;
}
