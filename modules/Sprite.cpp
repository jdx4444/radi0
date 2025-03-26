#include "Sprite.h"
#include <algorithm>

Sprite::Sprite()
    : position(ImVec2(0.0f, 0.0f)),
      size(ImVec2(24.0f, 24.0f))
{
}

Sprite::~Sprite() {}

void Sprite::Initialize(float scale)
{
    // Set sprite size to about 2×2 virtual units.
    // Note: Virtual coordinate system is now 80×25.
    size = ImVec2(2.0f * scale, 2.0f * scale);
}

void Sprite::UpdatePosition(float progress_fraction,
                            float line_start_x, float line_end_x,
                            float scale, float offset_x, float offset_y,
                            float sprite_x_offset, float sprite_y_offset,
                            float sprite_base_y)
{
    // Calculate the sprite's horizontal position based on a progress fraction along a line,
    // where line_start_x and line_end_x are given in virtual coordinates.
    float line_pixel_start = line_start_x * scale + offset_x;
    float line_pixel_end   = line_end_x   * scale + offset_x;
    float total_movement = (line_pixel_end - line_pixel_start);
    float new_x = line_pixel_start + total_movement * progress_fraction;
    
    // Position the sprite vertically using sprite_base_y (in virtual units)
    float new_y = sprite_base_y * scale + offset_y;
    new_x += sprite_x_offset * scale;
    new_y += sprite_y_offset * scale;
    position = ImVec2(new_x, new_y);
}

void Sprite::Draw(ImDrawList* draw_list, ImU32 color)
{
    // A simple "car" pattern (19×9 pixels)
    const int SPRITE_WIDTH = 19;
    const int SPRITE_HEIGHT = 9;
    
    // Use a smaller pixel size so the sprite isn’t too large.
    // Here, pixel_size is scaled relative to the sprite's overall size.
    float pixel_size = 0.08f * size.x;

    // -- First Pass: Draw a background rectangle for the border and fill.
    // Extend by 1 pixel (i.e. pixel_size) on all sides.
    ImVec2 bg_top_left = ImVec2(position.x - pixel_size, position.y - pixel_size);
    ImVec2 bg_bot_right = ImVec2(position.x + SPRITE_WIDTH * pixel_size + pixel_size,
                                 position.y + SPRITE_HEIGHT * pixel_size + pixel_size);
    draw_list->AddRectFilled(bg_top_left, bg_bot_right, IM_COL32(0, 0, 0, 255));

    // -- Second Pass: Draw the car sprite pattern, offset by 1 pixel to leave the black border.
    const int sprite_pattern[SPRITE_HEIGHT][SPRITE_WIDTH] = {
        {0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };

    // Offset to leave the border (1 pixel).
    float offset = pixel_size;
    for (int y = 0; y < SPRITE_HEIGHT; ++y) {
        for (int x = 0; x < SPRITE_WIDTH; ++x) {
            if (sprite_pattern[y][x]) {
                ImVec2 top_left = ImVec2(position.x + offset + x * pixel_size,
                                         position.y + offset + y * pixel_size);
                ImVec2 bot_right = ImVec2(top_left.x + pixel_size,
                                          top_left.y + pixel_size);
                draw_list->AddRectFilled(top_left, bot_right, color);
            }
        }
    }
}
