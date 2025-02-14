#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include <string>
#include <vector>

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

struct Track {
    std::string filepath;
    float duration; // Duration in seconds
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

    void AddToPlaylist(const std::string& filepath, float duration);
    void ClearPlaylist();

    void Update(float delta_time);

    float GetPlaybackFraction() const;
    std::string GetTimeRemaining() const;

#ifndef NO_DBUS
    std::string GetCurrentPlayerPath() const { return current_player_path; }
    std::string GetCurrentTrackTitle() const { return current_track_title; }
    std::string GetCurrentTrackArtist() const { return current_track_artist; }
    float GetCurrentPlaybackPosition() const { return playback_position; }
#endif // NO_DBUS

private:
    std::vector<Track> playlist;
    int current_track_index;
    PlaybackState state;
    int volume;
    float elapsed_time;
    float time_since_last_dbus_position;

#ifndef NO_DBUS
    struct DBusConnection* dbus_conn;
    std::string current_player_path; // Store MediaPlayer1 path
    std::string current_track_title;
    std::string current_track_artist;
    float playback_position; // In seconds

    // Flag to indicate whether to ignore incoming DBus position updates (e.g. when paused)
    bool ignore_position_updates;

    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(struct DBusMessage* msg);
    
    // New helper: send a volume update command to the phone.
    void SendVolumeUpdate(int vol);
#endif // NO_DBUS
};

#endif // BLUETOOTH_AUDIO_MANAGER_H
