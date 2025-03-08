#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include <string>
#include <dbus/dbus.h>

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class BluetoothAudioManager {
public:
    BluetoothAudioManager();
    ~BluetoothAudioManager();

    bool Initialize();
    void Shutdown();

    void Play();
    void Pause();
    void Resume();
    void NextTrack();
    void PreviousTrack();

    void SetVolume(int volume); // Volume range: 0-128
    int GetVolume() const;

    PlaybackState GetState() const;

    void Update(float delta_time);

    float GetPlaybackFraction() const;
    std::string GetTimeRemaining() const;

    // Accessors for DBus metadata.
    std::string GetCurrentTrackTitle() const { return current_track_title; }
    std::string GetCurrentTrackArtist() const { return current_track_artist; }
    float GetCurrentTrackDuration() const { return current_track_duration; }
    float GetCurrentPlaybackPosition() const { return playback_position; }

private:
    // MediaPlayer1 object path from DBus.
    std::string current_player_path;

    std::string current_track_title;
    std::string current_track_artist;
    float current_track_duration; // in seconds
    float playback_position;      // in seconds
    bool ignore_position_updates;
    float time_since_last_dbus_position;
    bool just_resumed;
    bool autoRefreshed;  // NEW: flag to ensure auto-refresh is triggered only once
    DBusConnection* dbus_conn;
    PlaybackState state;
    int volume;

    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(DBusMessage* msg);
    void HandleInterfacesAdded(DBusMessage* msg);

    // NEW: Query current playback position (in seconds).
    float QueryCurrentPlaybackPosition();

    // NEW: Query current media player status (e.g., "playing", "paused").
    std::string QueryMediaPlayerStatus();

    // NEW: Automatically refresh metadata by toggling playback.
    void AutoRefresh();

    void SendVolumeUpdate(int vol);
};

#endif // BLUETOOTH_AUDIO_MANAGER_H
