#include "BluetoothAudioManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <dbus/dbus.h>
#include <thread>

// Helper method to call a DBus method.
static DBusMessage* CallMethod(DBusConnection* conn, const char* destination, const char* path,
                               const char* interface, const char* method) {
    DBusMessage* msg = dbus_message_new_method_call(destination, path, interface, method);
    if (!msg) {
        std::cerr << "DEBUG: CallMethod: Out of memory.\n";
        return nullptr;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus call error: " << err.message << "\n";
        dbus_error_free(&err);
        return nullptr;
    }
    return reply;
}

// -----------------------------------------------------------------------------
// Constructor and Destructor
// -----------------------------------------------------------------------------
BluetoothAudioManager::BluetoothAudioManager()
    : state(PlaybackState::Stopped),
      volume(64),
      current_player_path(""),
      current_track_title(""),
      current_track_artist(""),
      current_track_duration(0.0f),
      playback_position(0.0f),
      ignore_position_updates(false),
      time_since_last_dbus_position(0.0f),
      just_resumed(false),
      dbus_conn(nullptr)
{
    std::cout << "DEBUG: BluetoothAudioManager constructed.\n";
}

BluetoothAudioManager::~BluetoothAudioManager() {
    Shutdown();
    std::cout << "DEBUG: BluetoothAudioManager destructed.\n";
}

// -----------------------------------------------------------------------------
// Initialize and Shutdown
// -----------------------------------------------------------------------------
bool BluetoothAudioManager::Initialize() {
    std::cout << "DEBUG: Initializing DBus connection...\n";
    if (!SetupDBus()) {
        std::cerr << "DEBUG: Warning: Failed to set up D-Bus connection. Cannot get metadata.\n";
    } else {
        std::cout << "DEBUG: SetupDBus() successful.\n";
        if (!GetManagedObjects()) {
            std::cerr << "DEBUG: GetManagedObjects call failed or returned no data.\n";
        }
        ListenForSignals();
    }
    return true;
}

void BluetoothAudioManager::Shutdown() {
    if (dbus_conn) {
        dbus_connection_unref(dbus_conn);
        dbus_conn = nullptr;
        std::cout << "DEBUG: DBus connection shutdown.\n";
    }
}

// -----------------------------------------------------------------------------
// Playback Commands
// -----------------------------------------------------------------------------
void BluetoothAudioManager::Play() {
    std::cout << "DEBUG: Play() called.\n";
    if (current_player_path.empty()) {
        std::cerr << "DEBUG: No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Play");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create D-Bus Play method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Play method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    state = PlaybackState::Playing;
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    std::cout << "DEBUG: Playback state set to Playing.\n";
    RefreshMetadata();
}

void BluetoothAudioManager::Pause() {
    std::cout << "DEBUG: Pause() called.\n";
    if (current_player_path.empty()) {
        std::cerr << "DEBUG: No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Pause");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create D-Bus Pause method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Pause method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    state = PlaybackState::Paused;
    ignore_position_updates = true;
    std::cout << "DEBUG: Playback state set to Paused.\n";
    RefreshMetadata();
}

void BluetoothAudioManager::Resume() {
    std::cout << "DEBUG: Resume() called.\n";
    if (current_player_path.empty()) {
        std::cerr << "DEBUG: No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Play");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create D-Bus Play method call in Resume().\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Play method call error in Resume(): " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    
    state = PlaybackState::Playing;
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    if (playback_position < 0.001f) {
        playback_position = 0.01f;
        std::cout << "DEBUG: Forced playback_position to 0.01f on resume.\n";
    }
    just_resumed = true;
    std::cout << "DEBUG: Resume() completed, state set to Playing, just_resumed flag set.\n";
    RefreshMetadata();
}

void BluetoothAudioManager::NextTrack() {
    std::cout << "DEBUG: NextTrack() called.\n";
    if (current_player_path.empty()) {
        std::cerr << "DEBUG: No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Next");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create D-Bus Next method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Next method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    std::cout << "DEBUG: NextTrack() command sent.\n";
    RefreshMetadata();
}

