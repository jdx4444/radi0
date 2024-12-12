// modules/Utilities.h

#ifndef UTILITIES_H
#define UTILITIES_H

#include "imgui.h"

// Converts virtual coordinates to pixel coordinates based on scaling factors
inline ImVec2 ToPixels(float x_virtual, float y_virtual, float scale_x, float scale_y) {
    return ImVec2(x_virtual * scale_x, y_virtual * scale_y);
}

#endif // UTILITIES_H
