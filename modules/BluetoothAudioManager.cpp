#include "BluetoothAudioManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib> // For std::system()

#ifdef NO_DBUS
  // NO_DBUS mode uses the local playlist (mock mode)
#else
#include <cstring>
#include <dbus/dbus.h>

// Helper method to call a DBus method.
static DBusMessage* CallMethod(DBusConnection* conn, const char* destination, const char* path,
                               const char* interface, const char* method) {
    DBusMessage* msg = dbus_message_new_method_call(destination, path, interface, method);
    if (!msg) {
        std::cerr << "CallMethod: Out of memory.\n";
        return nullptr;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus call error: " << err.message << "\n";
        dbus_error_free(&err);
        return nullptr;
    }
    return reply;
}
#endif

// -----------------------------------------------------------------------------
// Constructor and Destructor
// -----------------------------------------------------------------------------
BluetoothAudioManager::BluetoothAudioManager()
    : state(PlaybackState::Stopped),
      volume(64)
#ifdef NO_DBUS
    , current_track_index(0),
      elapsed_time(0.0f)
#else
    , current_player_path(""),
      current_track_title(""),
      current_track_artist(""),
      current_track_duration(0.0f),
      playback_position(0.0f),
      ignore_position_updates(false),
      time_since_last_dbus_position(0.0f),
      dbus_conn(nullptr)
#endif
{
}

BluetoothAudioManager::~BluetoothAudioManager() {
    Shutdown();
}

// -----------------------------------------------------------------------------
// Initialize and Shutdown
// -----------------------------------------------------------------------------
bool BluetoothAudioManager::Initialize() {
#ifdef NO_DBUS
    std::cout << "NO_DBUS mode enabled. Running mock logic only.\n";
#else
    if (!SetupDBus()) {
        std::cerr << "Warning: Failed to set up D-Bus connection. Cannot get metadata.\n";
    } else {
        std::cout << "D-Bus connection established successfully.\n";
        if (!GetManagedObjects()) {
            std::cerr << "GetManagedObjects call failed or returned no data.\n";
        }
        ListenForSignals();
    }
#endif
    return true;
}

void BluetoothAudioManager::Shutdown() {
#ifdef NO_DBUS
    // Nothing special to do in mock mode.
#else
    if (dbus_conn) {
        dbus_connection_unref(dbus_conn);
        dbus_conn = nullptr;
    }
#endif
}

