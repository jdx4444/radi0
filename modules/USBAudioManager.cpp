#include "USBAudioManager.h"
#include <filesystem>
#include <algorithm>
#include <random>
#include <iostream>
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

namespace fs = std::filesystem;

#define USB_MOUNT_POINT "/media/jdx4444/Mustick"

USBAudioManager::USBAudioManager()
    : currentTrackIndex(0), state(PlaybackState::Stopped), volume(64), currentMusic(nullptr),
      trackStartTicks(0), pausedTicks(0)
{
}

USBAudioManager::~USBAudioManager() {
    Shutdown();
}

bool USBAudioManager::Initialize() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! Error: " << Mix_GetError() << "\n";
        return false;
    }
    ScanForTracks();
    if (tracks.empty()) {
        std::cerr << "No mp3 tracks found on USB.\n";
        return false;
    }
    // Randomly shuffle the track list.
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(tracks.begin(), tracks.end(), g);

    currentTrackIndex = 0;
    StartTrack(currentTrackIndex);
    state = PlaybackState::Playing;
    return true;
}

void USBAudioManager::Shutdown() {
    if (currentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
    Mix_CloseAudio();
    state = PlaybackState::Stopped;
}

void USBAudioManager::Play() {
    if (state == PlaybackState::Paused) {
        Resume();
    }
}

void USBAudioManager::Pause() {
    if (state == PlaybackState::Playing) {
        pausedTicks = SDL_GetTicks() - trackStartTicks;
        Mix_PauseMusic();
        state = PlaybackState::Paused;
    }
}

void USBAudioManager::Resume() {
    if (state == PlaybackState::Paused) {
        trackStartTicks = SDL_GetTicks() - pausedTicks;
        Mix_ResumeMusic();
        state = PlaybackState::Playing;
    }
}

void USBAudioManager::NextTrack() {
    if (!tracks.empty()) {
        currentTrackIndex = (currentTrackIndex + 1) % tracks.size();
        StartTrack(currentTrackIndex);
    }
}

void USBAudioManager::PreviousTrack() {
    if (!tracks.empty()) {
        currentTrackIndex = (currentTrackIndex - 1 + tracks.size()) % tracks.size();
        StartTrack(currentTrackIndex);
    }
}

void USBAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    Mix_VolumeMusic(volume);
}

int USBAudioManager::GetVolume() const {
    return volume;
}

void USBAudioManager::Update(float /*delta_time*/) {
    // If the track has finished playing, move to the next one.
    if (state == PlaybackState::Playing && Mix_PlayingMusic() == 0) {
        NextTrack();
    }
}

std::string USBAudioManager::GetCurrentTrackTitle() const {
    if (tracks.empty()) return "";
    return tracks[currentTrackIndex].title;
}

std::string USBAudioManager::GetCurrentTrackArtist() const {
    if (tracks.empty()) return "";
    return tracks[currentTrackIndex].artist;
}

float USBAudioManager::GetCurrentTrackDuration() const {
    if (tracks.empty()) return 0.0f;
    return tracks[currentTrackIndex].duration;
}

float USBAudioManager::GetCurrentPlaybackPosition() const {
    if (state == PlaybackState::Playing) {
        return (SDL_GetTicks() - trackStartTicks) / 1000.0f;
    } else if (state == PlaybackState::Paused) {
        return pausedTicks / 1000.0f;
    }
    return 0.0f;
}

PlaybackState USBAudioManager::GetState() const {
    return state;
}

void USBAudioManager::ScanForTracks() {
    tracks.clear();
    if (!fs::exists(USB_MOUNT_POINT) || !fs::is_directory(USB_MOUNT_POINT)) {
        std::cerr << "USB mount point not found: " << USB_MOUNT_POINT << "\n";
        return;
    }
    for (const auto &entry : fs::directory_iterator(USB_MOUNT_POINT)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            if (entry.path().extension() == ".mp3" || entry.path().extension() == ".MP3") {
                std::string filename = entry.path().filename().string();
                std::string artist, title;
                ParseTrackFilename(filename, artist, title);
                float duration = GetTrackDuration(path);
                tracks.push_back({path, artist, title, duration});
            }
        }
    }
    if (tracks.empty()) {
        std::cerr << "No mp3 files found in " << USB_MOUNT_POINT << "\n";
    }
}

void USBAudioManager::ParseTrackFilename(const std::string &filename, std::string &artist, std::string &title) {
    size_t dotPos = filename.find_last_of('.');
    std::string nameWithoutExt = (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
    size_t delimPos = nameWithoutExt.find(" - ");
    if (delimPos != std::string::npos) {
        artist = nameWithoutExt.substr(0, delimPos);
        title = nameWithoutExt.substr(delimPos + 3);
    } else {
        artist = "Unknown Artist";
        title = nameWithoutExt;
    }
}

float USBAudioManager::GetTrackDuration(const std::string &path) {
    float duration = 0.0f;
    TagLib::FileRef f(path.c_str());
    if (!f.isNull() && f.audioProperties()) {
        duration = static_cast<float>(f.audioProperties()->length());
    }
    return duration;
}

void USBAudioManager::StartTrack(int index) {
    if (index < 0 || index >= static_cast<int>(tracks.size())) return;
    if (currentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
    std::string path = tracks[index].filePath;
    currentMusic = Mix_LoadMUS(path.c_str());
    if (!currentMusic) {
        std::cerr << "Failed to load music: " << path << " Error: " << Mix_GetError() << "\n";
        return;
    }
    // Start (or restart) the track timer.
    trackStartTicks = SDL_GetTicks();
    pausedTicks = 0;
    if (Mix_PlayMusic(currentMusic, 1) == -1) {
        std::cerr << "Mix_PlayMusic error: " << Mix_GetError() << "\n";
    }
}
