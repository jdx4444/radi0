#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include "IAudioManager.h"
#include <string>
#include <dbus/dbus.h>

class BluetoothAudioManager : public IAudioManager {
public:
    BluetoothAudioManager();
    ~BluetoothAudioManager();

    bool Initialize() override;
    void Shutdown() override;

    void Play() override;
    void Pause() override;
    void Resume() override;
    void NextTrack() override;
    void PreviousTrack() override;

    void SetVolume(int volume) override;
    int GetVolume() const override;

    PlaybackState GetState() const override;

    void Update(float delta_time) override;

    // Additional methods used by the UI.
    float GetPlaybackFraction() const;
    std::string GetTimeRemaining() const;

    // IAudioManager interface for metadata:
    std::string GetCurrentTrackTitle() const override { return current_track_title; }
    std::string GetCurrentTrackArtist() const override { return current_track_artist; }
    float GetCurrentTrackDuration() const override { return current_track_duration; }
    float GetCurrentPlaybackPosition() const override { return playback_position; }

private:
    std::string current_player_path;
    std::string current_track_title;
    std::string current_track_artist;
    float current_track_duration; // seconds
    float playback_position;      // seconds
    bool ignore_position_updates;
    float time_since_last_dbus_position;
    bool just_resumed;
    bool autoRefreshed;
    DBusConnection* dbus_conn;
    PlaybackState state;
    int volume;

    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(DBusMessage* msg);
    void HandleInterfacesAdded(DBusMessage* msg);
    float QueryCurrentPlaybackPosition();
    std::string QueryMediaPlayerStatus();
    void AutoRefresh();
    void SendVolumeUpdate(int vol);
};

#endif // BLUETOOTH_AUDIO_MANAGER_H
