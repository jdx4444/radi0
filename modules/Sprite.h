#ifndef SPRITE_H
#define SPRITE_H

#include "imgui.h"

class Sprite {
public:
    Sprite();
    ~Sprite();

    // Initialize the sprite using a unified scale factor based on a virtual coordinate system of 80×25.
    void Initialize(float scale);
    // UpdatePosition computes the sprite's position (using virtual coordinates)
    // based on progress along a line.
    void UpdatePosition(float progress_fraction,
                        float line_start_x, float line_end_x,
                        float scale, float offset_x, float offset_y,
                        float sprite_x_offset, float sprite_y_offset,
                        float sprite_base_y);
    void Draw(ImDrawList* draw_list, ImU32 color);
    
    // New: Return the exhaust emission position.
    ImVec2 GetExhaustPosition() const;

private:
    ImVec2 position;
    ImVec2 size;
};

#endif // SPRITE_H
