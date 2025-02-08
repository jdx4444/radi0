#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "BluetoothAudioManager.h"
#include "Sprite.h"
#include "Utilities.h"

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    void Render(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                Sprite& sprite, float scale_x, float scale_y);
    void Cleanup();

private:
    void DrawProgressLine(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                          float line_start_x, float line_end_x, float line_y,
                          float scale_x, float scale_y);
    void DrawMaskBars(ImDrawList* draw_list, float scale_x, float scale_y,
                      float line_start_x, float line_end_x, float line_y);
    void DrawTimeRemaining(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y);
    void DrawArtistAndTrackInfo(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y);
    void DrawVolumeBar(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y);
};

#endif // UI_H
