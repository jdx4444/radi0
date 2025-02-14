#include "BluetoothAudioManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib> // For std::system()

#ifndef NO_DBUS
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

BluetoothAudioManager::BluetoothAudioManager()
    : current_track_index(0),
      state(PlaybackState::Stopped),
      volume(64),
      elapsed_time(0.0f)
#ifndef NO_DBUS
    , dbus_conn(nullptr),
      current_player_path(""),
      current_track_title(""),
      current_track_artist(""),
      playback_position(0.0f),
      ignore_position_updates(false),
      time_since_last_dbus_position(0.0f)
#endif
{
}

BluetoothAudioManager::~BluetoothAudioManager() {
    Shutdown();
}

bool BluetoothAudioManager::Initialize() {
#ifndef NO_DBUS
    if (!SetupDBus()) {
        std::cerr << "Warning: Failed to set up D-Bus connection. Cannot get metadata.\n";
    } else {
        std::cout << "D-Bus connection established successfully.\n";
        if (!GetManagedObjects()) {
            std::cerr << "GetManagedObjects call failed or returned no data.\n";
        }
        ListenForSignals();
    }
#else
    std::cout << "D-Bus disabled. Running mock logic only.\n";
#endif
    return true;
}

void BluetoothAudioManager::Shutdown() {
#ifndef NO_DBUS
    if (dbus_conn) {
        dbus_connection_unref(dbus_conn);
        dbus_conn = nullptr;
    }
#endif
}

#ifndef NO_DBUS
bool BluetoothAudioManager::SetupDBus() {
    DBusError err;
    dbus_error_init(&err);
#ifdef __APPLE__
    dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    std::string bus_type_str = "session";
#else
    dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    std::string bus_type_str = "system";
#endif
    if (dbus_error_is_set(&err)) {
        std::cerr << "D-Bus Error: " << err.message << "\n";
        dbus_error_free(&err);
    }
    if (!dbus_conn) {
        std::cerr << "Failed to connect to the D-Bus.\n";
        return false;
    }
    std::cout << "Successfully connected to the " << bus_type_str << " D-Bus.\n";
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
                // Accept MediaPlayer1
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
        std::cout << "No MediaPlayer1 found via GetManagedObjects (no device playing yet).\n";
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
    std::cout << "Listening for InterfacesAdded and PropertiesChanged signals...\n";
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
#endif // NO_DBUS

// -----------------------------------------------------------------------------
// HandlePropertiesChanged() processes "Metadata"/"Track", "Status", "Volume", and "Position".
// Duration and Position are accepted as either 32-bit (milliseconds) or 64-bit integers.
// Updates the playback state and resets the simulation timer on DBus updates.
// -----------------------------------------------------------------------------
void BluetoothAudioManager::HandlePropertiesChanged(DBusMessage* msg) {
    DBusMessageIter iter;
    if (!dbus_message_iter_init(msg, &iter)) {
        std::cerr << "PropertiesChanged signal has no arguments.\n";
        return;
    }
    // First argument: interface name (ignored)
    const char* iface_name = nullptr;
    dbus_message_iter_get_basic(&iter, &iface_name);
    dbus_message_iter_next(&iter);
    // Second argument: changed properties (an array)
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
        
        // Process "Metadata" or "Track" keys.
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
                                    if (length_val < 1000000)
                                        playlist[current_track_index].duration = static_cast<float>(length_val) / 1000.0f;
                                    else
                                        playlist[current_track_index].duration = static_cast<float>(length_val) / 1000000.0f;
                                    std::cout << "Updated Track Duration: " << playlist[current_track_index].duration << "s\n";
                                } else if (basic_type == DBUS_TYPE_UINT32) {
                                    uint32_t length_val;
                                    dbus_message_iter_get_basic(&inner_variant, &length_val);
                                    playlist[current_track_index].duration = static_cast<float>(length_val) / 1000.0f;
                                    std::cout << "Updated Track Duration: " << playlist[current_track_index].duration << "s\n";
                                }
                            }
                        }
                        dbus_message_iter_next(&array_iter);
                    }
                }
            }
        }
        // Process "Status" signals.
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
                            std::cout << "Status update: paused\n";
                        } else if (status == "playing") {
                            state = PlaybackState::Playing;
                            time_since_last_dbus_position = 0.0f;
                            std::cout << "Status update: playing\n";
                        }
                    }
                }
            }
        }
        // Process "Volume" signals.
        else if (strcmp(key, "Volume") == 0) {
            int type = dbus_message_iter_get_arg_type(&entry_iter);
            if (type == DBUS_TYPE_VARIANT) {
                DBusMessageIter vol_variant;
                dbus_message_iter_recurse(&entry_iter, &vol_variant);
                if (dbus_message_iter_get_arg_type(&vol_variant) == DBUS_TYPE_INT32) {
                    int vol;
                    dbus_message_iter_get_basic(&vol_variant, &vol);
                    volume = vol;
                    std::cout << "Updated Volume from phone: " << volume << "\n";
                }
            }
        }
        // Process "Position" signals – update only if playing.
        else if (strcmp(key, "Position") == 0) {
            if (state == PlaybackState::Playing) {
                int type = dbus_message_iter_get_arg_type(&entry_iter);
                if (type == DBUS_TYPE_VARIANT) {
                    DBusMessageIter variant_iter;
                    dbus_message_iter_recurse(&entry_iter, &variant_iter);
                    int pos_type = dbus_message_iter_get_arg_type(&variant_iter);
                    if (pos_type == DBUS_TYPE_INT64) {
                        dbus_int64_t pos_val;
                        dbus_message_iter_get_basic(&variant_iter, &pos_val);
                        if (pos_val < 1000000)
                            playback_position = static_cast<float>(pos_val) / 1000.0f;
                        else
                            playback_position = static_cast<float>(pos_val) / 1000000.0f;
                        time_since_last_dbus_position = 0.0f;
                        std::cout << "Updated Playback Position: " << playback_position << "s\n";
                    } else if (pos_type == DBUS_TYPE_UINT32) {
                        uint32_t pos_val;
                        dbus_message_iter_get_basic(&variant_iter, &pos_val);
                        playback_position = static_cast<float>(pos_val) / 1000.0f;
                        time_since_last_dbus_position = 0.0f;
                        std::cout << "Updated Playback Position: " << playback_position << "s\n";
                    }
                }
            }
        }
        dbus_message_iter_next(&props_iter);
    }
}

