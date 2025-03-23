#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "modules/IAudioManager.h"
#include "modules/Sprite.h"

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    void Render(ImDrawList* draw_list,
                IAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y,
                int window_width,
                int window_height);
    void Cleanup();

private:
    // Internal drawing functions.
    void DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                IAudioManager& audioManager,
                                float scale,
                                float offset_x,
                                float offset_y);
    void DrawProgressLine(ImDrawList* draw_list,
                          IAudioManager& audioManager,
                          float scale,
                          float offset_x,
                          float offset_y);
    void DrawMaskBars(ImDrawList* draw_list,
                      float scale,
                      float offset_x,
                      float offset_y);
    void DrawVolumeSun(ImDrawList* draw_list,
                       IAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y);
    void DrawSunMask(ImDrawList* draw_list,
                     float scale,
                     float offset_x,
                     float offset_y);
    // Layout structure (using example values)
    struct {
        float progressBarStartX = 10.0f;
        float progressBarEndX = 70.0f;
        float progressBarY = 20.0f;
        float progressBarThickness = 1.0f;
        float artistTextX = 10.0f;
        float artistTextY = 2.0f;
        float artistTextWidth = 30.0f;
        float trackTextX = 40.0f;
        float trackTextY = 2.0f;
        float trackTextWidth = 30.0f;
        float borderPadding = 2.0f;
        float indicatorCenterX = 40.0f;
        float indicatorCenterY = 12.5f;
        float indicatorRadius = 5.0f;
        float sunDiameter = 8.0f;
        float sunMaskTop = 0.0f;
        float sunMaskBottom = 5.0f;
        float spriteXCorrection = 0.0f;
        float spriteXOffset = 0.0f;
        float spriteYOffset = 0.0f;
        float spriteBaseY = 0.0f;
    } layout;
    // Utility: convert virtual coordinates to pixels.
    ImVec2 ToPixels(float x, float y, float scale, float offset_x, float offset_y) {
        return ImVec2(x * scale + offset_x, y * scale + offset_y);
    }
};

#endif // UI_H
