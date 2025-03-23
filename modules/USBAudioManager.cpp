#include "USBAudioManager.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <random>

// Path where the USB flash drive is mounted.
static const std::string USB_MOUNT_PATH = "/media/jdx4444/Mustick";

// Utility function to check if a directory exists.
static bool directoryExists(const std::string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

// Updated utility function to parse a filename into artist, title, and duration.
// Assumes the filename is of the form "Artist Name - Track Name - mm:ss.mp3"
static void parseFilename(const std::string &filename, std::string &artist, std::string &title, float &duration) {
    // Remove extension.
    std::string base = filename;
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        base = filename.substr(0, dotPos);
    }
    
    // Split based on " - "
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = base.find(" - ");
    while (pos != std::string::npos) {
        parts.push_back(base.substr(start, pos - start));
        start = pos + 3; // length of " - "
        pos = base.find(" - ", start);
    }
    parts.push_back(base.substr(start));
    
    // Expecting three parts: artist, title, and duration in mm:ss.
    if (parts.size() >= 3) {
        artist = parts[0];
        title = parts[1];
        std::string durationStr = parts[2];
        size_t colonPos = durationStr.find(":");
        if (colonPos != std::string::npos) {
            try {
                int minutes = std::stoi(durationStr.substr(0, colonPos));
                int seconds = std::stoi(durationStr.substr(colonPos + 1));
                duration = static_cast<float>(minutes * 60 + seconds);
            } catch (...) {
                duration = 0.0f;
            }
        } else {
            duration = 0.0f;
        }
    } else if (parts.size() == 2) {
        // Fallback if duration is not provided.
        artist = parts[0];
        title = parts[1];
        duration = 0.0f;
    } else {
        title = base;
        artist = "Unknown Artist";
        duration = 0.0f;
    }
}

USBAudioManager::USBAudioManager()
    : currentTrackIndex(0),
      state(PlaybackState::Stopped),
      volume(64),
      playbackPosition(0.0f),
      currentMusic(nullptr)
{
}

USBAudioManager::~USBAudioManager() {
    Shutdown();
}

bool USBAudioManager::Initialize() {
    if (!directoryExists(USB_MOUNT_PATH)) {
        std::cerr << "USB drive not found at " << USB_MOUNT_PATH << "\n";
        return false;
    }
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL audio initialization failed: " << SDL_GetError() << "\n";
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << "\n";
        return false;
    }
    // Scan the USB directory for MP3 files.
    if (!scanUSBDirectory()) {
        std::cerr << "No MP3 files found on USB drive.\n";
        return false;
    }
    // Shuffle the playlist to randomize playback order.
    {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(playlist.begin(), playlist.end(), g);
        currentTrackIndex = 0; // Reset to the first track after shuffling.
    }
    // Load the first track.
    loadCurrentTrack();
    return true;
}

void USBAudioManager::Shutdown() {
    unloadCurrentTrack();
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void USBAudioManager::Play() {
    if (playlist.empty()) {
        std::cerr << "Playlist is empty.\n";
        return;
    }
    if (currentMusic == nullptr) {
        loadCurrentTrack();
    }
    if (Mix_PlayMusic(currentMusic, 0) == -1) {
        std::cerr << "Error playing music: " << Mix_GetError() << "\n";
        return;
    }
    state = PlaybackState::Playing;
    playbackPosition = 0.0f;
}

void USBAudioManager::Pause() {
    if (state == PlaybackState::Playing) {
        Mix_PauseMusic();
        state = PlaybackState::Paused;
    }
}

void USBAudioManager::Resume() {
    if (state == PlaybackState::Paused) {
        Mix_ResumeMusic();
        state = PlaybackState::Playing;
    }
}

void USBAudioManager::NextTrack() {
    unloadCurrentTrack();
    currentTrackIndex = (currentTrackIndex + 1) % playlist.size();
    loadCurrentTrack();
    Play();
}

void USBAudioManager::PreviousTrack() {
    unloadCurrentTrack();
    if (currentTrackIndex == 0)
        currentTrackIndex = playlist.size() - 1;
    else
        currentTrackIndex--;
    loadCurrentTrack();
    Play();
}

void USBAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    Mix_VolumeMusic(volume);  // SDL_mixer volume range is 0–128
}

int USBAudioManager::GetVolume() const {
    return volume;
}

PlaybackState USBAudioManager::GetState() const {
    return state;
}

void USBAudioManager::Update(float delta_time) {
    if (state == PlaybackState::Playing) {
        playbackPosition += delta_time;
        // If music has finished playing, automatically move to the next track.
        if (!Mix_PlayingMusic()) {
            NextTrack();
        }
    }
}

float USBAudioManager::GetPlaybackFraction() const {
    float duration = GetCurrentTrackDuration();
    return (duration > 0.0f) ? (playbackPosition / duration) : 0.0f;
}

std::string USBAudioManager::GetTimeRemaining() const {
    float remaining = GetCurrentTrackDuration() - playbackPosition;
    if (remaining < 0.0f)
        remaining = 0.0f;
    int minutes = static_cast<int>(remaining) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return std::string(buffer);
}

std::string USBAudioManager::GetCurrentTrackTitle() const {
    if (playlist.empty())
        return "Unknown Track";
    return playlist[currentTrackIndex].title;
}

std::string USBAudioManager::GetCurrentTrackArtist() const {
    if (playlist.empty())
        return "Unknown Artist";
    return playlist[currentTrackIndex].artist;
}

float USBAudioManager::GetCurrentTrackDuration() const {
    if (playlist.empty())
        return 0.0f;
    return playlist[currentTrackIndex].duration;
}

float USBAudioManager::GetCurrentPlaybackPosition() const {
    return playbackPosition;
}

// -----------------------------------------------------------------------------
// Private Helper Functions
// -----------------------------------------------------------------------------
bool USBAudioManager::scanUSBDirectory() {
    DIR* dir = opendir(USB_MOUNT_PATH.c_str());
    if (!dir) {
        std::cerr << "Failed to open USB directory: " << USB_MOUNT_PATH << "\n";
        return false;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".mp3") {
            std::string fullPath = USB_MOUNT_PATH + "/" + filename;
            TrackInfo info;
            info.filePath = fullPath;
            float fileDuration = 0.0f;
            // Parse artist, title, and duration from the filename.
            parseFilename(filename, info.artist, info.title, fileDuration);
            info.duration = fileDuration; // duration in seconds
            playlist.push_back(info);
        }
    }
    closedir(dir);
    return !playlist.empty();
}

// Loads the current track into memory using SDL_RWops.
void USBAudioManager::loadCurrentTrack() {
    if (playlist.empty())
        return;
    unloadCurrentTrack();
    const TrackInfo &track = playlist[currentTrackIndex];
    
    // Open the file in binary mode.
    SDL_RWops* rw = SDL_RWFromFile(track.filePath.c_str(), "rb");
    if (!rw) {
        std::cerr << "Failed to open file " << track.filePath << "\n";
        return;
    }
    // Load the music from the RWops.
    // The second parameter (1) tells SDL_mixer to free the RWops automatically.
    currentMusic = Mix_LoadMUS_RW(rw, 1);
    if (!currentMusic) {
        std::cerr << "Failed to load track from memory: " << Mix_GetError() << "\n";
    }
}

// Frees the currently loaded track.
void USBAudioManager::unloadCurrentTrack() {
    if (currentMusic) {
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
}
