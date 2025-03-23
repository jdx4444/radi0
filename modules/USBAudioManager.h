#ifndef USB_AUDIO_MANAGER_H
#define USB_AUDIO_MANAGER_H

#include "IAudioManager.h"
#include <vector>
#include <string>
#include <SDL_mixer.h>   // For Mix_Music definition

// Structure to hold track metadata.
struct TrackInfo {
    std::string filePath;
    std::string artist;
    std::string title;
    float duration; // in seconds
};

class USBAudioManager : public IAudioManager {
public:
    USBAudioManager();
    virtual ~USBAudioManager();

    virtual bool Initialize() override;
    virtual void Shutdown() override;

    virtual void Play() override;
    virtual void Pause() override;
    virtual void Resume() override;
    virtual void NextTrack() override;
    virtual void PreviousTrack() override;

    virtual void SetVolume(int volume) override;
    virtual int GetVolume() const override;

    virtual PlaybackState GetState() const override;

    virtual void Update(float delta_time) override;

    virtual float GetPlaybackFraction() const override;
    virtual std::string GetTimeRemaining() const override;

    virtual std::string GetCurrentTrackTitle() const override;
    virtual std::string GetCurrentTrackArtist() const override;
    virtual float GetCurrentTrackDuration() const override;
    virtual float GetCurrentPlaybackPosition() const override;

    // New methods for gain adjustment.
    void SetGain(float factor);
    float GetGain() const;

private:
    bool scanUSBDirectory();
    void loadCurrentTrack();
    void unloadCurrentTrack();

    std::vector<TrackInfo> playlist;
    int currentTrackIndex;
    PlaybackState state;
    int volume; // Current effective volume (0-128)
    float playbackPosition; // in seconds

    // SDL_mixer music pointer
    Mix_Music* currentMusic;

    // New private members for volume gain control.
    int baseVolume;      // The user-set base volume (before gain)
    float gainFactor;    // Multiplier for adjusting the effective volume
};

#endif // USB_AUDIO_MANAGER_H
