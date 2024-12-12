#include "Sprite.h"
#include <algorithm> // For std::clamp

// Constructor
Sprite::Sprite() : position(ImVec2(0.0f, 0.0f)), size(ImVec2(24.0f, 24.0f)) {}

// Destructor
Sprite::~Sprite() {}

// Initialize the sprite size based on scale
void Sprite::Initialize(float scale_x, float scale_y) {
    size = ImVec2(scale_x * 1.0f, scale_y * 1.0f); // Adjust as needed
}

// Update sprite position based on song progress
void Sprite::UpdatePosition(float progress_fraction, float line_start_x, float line_end_x, float scale_x, float scale_y, float sprite_x_offset, float sprite_y_offset) {
    float total_movement = (line_end_x - line_start_x) + (2.0f * size.x);
    float new_x = line_start_x - size.x + (total_movement * progress_fraction) + sprite_x_offset;
    float line_y = scale_y * 8.5f; // Assuming line_y is consistent; adjust if needed
    float new_y = line_y - (size.y + 6.0f) + sprite_y_offset;
    position = ImVec2(new_x, new_y);
}

// Draw the sprite
void Sprite::Draw(ImDrawList* draw_list, ImU32 color) {
    // Define a simple car sprite pattern (19x9)
    const int SPRITE_WIDTH = 19;
    const int SPRITE_HEIGHT = 9;
    const int sprite_pattern[SPRITE_HEIGHT][SPRITE_WIDTH] = {
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    float pixel_size = size.x * 0.08f; // Adjust scale as needed

    for (int y = 0; y < SPRITE_HEIGHT; ++y) {
        for (int x = 0; x < SPRITE_WIDTH; ++x) {
            if (sprite_pattern[y][x]) {
                ImVec2 pixel_top_left = ImVec2(position.x + x * pixel_size, position.y + y * pixel_size);
                ImVec2 pixel_bottom_right = ImVec2(pixel_top_left.x + pixel_size, pixel_top_left.y + pixel_size);
                draw_list->AddRectFilled(pixel_top_left, pixel_bottom_right, color);
            }
        }
    }
}
