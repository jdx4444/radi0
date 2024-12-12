#include "UI.h"
#include <algorithm>
#include <string>

const ImU32 COLOR_GREEN = IM_COL32(0, 255, 0, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize() {
    // No changes
}

void UI::Render(ImDrawList* draw_list, BluetoothAudioManager& audioManager, Sprite& sprite, float scale_x, float scale_y) {
    float line_start_x = scale_x * 10.0f;
    float line_end_x = scale_x * 19.0f;
    float line_y = scale_y * 8.5f;

    DrawProgressLine(draw_list, audioManager, line_start_x, line_end_x, line_y, scale_x, scale_y);
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(), line_start_x, line_end_x, scale_x, scale_y, -12.5f, 11.0f);
    sprite.Draw(draw_list, COLOR_GREEN);
    DrawMaskBars(draw_list, scale_x, scale_y);

    // Removed albumArt.Render(...) line

    DrawTimeRemaining(draw_list, audioManager, scale_x, scale_y);
    DrawArtistAndTrackInfo(draw_list, audioManager, scale_x, scale_y);
    DrawVolumeBar(draw_list, audioManager, scale_x, scale_y);
}

void UI::Cleanup() {}

void UI::DrawProgressLine(ImDrawList* draw_list, BluetoothAudioManager& audioManager, float line_start_x, float line_end_x, float line_y, float scale_x, float scale_y) {
    float progress_fraction = audioManager.GetPlaybackFraction();

    draw_list->AddLine(ImVec2(line_start_x, line_y), ImVec2(line_end_x, line_y), IM_COL32(100, 100, 100, 255), 2.0f);
    draw_list->AddLine(ImVec2(line_start_x, line_y), ImVec2(line_end_x, line_y), COLOR_GREEN, 4.0f);
}

void UI::DrawMaskBars(ImDrawList* draw_list, float scale_x, float scale_y) {
    const float MASK_BAR_WIDTH = 12.0f;
    const float MASK_BAR_HEIGHT = 4.0f;
    ImVec2 left_mask_pos = ToPixels(10.0f - MASK_BAR_WIDTH, 8.5f - MASK_BAR_HEIGHT / 2.0f, scale_x, scale_y);
    ImVec2 right_mask_pos = ToPixels(19.0f, 8.5f - MASK_BAR_HEIGHT / 2.0f, scale_x, scale_y);
    ImVec2 mask_bar_size = ToPixels(MASK_BAR_WIDTH, MASK_BAR_HEIGHT, scale_x, scale_y);

    draw_list->AddRectFilled(left_mask_pos, ImVec2(left_mask_pos.x + mask_bar_size.x, left_mask_pos.y + mask_bar_size.y), COLOR_BLACK);
    draw_list->AddRectFilled(right_mask_pos, ImVec2(right_mask_pos.x + mask_bar_size.x, right_mask_pos.y + mask_bar_size.y), COLOR_BLACK);
}

void UI::DrawTimeRemaining(ImDrawList* draw_list, BluetoothAudioManager& audioManager, float scale_x, float scale_y) {
    ImVec2 time_text_pos = ToPixels(10.0f, 8.75f, scale_x, scale_y);
    ImVec2 progress_bar_size = ToPixels(9.0f, 1.0f, scale_x, scale_y);

    std::string time_remaining_str = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_remaining_str.c_str());
    float text_x = time_text_pos.x + (progress_bar_size.x - text_size.x) / 2.0f;

    ImGui::SetCursorPos(ToPixels(10.0f, 8.75f, scale_x, scale_y));
    ImGui::SetCursorPosX(text_x);
    ImGui::Text("%s", time_remaining_str.c_str());
}

