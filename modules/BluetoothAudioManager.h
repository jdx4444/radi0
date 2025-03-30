#ifndef BLUETOOTH_AUDIO_MANAGER_H
#define BLUETOOTH_AUDIO_MANAGER_H

#include "IAudioManager.h"
#include <string>
#include <dbus/dbus.h>


// ...
class BluetoothAudioManager : public IAudioManager {
public:
    BluetoothAudioManager();
    virtual ~BluetoothAudioManager();
    
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
        
    // Inline method to check if a phone is paired (i.e. if MediaPlayer1 was found).
    bool IsPaired() const { return !current_player_path.empty(); }
    
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
    bool autoRefreshed;  // flag to ensure auto-refresh is triggered only once
    DBusConnection* dbus_conn;
    PlaybackState state;
    int volume;
    
    bool SetupDBus();
    bool GetManagedObjects();
    void ListenForSignals();
    void ProcessPendingDBusMessages();
    void HandlePropertiesChanged(DBusMessage* msg);
    void HandleInterfacesAdded(DBusMessage* msg);
    
    // Query current playback position (in seconds).
    float QueryCurrentPlaybackPosition();
    
    // Query current media player status (e.g., "playing", "paused").
    std::string QueryMediaPlayerStatus();
    
    // Automatically refresh metadata by toggling playback.
    void AutoRefresh();
    
    void SendVolumeUpdate(int vol);
};
    
#endif // BLUETOOTH_AUDIO_MANAGER_H
    
