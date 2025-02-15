#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include <string>
#include <vector>

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

#ifdef NO_DBUS
struct Track {
    std::string filepath;
    float duration; // Duration in seconds
};
#endif

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

#ifdef NO_DBUS
    void AddToPlaylist(const std::string& filepath, float duration);
    void ClearPlaylist();
#endif

    void Update(float delta_time);

    float GetPlaybackFraction() const;
    std::string GetTimeRemaining() const;

#ifndef NO_DBUS
    // Accessors for metadata in DBus mode
    std::string GetCurrentTrackTitle() const { return current_track_title; }
    std::string GetCurrentTrackArtist() const { return current_track_artist; }
    float GetCurrentTrackDuration() const { return current_track_duration; }
    float GetCurrentPlaybackPosition() const { return playback_position; }
#endif // NO_DBUS

private:
#ifdef NO_DBUS
    std::vector<Track> playlist;
    int current_track_index;
    float elapsed_time;
#else
    // Variables solely updated via DBus
    std::string current_player_path; // MediaPlayer1 path from DBus
    std::string current_track_title;
    std::string current_track_artist;
    float current_track_duration; // Duration in seconds (from Metadata)
    float playback_position;      // In seconds (from DBus "Position" updates)
    bool ignore_position_updates;
    float time_since_last_dbus_position;
    struct DBusConnection* dbus_conn;

    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(struct DBusMessage* msg);
    void SendVolumeUpdate(int vol);
#endif // NO_DBUS

    PlaybackState state;
    int volume;
};

#endif // BLUETOOTH_AUDIO_MANAGER_H