#ifndef NO_DBUS
// End of DBus-related functions.
#endif

// --- Unified Playback Methods ---

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
    playback_position = 0.0f;
    std::cout << "Playing (DBus): " << playlist[current_track_index].filepath << "\n";
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
    // For DBus, simply call Play() to resume.
    Play();
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
    current_track_index = (current_track_index + 1) % static_cast<int>(playlist.size());
    state = PlaybackState::Playing;
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    playback_position = 0.0f;
    std::cout << "Next track (DBus): " << playlist[current_track_index].filepath << "\n";
#endif
}

void BluetoothAudioManager::PreviousTrack() {
#ifdef NO_DBUS
    if (playlist.empty()) return;
    current_track_index = (current_track_index - 1 + static_cast<int>(playlist.size())) % static_cast<int>(playlist.size());
    Play();
#else
    if (playback_position > 5.0f) {
        playback_position = 0.0f;
        std::cout << "Reset current track position to start.\n";
        return;
    }
    if (current_player_path.empty()) {
        std::cerr << "No active MediaPlayer1 found.\n";
        return;
    }
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
    current_track_index = (current_track_index - 1 + static_cast<int>(playlist.size())) % static_cast<int>(playlist.size());
    state = PlaybackState::Playing;
    ignore_position_updates = false;
    time_since_last_dbus_position = 0.0f;
    playback_position = 0.0f;
    std::cout << "Previous track (DBus): " << playlist[current_track_index].filepath << "\n";
#endif
}

void BluetoothAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    std::cout << "Volume set to: " << volume << "\n";
#ifndef NO_DBUS
    // Instead of calling a DBus method (which fails), adjust the local output volume using ALSA.
    int percentage = (volume * 100) / 128;
    std::string command = "amixer set Master " + std::to_string(percentage) + "%";
    std::system(command.c_str());
#endif
}

int BluetoothAudioManager::GetVolume() const {
    return volume;
}

PlaybackState BluetoothAudioManager::GetState() const {
    return state;
}

void BluetoothAudioManager::AddToPlaylist(const std::string& filepath, float duration) {
    playlist.emplace_back(Track{filepath, duration});
    std::cout << "Added to playlist: " << filepath << " Duration: " << duration << "s\n";
}

void BluetoothAudioManager::ClearPlaylist() {
    playlist.clear();
    state = PlaybackState::Stopped;
    std::cout << "Playlist cleared.\n";
}

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
    if (state == PlaybackState::Playing && !playlist.empty()) {
        // Process incoming DBus messages (which may update metadata, state, position, volume, etc.)
        ProcessPendingDBusMessages();
        // Simulate continuous playback advancement if no recent DBus "Position" update has arrived.
        time_since_last_dbus_position += delta_time;
        if (time_since_last_dbus_position >= 0.1f) {
            playback_position += delta_time;
            const Track& current_track = playlist[current_track_index];
            if (playback_position >= current_track.duration) {
                playback_position = 0.0f;
                NextTrack();
            }
        }
    }
#endif
}

float BluetoothAudioManager::GetPlaybackFraction() const {
    if (playlist.empty()) return 0.0f;
    const Track& current_track = playlist[current_track_index];
    if (current_track.duration <= 0.0f) return 0.0f;
#ifdef NO_DBUS
    return std::clamp(elapsed_time / current_track.duration, 0.0f, 1.0f);
#else
    return std::clamp(playback_position / current_track.duration, 0.0f, 1.0f);
#endif
}

std::string BluetoothAudioManager::GetTimeRemaining() const {
    if (playlist.empty()) return "00:00";
    const Track& current_track = playlist[current_track_index];
#ifdef NO_DBUS
    float remaining = current_track.duration - elapsed_time;
#else
    float remaining = current_track.duration - playback_position;
#endif
    if (remaining < 0.0f) remaining = 0.0f;
    int minutes = static_cast<int>(remaining) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

#ifndef NO_DBUS
// -----------------------------------------------------------------------------
// SendVolumeUpdate() adjusts the local audio output volume using ALSA's amixer.
// This decouples the phone's (read-only) volume from the output volume.
// -----------------------------------------------------------------------------
void BluetoothAudioManager::SendVolumeUpdate(int vol) {
    int percentage = (vol * 100) / 128;
    std::string command = "amixer set Master " + std::to_string(percentage) + "%";
    std::system(command.c_str());
}
#endif
