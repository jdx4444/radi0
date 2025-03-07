#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include <string>

// Forward declarations for DBus types
struct DBusConnection;
struct DBusMessage;

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

    void SetVolume(int volume); // Range: 0-128
    int GetVolume() const;

    PlaybackState GetState() const;

    void Update(float delta_time);

    float GetPlaybackFraction() const;
    std::string GetTimeRemaining() const;

    // Accessors for DBus metadata
    std::string GetCurrentTrackTitle() const { return current_track_title; }
    std::string GetCurrentTrackArtist() const { return current_track_artist; }
    float GetCurrentTrackDuration() const { return current_track_duration; }
    float GetCurrentPlaybackPosition() const { return playback_position; }

private:
    std::string current_player_path; // MediaPlayer1 path from DBus
    std::string current_track_title;
    std::string current_track_artist;
    float current_track_duration; // Duration in seconds (from Metadata)
    float playback_position;      // In seconds (from DBus "Position" updates)
    bool ignore_position_updates;
    float time_since_last_dbus_position;
    bool just_resumed;  // Flag to force update after resume
    DBusConnection* dbus_conn;
    PlaybackState state;
    int volume;

    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(DBusMessage* msg);
    void SendVolumeUpdate(int vol);

    // NEW: Helper method to refresh all metadata
    void RefreshMetadata();
};

#endif // BLUETOOTH_AUDIO_MANAGER_H
