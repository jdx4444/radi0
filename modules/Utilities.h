#ifndef UTILITIES_H
#define UTILITIES_H

#include "imgui.h"

// Converts virtual coordinates to actual pixels using a unified scale and offsets.
inline ImVec2 ToPixels(float x_virtual, float y_virtual, float scale, float offset_x, float offset_y)
{
    return ImVec2(x_virtual * scale + offset_x, y_virtual * scale + offset_y);
}

#endif // UTILITIES_H