void BluetoothAudioManager::PreviousTrack() {
    std::cout << "DEBUG: PreviousTrack() called.\n";
    if (current_player_path.empty()) {
        std::cerr << "DEBUG: No active MediaPlayer1 found.\n";
        return;
    }
    float current_pos = playback_position;
    if (current_pos > 5.0f) {
        playback_position = 0.0f;
        DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                        "org.bluez.MediaPlayer1", "Previous");
        if (!msg) {
            std::cerr << "DEBUG: Failed to create D-Bus Previous method call.\n";
            return;
        }
        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
        dbus_message_unref(msg);
        if (dbus_error_is_set(&err)) {
            std::cerr << "DEBUG: D-Bus Previous method call error: " << err.message << "\n";
            dbus_error_free(&err);
            return;
        }
        dbus_message_unref(reply);
        ignore_position_updates = false;
        time_since_last_dbus_position = 0.0f;
        std::cout << "DEBUG: PreviousTrack() - Reset track position to start.\n";
        RefreshMetadata();
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Previous");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create D-Bus Previous method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Previous method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    std::cout << "DEBUG: PreviousTrack() command sent.\n";
    RefreshMetadata();
}

// -----------------------------------------------------------------------------
// SetVolume, GetVolume, and GetState
// -----------------------------------------------------------------------------
void BluetoothAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    std::cout << "DEBUG: Volume set to: " << volume << "\n";
    SendVolumeUpdate(volume);
}

int BluetoothAudioManager::GetVolume() const {
    return volume;
}

PlaybackState BluetoothAudioManager::GetState() const {
    return state;
}

// -----------------------------------------------------------------------------
// Update and Playback Fraction
// -----------------------------------------------------------------------------
void BluetoothAudioManager::Update(float delta_time) {
    // Process pending DBus messages.
    ProcessPendingDBusMessages();

    // Periodic refresh: every 1 second, check for connection/disconnection.
    static float refresh_timer = 0.0f;
    refresh_timer += delta_time;
    if (refresh_timer >= 1.0f) {
        refresh_timer = 0.0f;
        std::string old_player = current_player_path;
        current_player_path = "";
        bool found = GetManagedObjects();
        if (!old_player.empty() && current_player_path.empty()) {
            std::cout << "DEBUG: MediaPlayer1 disconnected.\n";
            current_track_title = "";
            current_track_artist = "";
            current_track_duration = 0.0f;
            playback_position = 0.0f;
            state = PlaybackState::Stopped;
        } else if (old_player.empty() && !current_player_path.empty()) {
            std::cout << "DEBUG: New MediaPlayer1 found: " << current_player_path << "\n";
            RefreshMetadata();
        }
    }
    
    if (state == PlaybackState::Playing) {
        if (!ignore_position_updates) {
            time_since_last_dbus_position += delta_time;
            playback_position += delta_time;
            if (current_track_duration > 0 && playback_position >= current_track_duration) {
                playback_position = 0.0f;
                NextTrack();
            }
        }
    }
}

float BluetoothAudioManager::GetPlaybackFraction() const {
    return (current_track_duration > 0.0f) ? playback_position / current_track_duration : 0.0f;
}

std::string BluetoothAudioManager::GetTimeRemaining() const {
    float remaining = current_track_duration - playback_position;
    if (remaining < 0.0f)
        remaining = 0.0f;
    int minutes = static_cast<int>(remaining) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

// -----------------------------------------------------------------------------
// New Helper Method: RefreshMetadata()
// -----------------------------------------------------------------------------
void BluetoothAudioManager::RefreshMetadata() {
    if (current_player_path.empty()) return;
    
    // Send GetAll on the Properties interface to get metadata.
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.freedesktop.DBus.Properties", "GetAll");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create DBus GetAll message for metadata.\n";
        return;
    }
    const char* iface = "org.bluez.MediaPlayer1";
    dbus_message_append_args(msg,
                             DBUS_TYPE_STRING, &iface,
                             DBUS_TYPE_INVALID);
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, 5000, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: Error in RefreshMetadata: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    if (reply) {
        // For GetAll, the reply is a dictionary (a{sv}). Process it.
        DBusMessageIter iter;
        if (!dbus_message_iter_init(reply, &iter)) {
            std::cerr << "DEBUG: RefreshMetadata: no arguments in reply.\n";
            dbus_message_unref(reply);
            return;
        }
        int arg_type = dbus_message_iter_get_arg_type(&iter);
        if (arg_type == DBUS_TYPE_STRING) {
            dbus_message_iter_next(&iter);
        }
        ProcessMetadataProperties(&iter);
        dbus_message_unref(reply);
    }
}