void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list, BluetoothAudioManager& audioManager, float scale_x, float scale_y) {
    // Remain consistent with your original formatting.
    // These were placeholders; you can still show placeholders.

    ImVec2 artist_name_pos = ToPixels(11.0f, 4.5f, scale_x, scale_y);
    ImVec2 artist_name_size = ToPixels(8.0f, 1.5f, scale_x, scale_y);
    const char* artist_name = "Tigers Jaw";
    ImGui::SetWindowFontScale(1.3f);
    ImVec2 text_size = ImGui::CalcTextSize(artist_name);
    ImGui::SetCursorPos(artist_name_pos);

    float available_width = artist_name_size.x;
    static float artist_scroll_offset = 0.0f;
    float artist_text_width = text_size.x;

    if (artist_text_width > available_width) {
        artist_scroll_offset += ImGui::GetIO().DeltaTime * 30.0f;
        if (artist_scroll_offset > artist_text_width)
            artist_scroll_offset = -available_width;
    } else {
        artist_scroll_offset = 0.0f;
    }

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 clip_min = ImVec2(window_pos.x + artist_name_pos.x, window_pos.y + artist_name_pos.y);
    ImVec2 clip_max = ImVec2(clip_min.x + artist_name_size.x, clip_min.y + artist_name_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);

    ImGui::SetCursorPosX(artist_name_pos.x - artist_scroll_offset);
    ImGui::Text("%s", artist_name);
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);

    ImVec2 track_name_pos = ToPixels(11.0f, 5.5f, scale_x, scale_y);
    ImVec2 track_name_size = ToPixels(8.0f, 1.5f, scale_x, scale_y);
    const char* track_name = "Never Saw It Coming";
    ImGui::SetWindowFontScale(1.1f);
    text_size = ImGui::CalcTextSize(track_name);
    ImGui::SetCursorPos(track_name_pos);

    float track_available_width = track_name_size.x;
    static float track_scroll_offset = 0.0f;
    float track_text_width = text_size.x;

    if (track_text_width > track_available_width) {
        track_scroll_offset += ImGui::GetIO().DeltaTime * 30.0f;
        if (track_scroll_offset > track_text_width)
            track_scroll_offset = -track_available_width;
    } else {
        track_scroll_offset = 0.0f;
    }

    clip_min = ImVec2(window_pos.x + track_name_pos.x, window_pos.y + track_name_pos.y);
    clip_max = ImVec2(clip_min.x + track_name_size.x, clip_min.y + track_name_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);
    ImGui::SetCursorPosX(track_name_pos.x - track_scroll_offset);
    ImGui::Text("%s", track_name);
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);
}

void UI::DrawVolumeBar(ImDrawList* draw_list, BluetoothAudioManager& audioManager, float scale_x, float scale_y) {
    ImVec2 volume_pos = ToPixels(11.0f, 1.0f, scale_x, scale_y);
    ImVec2 volume_size = ToPixels(6.0f, 0.5f, scale_x, scale_y);
    ImGui::SetCursorPos(volume_pos);
    ImGui::Text("Vol:");
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");

    float bar_x = volume_pos.x + text_size.x + ToPixels(0.2f, 0.0f, scale_x, scale_y).x;
    float bar_y = volume_pos.y + (volume_size.y - ToPixels(0.0f, 0.6f, scale_x, scale_y).y) / 2.0f;
    ImVec2 bar_size = ImVec2(ToPixels(5.0f, 0.0f, scale_x, scale_y).x, ToPixels(0.0f, 0.6f, scale_x, scale_y).y);

    ImGui::SetCursorPos(ImVec2(bar_x, bar_y));

    float volume_fraction = (float)audioManager.GetVolume() / 128.0f;
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    draw_list->AddRectFilled(cursor_pos, ImVec2(cursor_pos.x + bar_size.x, cursor_pos.y + bar_size.y), COLOR_BLACK);
    draw_list->AddRectFilled(cursor_pos, ImVec2(cursor_pos.x + bar_size.x * volume_fraction, cursor_pos.y + bar_size.y), COLOR_GREEN);
    draw_list->AddRect(cursor_pos, ImVec2(cursor_pos.x + bar_size.x, cursor_pos.y + bar_size.y), COLOR_GREEN);

    ImGui::InvisibleButton("volume_bar", bar_size);
    if (ImGui::IsItemActive()) {
        ImVec2 mouse_pos_in_bar = ImVec2(ImGui::GetIO().MousePos.x - cursor_pos.x, ImGui::GetIO().MousePos.y - cursor_pos.y);
        float new_volume_fraction = mouse_pos_in_bar.x / bar_size.x;
        new_volume_fraction = std::clamp(new_volume_fraction, 0.0f, 1.0f);
        int new_volume = (int)(new_volume_fraction * 128.0f);
        audioManager.SetVolume(new_volume);
    }
}
