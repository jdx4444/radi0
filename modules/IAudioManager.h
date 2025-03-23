#ifndef IAUDIOMANAGER_H
#define IAUDIOMANAGER_H

#include <string>

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class IAudioManager {
public:
    virtual ~IAudioManager() {}

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void NextTrack() = 0;
    virtual void PreviousTrack() = 0;

    virtual void SetVolume(int volume) = 0;
    virtual int GetVolume() const = 0;

    virtual PlaybackState GetState() const = 0;

    virtual void Update(float delta_time) = 0;

    virtual float GetPlaybackFraction() const = 0;
    virtual std::string GetTimeRemaining() const = 0;

    // Metadata accessors
    virtual std::string GetCurrentTrackTitle() const = 0;
    virtual std::string GetCurrentTrackArtist() const = 0;
    virtual float GetCurrentTrackDuration() const = 0;
    virtual float GetCurrentPlaybackPosition() const = 0;
};

#endif // IAUDIOMANAGER_H