// -----------------------------------------------------------------------------
// Playback Commands
// -----------------------------------------------------------------------------
void BluetoothAudioManager::Play() {
#ifdef NO_DBUS
    if (playlist.empty()) {
        std::cerr << "Playlist is empty.\n";
        return;
    }
    if (state == PlaybackState::Paused) {
        Resume();
        return;
    }
    state = PlaybackState::Playing;
    elapsed_time = 0.0f;
    std::cout << "Playing (mock): " << playlist[current_track_index].filepath << "\n";
#else
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Play");
    if (!msg) {
        std::cerr << "Failed to create D-Bus Play method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Play method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    state = PlaybackState::Playing;
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    std::cout << "Playing (DBus).\n";
#endif
}

void BluetoothAudioManager::Pause() {
#ifdef NO_DBUS
    if (state == PlaybackState::Playing) {
        state = PlaybackState::Paused;
        std::cout << "Music paused (mock).\n";
    }
#else
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Pause");
    if (!msg) {
        std::cerr << "Failed to create D-Bus Pause method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Pause method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    state = PlaybackState::Paused;
    ignore_position_updates = true;
    std::cout << "Music paused (DBus).\n";
#endif
}

void BluetoothAudioManager::Resume() {
#ifdef NO_DBUS
    if (state == PlaybackState::Paused) {
        state = PlaybackState::Playing;
        std::cout << "Music resumed (mock).\n";
    }
#else
    // In DBus mode, resuming is simply issuing Play again.
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
    
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Play");
    if (!msg) {
        std::cerr << "Failed to create D-Bus Play method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Play method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    
    // Ensure playback state is updated immediately
    state = PlaybackState::Playing;
    ignore_position_updates = false;  // Make sure this flag is reset
    time_since_last_dbus_position = 0.0f;  // Reset this timer
    
    std::cout << "Resumed (DBus).\n";
#endif
}

void BluetoothAudioManager::NextTrack() {
#ifdef NO_DBUS
    if (playlist.empty()) return;
    current_track_index = (current_track_index + 1) % static_cast<int>(playlist.size());
    Play();
#else
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
    // In DBus mode, only send the Next command.
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Next");
    if (!msg) {
        std::cerr << "Failed to create D-Bus Next method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Next method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    std::cout << "Next track (DBus) command sent.\n";
    // Do not update any local track index here.
#endif
}

void BluetoothAudioManager::PreviousTrack() {
#ifdef NO_DBUS
    if (playlist.empty()) return;
    // In mock mode, if more than 5 seconds into the track, restart.
    if (elapsed_time > 5.0f) {
        elapsed_time = 0.0f;
        std::cout << "Reset current track position to start (mock).\n";
        return;
    }
    current_track_index = (current_track_index - 1 + static_cast<int>(playlist.size())) % static_cast<int>(playlist.size());
    Play();
#else
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
    
    // Store the current position before making any changes
    float current_pos = playback_position;
    
    // In DBus mode, if more than 5 seconds have elapsed, restart current track.
    if (current_pos > 5.0f) {
        // Use a specific DBus call to reset position if available,
        // or just send a Previous and then Next to return to the same track
        // at the beginning
        playback_position = 0.0f;
        DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                    "org.bluez.MediaPlayer1", "Previous");
        if (!msg) {
            std::cerr << "Failed to create D-Bus Previous method call.\n";
            return;
        }
        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
        dbus_message_unref(msg);
        if (dbus_error_is_set(&err)) {
            std::cerr << "D-Bus Previous method call error: " << err.message << "\n";
            dbus_error_free(&err);
            return;
        }
        dbus_message_unref(reply);
        
        // Force an immediate state update
        ignore_position_updates = false;
        time_since_last_dbus_position = 0.0f;
        
        std::cout << "Reset current track position to start (DBus).\n";
        return;
    }
    
    // If we're not resetting the current track, go to previous track
    DBusMessage* msg = dbus_message_new_method_call("org.bluez", current_player_path.c_str(),
                                                "org.bluez.MediaPlayer1", "Previous");
    if (!msg) {
        std::cerr << "Failed to create D-Bus Previous method call.\n";
        return;
    }
    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Previous method call error: " << err.message << "\n";
        dbus_error_free(&err);
        return;
    }
    dbus_message_unref(reply);
    
    // Force an immediate state update
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    
    std::cout << "Previous track (DBus) command sent.\n";
#endif
}

// -----------------------------------------------------------------------------
// SetVolume, GetVolume, and GetState (common to both modes)
// -----------------------------------------------------------------------------
void BluetoothAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    std::cout << "Volume set to: " << volume << "\n";
#ifndef NO_DBUS
    SendVolumeUpdate(volume);
#else
    // In mock mode, you might simulate volume change.
#endif
}

int BluetoothAudioManager::GetVolume() const {
    return volume;
}

PlaybackState BluetoothAudioManager::GetState() const {
    return state;
}

#ifdef NO_DBUS
void BluetoothAudioManager::AddToPlaylist(const std::string& filepath, float duration) {
    playlist.push_back(Track{filepath, duration});
    std::cout << "Added to playlist: " << filepath << " Duration: " << duration << "s\n";
}

void BluetoothAudioManager::ClearPlaylist() {
    playlist.clear();
    state = PlaybackState::Stopped;
    std::cout << "Playlist cleared.\n";
}
#endif

// -----------------------------------------------------------------------------
// Update and Playback Fraction
// -----------------------------------------------------------------------------
void BluetoothAudioManager::Update(float delta_time) {
#ifdef NO_DBUS
    if (state == PlaybackState::Playing && !playlist.empty()) {
        elapsed_time += delta_time;
        const Track& current_track = playlist[current_track_index];
        if (elapsed_time >= current_track.duration) {
            elapsed_time = 0.0f;
            NextTrack();
        }
    }
#else
    if (state == PlaybackState::Playing) {
        ProcessPendingDBusMessages();
        
        // Update position even if we haven't received a recent DBus update
        if (!ignore_position_updates) {
            time_since_last_dbus_position += delta_time;
            playback_position += delta_time;
            
            // Check for end of track
            if (current_track_duration > 0 && playback_position >= current_track_duration) {
                playback_position = 0.0f;
                NextTrack();
            }
        }
    }
#endif
}

float BluetoothAudioManager::GetPlaybackFraction() const {
#ifdef NO_DBUS
    if (playlist.empty()) return 0.0f;
    const Track& current_track = playlist[current_track_index];
    return (current_track.duration > 0.0f) ? elapsed_time / current_track.duration : 0.0f;
#else
    return (current_track_duration > 0.0f) ? playback_position / current_track_duration : 0.0f;
#endif
}

std::string BluetoothAudioManager::GetTimeRemaining() const {
#ifdef NO_DBUS
    if (playlist.empty()) return "00:00";
    float remaining = playlist[current_track_index].duration - elapsed_time;
#else
    float remaining = current_track_duration - playback_position;
#endif
    if (remaining < 0.0f)
        remaining = 0.0f;
    int minutes = static_cast<int>(remaining) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

#ifndef NO_DBUS
// -----------------------------------------------------------------------------
// DBus Helper Functions (Setup, GetManagedObjects, Listen, Process, etc.)
// -----------------------------------------------------------------------------
bool BluetoothAudioManager::SetupDBus() {
    DBusError err;
    dbus_error_init(&err);
#ifdef __APPLE__
    dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
#else
    dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
#endif
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Error in SetupDBus: " << err.message << "\n";
        dbus_error_free(&err);
    }
    if (!dbus_conn) {
        std::cerr << "Failed to connect to D-Bus.\n";
        return false;
    }
    return true;
}

bool BluetoothAudioManager::GetManagedObjects() {
    DBusMessage* reply = CallMethod(dbus_conn, "org.bluez", "/",
                                    "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    if (!reply) {
        std::cerr << "GetManagedObjects call failed.\n";
        return false;
    }
    DBusMessageIter iter;
    if (!dbus_message_iter_init(reply, &iter)) {
        std::cerr << "GetManagedObjects: no arguments in reply.\n";
        dbus_message_unref(reply);
        return false;
    }
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        std::cerr << "GetManagedObjects: expected an array.\n";
        dbus_message_unref(reply);
        return false;
    }
    DBusMessageIter outer_array;
    dbus_message_iter_recurse(&iter, &outer_array);
    bool found_player = false;
    while (dbus_message_iter_get_arg_type(&outer_array) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter dict_entry, iface_array;
        dbus_message_iter_recurse(&outer_array, &dict_entry);
        const char* object_path = nullptr;
        if (dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_OBJECT_PATH)
            dbus_message_iter_get_basic(&dict_entry, &object_path);
        dbus_message_iter_next(&dict_entry);
        if (dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&dict_entry, &iface_array);
            while (dbus_message_iter_get_arg_type(&iface_array) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter iface_entry;
                dbus_message_iter_recurse(&iface_array, &iface_entry);
                const char* iface_name = nullptr;
                dbus_message_iter_get_basic(&iface_entry, &iface_name);
                dbus_message_iter_next(&iface_entry); // skip properties array
                if (iface_name && strcmp(iface_name, "org.bluez.MediaPlayer1") == 0) {
                    std::cout << "Found MediaPlayer1 at: " << object_path << "\n";
                    current_player_path = object_path;
                    found_player = true;
                }
                dbus_message_iter_next(&iface_array);
            }
        }
        dbus_message_iter_next(&outer_array);
    }
    if (!found_player) {
        std::cout << "No MediaPlayer1 found via GetManagedObjects.\n";
    }
    dbus_message_unref(reply);
    return true;
}