// -----------------------------------------------------------------------------
// New Helper Method: ProcessMetadataProperties()
// -----------------------------------------------------------------------------
void BluetoothAudioManager::ProcessMetadataProperties(DBusMessageIter* props_iter) {
    while (dbus_message_iter_get_arg_type(props_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter;
        dbus_message_iter_recurse(props_iter, &entry_iter);
        const char* key = nullptr;
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        if (key && strcmp(key, "Metadata") == 0) {
            if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_VARIANT) {
                DBusMessageIter meta_variant;
                dbus_message_iter_recurse(&entry_iter, &meta_variant);
                if (dbus_message_iter_get_arg_type(&meta_variant) == DBUS_TYPE_ARRAY) {
                    DBusMessageIter meta_dict;
                    dbus_message_iter_recurse(&meta_variant, &meta_dict);
                    while (dbus_message_iter_get_arg_type(&meta_dict) == DBUS_TYPE_DICT_ENTRY) {
                        DBusMessageIter meta_entry;
                        dbus_message_iter_recurse(&meta_dict, &meta_entry);
                        const char* meta_key = nullptr;
                        dbus_message_iter_get_basic(&meta_entry, &meta_key);
                        dbus_message_iter_next(&meta_entry);
                        if (dbus_message_iter_get_arg_type(&meta_entry) == DBUS_TYPE_VARIANT) {
                            DBusMessageIter value_iter;
                            dbus_message_iter_recurse(&meta_entry, &value_iter);
                            int basic_type = dbus_message_iter_get_arg_type(&value_iter);
                            if ((strcmp(meta_key, "xesam:title") == 0 || strcmp(meta_key, "Title") == 0) &&
                                basic_type == DBUS_TYPE_STRING) {
                                const char* title = nullptr;
                                dbus_message_iter_get_basic(&value_iter, &title);
                                current_track_title = title ? title : "";
                                std::cout << "DEBUG: Updated Title: " << current_track_title << "\n";
                            } else if ((strcmp(meta_key, "xesam:artist") == 0 || strcmp(meta_key, "Artist") == 0)) {
                                if (basic_type == DBUS_TYPE_ARRAY) {
                                    DBusMessageIter artist_array;
                                    dbus_message_iter_recurse(&value_iter, &artist_array);
                                    if (dbus_message_iter_get_arg_type(&artist_array) == DBUS_TYPE_STRING) {
                                        const char* artist = nullptr;
                                        dbus_message_iter_get_basic(&artist_array, &artist);
                                        current_track_artist = artist ? artist : "";
                                        std::cout << "DEBUG: Updated Artist: " << current_track_artist << "\n";
                                    }
                                } else if (basic_type == DBUS_TYPE_STRING) {
                                    const char* artist = nullptr;
                                    dbus_message_iter_get_basic(&value_iter, &artist);
                                    current_track_artist = artist ? artist : "";
                                    std::cout << "DEBUG: Updated Artist: " << current_track_artist << "\n";
                                }
                            } else if ((strcmp(meta_key, "xesam:length") == 0 || strcmp(meta_key, "Duration") == 0)) {
                                if (basic_type == DBUS_TYPE_INT64) {
                                    dbus_int64_t length_val;
                                    dbus_message_iter_get_basic(&value_iter, &length_val);
                                    current_track_duration = (length_val < 1000000)
                                        ? static_cast<float>(length_val) / 1000.0f
                                        : static_cast<float>(length_val) / 1000000.0f;
                                    std::cout << "DEBUG: Updated Track Duration: " << current_track_duration << "s\n";
                                } else if (basic_type == DBUS_TYPE_UINT32) {
                                    uint32_t length_val;
                                    dbus_message_iter_get_basic(&value_iter, &length_val);
                                    current_track_duration = static_cast<float>(length_val) / 1000.0f;
                                    std::cout << "DEBUG: Updated Track Duration: " << current_track_duration << "s\n";
                                }
                            }
                        }
                        dbus_message_iter_next(&meta_dict);
                    }
                }
            }
        }
        dbus_message_iter_next(props_iter);
    }
}

void BluetoothAudioManager::SendVolumeUpdate(int vol) {
    int percentage = (vol * 100) / 128;
    std::string command = "pactl set-sink-volume alsa_output.platform-bcm2835_audio.analog-stereo " 
                          + std::to_string(percentage) + "%";
    std::thread([command](){
        std::system(command.c_str());
    }).detach();
}
