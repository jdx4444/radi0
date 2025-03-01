#ifndef SPRITE_H
#define SPRITE_H

#include "imgui.h"

class Sprite {
public:
    Sprite();
    ~Sprite();

    // Initialize using a unified scale factor.
    void Initialize(float scale);
    // Updated UpdatePosition to use unified scale and offsets,
    // plus a base Y position for vertical placement.
    void UpdatePosition(float progress_fraction,
                        float line_start_x, float line_end_x,
                        float scale, float offset_x, float offset_y,
                        float sprite_x_offset, float sprite_y_offset,
                        float sprite_base_y);
    void Draw(ImDrawList* draw_list, ImU32 color);

private:
    ImVec2 position;
    ImVec2 size;
};

#endif // SPRITE_H
