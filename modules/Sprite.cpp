#include "Sprite.h"
#include <algorithm>

Sprite::Sprite()
    : position(ImVec2(0.0f, 0.0f)),
      size(ImVec2(24.0f, 24.0f)) // Not super important; updated later
{
}

Sprite::~Sprite()
{
}

void Sprite::Initialize(float scale_x, float scale_y)
{
    // CHANGED: Make the sprite about 2×2 virtual units, for example
    // So that at scale=16, it's ~32×32 pixels in size
    size = ImVec2(2.0f * scale_x, 2.0f * scale_y);
}

// Adjust position horizontally based on progress fraction
void Sprite::UpdatePosition(float progress_fraction,
                            float line_start_x, float line_end_x,
                            float scale_x, float scale_y,
                            float sprite_x_offset, float sprite_y_offset)
{
    float line_pixel_start = line_start_x * scale_x;
    float line_pixel_end   = line_end_x   * scale_x;

    // We'll move from (line_pixel_start) to (line_pixel_end).
    // The sprite’s “anchor” could be center or left. We'll keep it simple.
    float total_movement = (line_pixel_end - line_pixel_start);

    float new_x = line_pixel_start + total_movement * progress_fraction;

    // For vertical, we use line_y * scale_y. We rely on the caller to supply
    // the correct line_y offset or any extra offsets. For simplicity we do:
    // (We’ll ignore the progress fraction in vertical dimension.)
    float new_y = 28.0f * scale_y; // near the bar. Tweak if needed

    // Then apply the caller’s offsets in pixels
    new_x += sprite_x_offset * scale_x;
    new_y += sprite_y_offset * scale_y;

    position = ImVec2(new_x, new_y);
}

void Sprite::Draw(ImDrawList* draw_list, ImU32 color)
{
    // A simple “car” pattern 19×9
    const int SPRITE_WIDTH = 19;
    const int SPRITE_HEIGHT = 9;

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

    // CHANGED: Use a smaller pixel size so the car is not huge
    float pixel_size = 0.08f * size.x; // Now smaller than the previous 0.08f

    for (int y = 0; y < SPRITE_HEIGHT; ++y) {
        for (int x = 0; x < SPRITE_WIDTH; ++x) {
            if (sprite_pattern[y][x]) {
                ImVec2 top_left = ImVec2(position.x + x * pixel_size,
                                         position.y + y * pixel_size);
                ImVec2 bot_right = ImVec2(top_left.x + pixel_size,
                                          top_left.y + pixel_size);
                draw_list->AddRectFilled(top_left, bot_right, color);
            }
        }
    }
}
