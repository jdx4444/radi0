#ifndef UTILITIES_H
#define UTILITIES_H

#include "imgui.h"

// -----------------------------------------------------------------------------
// CHANGED: We still convert from (virtual_x, virtual_y) to actual pixels
// but now our "virtual" dimensions are 80×30 (in main.cpp).
// We simply do x * scale_x, y * scale_y.
//
// If you want to unify scale_x and scale_y, you could do so, but here we keep both.
// -----------------------------------------------------------------------------
inline ImVec2 ToPixels(float x_virtual, float y_virtual, float scale_x, float scale_y)
{
    return ImVec2(x_virtual * scale_x, y_virtual * scale_y);
}

#endif // UTILITIES_H
