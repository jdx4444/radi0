#ifndef SPRITE_H
#define SPRITE_H

#include "imgui.h"

class Sprite {
public:
    Sprite();
    ~Sprite();

    void Initialize(float scale_x, float scale_y);
    void UpdatePosition(float progress_fraction,
                        float line_start_x, float line_end_x,
                        float scale_x, float scale_y,
                        float sprite_x_offset, float sprite_y_offset);
    void Draw(ImDrawList* draw_list, ImU32 color);

private:
    ImVec2 position;
    ImVec2 size;
};

#endif // SPRITE_H
