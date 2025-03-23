#ifndef USB_AUDIO_MANAGER_H
#define USB_AUDIO_MANAGER_H

#include "IAudioManager.h"
#include <vector>
#include <string>
#include "SDL_mixer.h"
#include "SDL.h" // for SDL_GetTicks()

class USBAudioManager : public IAudioManager {
public:
    USBAudioManager();
    virtual ~USBAudioManager();

    bool Initialize() override;
    void Shutdown() override;

    void Play() override;
    void Pause() override;
    void Resume() override;
    void NextTrack() override;
    void PreviousTrack() override;

    void SetVolume(int vol) override;
    int GetVolume() const override;

    void Update(float delta_time) override;

    std::string GetCurrentTrackTitle() const override;
    std::string GetCurrentTrackArtist() const override;
    float GetCurrentTrackDuration() const override;
    float GetCurrentPlaybackPosition() const override;

    PlaybackState GetState() const override;

private:
    struct Track {
        std::string filePath;
        std::string artist;
        std::string title;
        float duration; // in seconds
    };

    std::vector<Track> tracks;
    int currentTrackIndex;
    PlaybackState state;
    int volume; // volume scale 0-128
    Mix_Music* currentMusic;

    // Timing for playback progress:
    Uint32 trackStartTicks; // When the current track (or resumed track) started
    Uint32 pausedTicks;     // Time accumulated when paused

    // Scans the fixed USB mount directory for .mp3 files.
    void ScanForTracks();
    // Parses a filename "Artist Name - Song Name.mp3" into artist and title.
    void ParseTrackFilename(const std::string &filename, std::string &artist, std::string &title);
    // Reads the track duration (in seconds) using TagLib.
    float GetTrackDuration(const std::string &path);
    // Begins playback of the track at the given index.
    void StartTrack(int index);
};

#endif // USB_AUDIO_MANAGER_H
