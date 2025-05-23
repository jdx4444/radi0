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
#include <vector>

// Utility function to check if a directory exists
static bool directoryExists(const std::string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

// Function to get the USB mount path dynamically based on the current username.
// It assumes that the USB is named "Mustick".
static std::string getUSBMountPath() {
    const char* username = getenv("USER");
    std::string user = (username) ? username : "default";
    return "/media/" + user + "/Mustick";
}

// Updated utility function to parse a filename into artist, title, and duration.
// Expects the filename in the format: "Artist Name - Track Name - XmYYs.mp3"
// For example: "Tigers Jaw - Do You Really Wanna Know - 3m45s.mp3"
// matches script here: 
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
    
    // Expecting three parts: artist, title, and duration in "XmYYs" format
    if (parts.size() >= 3) {
        artist = parts[0];
        title = parts[1];
        std::string durationStr = parts[2];
        size_t mPos = durationStr.find("m");
        size_t sPos = durationStr.find("s", mPos);
        if (mPos != std::string::npos && sPos != std::string::npos) {
            try {
                int minutes = std::stoi(durationStr.substr(0, mPos));
                int seconds = std::stoi(durationStr.substr(mPos + 1, sPos - (mPos + 1)));
                duration = static_cast<float>(minutes * 60 + seconds);
            } catch (...) {
                duration = 0.0f;
            }
        } else {
            duration = 0.0f;
        }
    } else if (parts.size() == 2) {
        // Fallback if duration is not provided
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
      baseVolume(64),      // User-set volume (0 to MIX_MAX_VOLUME)
      gainFactor(0.40f),    // Default gain factor (1.0 means no change)
      playbackPosition(0.0f),
      currentMusic(nullptr)
{
}

USBAudioManager::~USBAudioManager() {
    Shutdown();
}

bool USBAudioManager::Initialize() {
    if (!directoryExists(getUSBMountPath())) {
        std::cerr << "USB drive not found at " << getUSBMountPath() << "\n";
        return false;
    }
    // Clear any previous playlist
    playlist.clear();

    // Wait a short time to allow the USB drive to settle
    SDL_Delay(1000);  // 1 second delay

    // (Reeeee)initialize the audio subsystem for this manager.
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL audio initialization failed: " << SDL_GetError() << "\n";
        return false;
    }
    // Increased chunk size from 2048 to 4096 to help reduce crackling (testing fine so far)
    const int audioFrequency = 44100;
    const int audioFormat = MIX_DEFAULT_FORMAT;
    const int audioChannels = 2;
    const int audioChunkSize = 4096;
    if (Mix_OpenAudio(audioFrequency, audioFormat, audioChannels, audioChunkSize) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << "\n";
        return false;
    }
    // Scan the USB directory for MP3 files
    if (!scanUSBDirectory()) {
        std::cerr << "No MP3 files found on USB drive.\n";
        return false;
    }
    printf("Found %zu MP3 file(s) on USB drive.\n", playlist.size());
    // Shuffle the playlist.
    {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(playlist.begin(), playlist.end(), g);
        currentTrackIndex = 0;
    }
    // Load first trak
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

// Updated SetVolume: The UI uses baseVolume (full range), while effective volume = baseVolume * gainFactor.
void USBAudioManager::SetVolume(int vol) {
    baseVolume = std::clamp(vol, 0, MIX_MAX_VOLUME);
    int effectiveVolume = static_cast<int>(baseVolume * gainFactor);
    if (effectiveVolume > MIX_MAX_VOLUME)
        effectiveVolume = MIX_MAX_VOLUME;
    Mix_VolumeMusic(effectiveVolume);
    // Note: GetVolume() returns baseVolume, so the UI sees the full range
}

int USBAudioManager::GetVolume() const {
    return baseVolume;
}

PlaybackState USBAudioManager::GetState() const {
    return state;
}

void USBAudioManager::Update(float delta_time) {
    if (state == PlaybackState::Playing) {
        playbackPosition += delta_time;
        // If music has finished playing, automatically move to the next track
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

// New methods for gain adjustment to match bt volume
void USBAudioManager::SetGain(float factor) {
    gainFactor = factor;
    // Reapply volume with the new gain factor
    SetVolume(baseVolume);
}

float USBAudioManager::GetGain() const {
    return gainFactor;
}

// -----------------------------------------------------------------------------
// Private Helper Functions
// -----------------------------------------------------------------------------
bool USBAudioManager::scanUSBDirectory() {
    std::string mountPath = getUSBMountPath();
    DIR* dir = opendir(mountPath.c_str());
    if (!dir) {
        std::cerr << "Failed to open USB directory: " << mountPath << "\n";
        return false;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".mp3") {
            std::string fullPath = mountPath + "/" + filename;
            TrackInfo info;
            info.filePath = fullPath;
            float fileDuration = 0.0f;
            // Parse artist, title, and duration from the filename 
            parseFilename(filename, info.artist, info.title, fileDuration);
            info.duration = fileDuration; // duration in seconds
            playlist.push_back(info);
        }
    }
    closedir(dir);
    return !playlist.empty();
}

// Loads the current track into memory using SDL_RWops
void USBAudioManager::loadCurrentTrack() {
    if (playlist.empty())
        return;
    unloadCurrentTrack();
    const TrackInfo &track = playlist[currentTrackIndex];
    
    // open the file in binary mode
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

// Frees the currently loaded track
void USBAudioManager::unloadCurrentTrack() {
    if (currentMusic) {
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
}