void BluetoothAudioManager::ListenForSignals() {
    const char* rule_added = "type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded',sender='org.bluez'";
    dbus_bus_add_match(dbus_conn, rule_added, NULL);
    dbus_connection_flush(dbus_conn);
    const char* rule_props = "type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',sender='org.bluez'";
    dbus_bus_add_match(dbus_conn, rule_props, NULL);
    dbus_connection_flush(dbus_conn);
    std::cout << "Listening for DBus signals...\n";
}

void BluetoothAudioManager::ProcessPendingDBusMessages() {
    if (!dbus_conn) return;
    while (true) {
        dbus_connection_read_write(dbus_conn, 0);
        DBusMessage* msg = dbus_connection_pop_message(dbus_conn);
        if (!msg)
            break;
        if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL) {
            const char* interface = dbus_message_get_interface(msg);
            const char* member = dbus_message_get_member(msg);
            if (interface && member) {
                if (strcmp(interface, "org.freedesktop.DBus.ObjectManager") == 0 &&
                    strcmp(member, "InterfacesAdded") == 0) {
                    std::cout << "Signal received: InterfacesAdded\n";
                } else if (strcmp(interface, "org.freedesktop.DBus.Properties") == 0 &&
                           strcmp(member, "PropertiesChanged") == 0) {
                    std::cout << "Signal received: PropertiesChanged\n";
                    HandlePropertiesChanged(msg);
                }
            }
        }
        dbus_message_unref(msg);
    }
}

void BluetoothAudioManager::HandlePropertiesChanged(DBusMessage* msg) {
    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        std::cerr << "PropertiesChanged signal has no arguments.\n";
        return;
    }
    // Skip the interface name.
    const char* iface_name = nullptr;
    dbus_message_iter_get_basic(&iter, &iface_name);
    dbus_message_iter_next(&iter);
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        std::cerr << "PropertiesChanged: expected an array of properties.\n";
        return;
    }
    DBusMessageIter props_iter;
    dbus_message_iter_recurse(&iter, &props_iter);
    while (dbus_message_iter_get_arg_type(&props_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter;
        dbus_message_iter_recurse(&props_iter, &entry_iter);
        const char* key = nullptr;
        dbus_message_iter_get_basic(&entry_iter, &key);
        dbus_message_iter_next(&entry_iter);
        
        // Process Metadata/Track changes.
        if (strcmp(key, "Metadata") == 0 || strcmp(key, "Track") == 0) {
            int variant_type = dbus_message_iter_get_arg_type(&entry_iter);
            if (variant_type == DBUS_TYPE_VARIANT) {
                DBusMessageIter variant_iter;
                dbus_message_iter_recurse(&entry_iter, &variant_iter);
                if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY) {
                    DBusMessageIter array_iter;
                    dbus_message_iter_recurse(&variant_iter, &array_iter);
                    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_DICT_ENTRY) {
                        DBusMessageIter dict_entry_iter;
                        dbus_message_iter_recurse(&array_iter, &dict_entry_iter);
                        const char* meta_key = nullptr;
                        dbus_message_iter_get_basic(&dict_entry_iter, &meta_key);
                        dbus_message_iter_next(&dict_entry_iter);
                        if (dbus_message_iter_get_arg_type(&dict_entry_iter) == DBUS_TYPE_VARIANT) {
                            DBusMessageIter inner_variant;
                            dbus_message_iter_recurse(&dict_entry_iter, &inner_variant);
                            int basic_type = dbus_message_iter_get_arg_type(&inner_variant);
                            if ((strcmp(meta_key, "xesam:title") == 0 || strcmp(meta_key, "Title") == 0) &&
                                basic_type == DBUS_TYPE_STRING) {
                                const char* title = nullptr;
                                dbus_message_iter_get_basic(&inner_variant, &title);
                                current_track_title = title ? title : "";
                                std::cout << "Updated Title: " << current_track_title << "\n";
                            }
                            else if ((strcmp(meta_key, "xesam:artist") == 0 || strcmp(meta_key, "Artist") == 0)) {
                                if (basic_type == DBUS_TYPE_ARRAY) {
                                    DBusMessageIter artist_array;
                                    dbus_message_iter_recurse(&inner_variant, &artist_array);
                                    if (dbus_message_iter_get_arg_type(&artist_array) == DBUS_TYPE_STRING) {
                                        const char* artist = nullptr;
                                        dbus_message_iter_get_basic(&artist_array, &artist);
                                        current_track_artist = artist ? artist : "";
                                        std::cout << "Updated Artist: " << current_track_artist << "\n";
                                    }
                                } else if (basic_type == DBUS_TYPE_STRING) {
                                    const char* artist = nullptr;
                                    dbus_message_iter_get_basic(&inner_variant, &artist);
                                    current_track_artist = artist ? artist : "";
                                    std::cout << "Updated Artist: " << current_track_artist << "\n";
                                }
                            }
                            else if ((strcmp(meta_key, "xesam:length") == 0 || strcmp(meta_key, "Duration") == 0)) {
                                if (basic_type == DBUS_TYPE_INT64) {
                                    dbus_int64_t length_val;
                                    dbus_message_iter_get_basic(&inner_variant, &length_val);
                                    current_track_duration = (length_val < 1000000)
                                        ? static_cast<float>(length_val) / 1000.0f
                                        : static_cast<float>(length_val) / 1000000.0f;
                                    std::cout << "Updated Track Duration: " << current_track_duration << "s\n";
                                } else if (basic_type == DBUS_TYPE_UINT32) {
                                    uint32_t length_val;
                                    dbus_message_iter_get_basic(&inner_variant, &length_val);
                                    current_track_duration = static_cast<float>(length_val) / 1000.0f;
                                    std::cout << "Updated Track Duration: " << current_track_duration << "s\n";
                                }
                            }
                        }
                        dbus_message_iter_next(&array_iter);
                    }
                }
            }
        }
        // Process "Status" changes.
        else if (strcmp(key, "Status") == 0) {
            int type = dbus_message_iter_get_arg_type(&entry_iter);
            if (type == DBUS_TYPE_VARIANT) {
                DBusMessageIter status_variant;
                dbus_message_iter_recurse(&entry_iter, &status_variant);
                if (dbus_message_iter_get_arg_type(&status_variant) == DBUS_TYPE_STRING) {
                    const char* status_str = nullptr;
                    dbus_message_iter_get_basic(&status_variant, &status_str);
                    if (status_str) {
                        std::string status(status_str);
                        if (status == "paused") {
                            state = PlaybackState::Paused;
                            ignore_position_updates = true;
                            std::cout << "Status update: paused\n";
                        } else if (status == "playing") {
                            state = PlaybackState::Playing;
                            std::cout << "Status update: playing\n";
                        }
                    }
                }
            }
        }
        // Process "Volume" changes.
        else if (strcmp(key, "Volume") == 0) {
            int type = dbus_message_iter_get_arg_type(&entry_iter);
            if (type == DBUS_TYPE_VARIANT) {
                DBusMessageIter vol_variant;
                dbus_message_iter_recurse(&entry_iter, &vol_variant);
                if (dbus_message_iter_get_arg_type(&vol_variant) == DBUS_TYPE_INT32) {
                    int vol;
                    dbus_message_iter_get_basic(&vol_variant, &vol);
                    volume = vol;
                    std::cout << "Updated Volume from DBus: " << volume << "\n";
                }
            }
        }
        // Process "Position" updates.
        else if (strcmp(key, "Position") == 0) {
            if (state == PlaybackState::Playing) {
                int type = dbus_message_iter_get_arg_type(&entry_iter);
                if (type == DBUS_TYPE_VARIANT) {
                    DBusMessageIter variant_iter;
                    dbus_message_iter_recurse(&entry_iter, &variant_iter);
                    int pos_type = dbus_message_iter_get_arg_type(&variant_iter);
                    float new_position = 0.0f;
                    if (pos_type == DBUS_TYPE_INT64) {
                        dbus_int64_t pos_val;
                        dbus_message_iter_get_basic(&variant_iter, &pos_val);
                        new_position = (pos_val < 1000000) ? static_cast<float>(pos_val) / 1000.0f
                                                          : static_cast<float>(pos_val) / 1000000.0f;
                    } else if (pos_type == DBUS_TYPE_UINT32) {
                        uint32_t pos_val;
                        dbus_message_iter_get_basic(&variant_iter, &pos_val);
                        new_position = static_cast<float>(pos_val) / 1000.0f;
                    }
                    if (std::abs(new_position - playback_position) > 0.05f) {
                        playback_position = new_position;
                        time_since_last_dbus_position = 0.0f;
                        std::cout << "Updated Playback Position: " << playback_position << "s\n";
                    }
                }
            }
        }
        dbus_message_iter_next(&props_iter);
    }
}

void BluetoothAudioManager::SendVolumeUpdate(int vol) {
    int percentage = (vol * 100) / 128;
    std::string command = "amixer set Master " + std::to_string(percentage) + "%";
    std::system(command.c_str());
}
#endif
