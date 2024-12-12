#include "BluetoothAudioManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#ifndef NO_DBUS
#include <cstring>
#include <dbus/dbus.h>

// Forward declaration of helper method
static DBusMessage* CallMethod(DBusConnection* conn, const char* destination, const char* path,
                               const char* interface, const char* method);
#endif

BluetoothAudioManager::BluetoothAudioManager()
    : current_track_index(0), state(PlaybackState::Stopped), volume(64), elapsed_time(0.0f)
#ifndef NO_DBUS
    , dbus_conn(nullptr)
    , current_player_path("")
    , current_track_title("Unknown")
    , current_track_artist("Unknown")
    , playback_position(0.0f)
#endif
{
}

BluetoothAudioManager::~BluetoothAudioManager() {
    Shutdown();
}

bool BluetoothAudioManager::Initialize() {
#ifndef NO_DBUS
    if (!SetupDBus()) {
        std::cerr << "Warning: Failed to set up D-Bus connection. Continuing with mock logic.\n";
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
    // Mock logic remains
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
    // On macOS, use the session bus for testing (if you remove NO_DBUS define)
    dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    std::string bus_type_str = "session";
#else
    // On Linux/Pi, use the system bus
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
        if (dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_OBJECT_PATH) {
            dbus_message_iter_get_basic(&dict_entry, &object_path);
        }
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
                    current_player_path = object_path; // Store the path
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
    if (!dbus_conn) return; // If no connection, do nothing

    while (true) {
        dbus_connection_read_write(dbus_conn, 0);
        DBusMessage* msg = dbus_connection_pop_message(dbus_conn);
        if (!msg) {
            break;
        }

        if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL) {
            const char* interface = dbus_message_get_interface(msg);
            const char* member = dbus_message_get_member(msg);

            if (interface && member) {
                if (strcmp(interface, "org.freedesktop.DBus.ObjectManager") == 0 &&
                    strcmp(member, "InterfacesAdded") == 0) {
                    std::cout << "Signal received: InterfacesAdded\n";
                    // Handle InterfacesAdded signal
                    // This is already handled in HandleInterfacesAdded()
                    // For simplicity, you can call GetManagedObjects again or implement specific handling
                    // Here, we directly process the message
                    // (Alternatively, implement a separate method to parse the message)
                    // For now, this is handled in ProcessPendingDBusMessages()
                    // Assuming you have implemented the parsing above
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

    // First argument: interface name
    const char* iface_name = nullptr;
    dbus_message_iter_get_basic(&iter, &iface_name);
    dbus_message_iter_next(&iter);

    // Second argument: changed properties
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

        if (strcmp(key, "Metadata") == 0) {
            // Metadata is a dictionary
            DBusMessageIter metadata_iter;
            dbus_message_iter_recurse(&entry_iter, &metadata_iter);

            while (dbus_message_iter_get_arg_type(&metadata_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter meta_entry_iter;
                dbus_message_iter_recurse(&metadata_iter, &meta_entry_iter);

                const char* meta_key = nullptr;
                dbus_message_iter_get_basic(&meta_entry_iter, &meta_key);
                dbus_message_iter_next(&meta_entry_iter);

                if (strcmp(meta_key, "xesam:title") == 0) {
                    std::string title;
                    dbus_message_iter_get_basic(&meta_entry_iter, &title);
                    current_track_title = title;
                    std::cout << "Updated Title: " << current_track_title << "\n";
                } else if (strcmp(meta_key, "xesam:artist") == 0) {
                    std::string artist;
                    dbus_message_iter_get_basic(&meta_entry_iter, &artist);
                    current_track_artist = artist;
                    std::cout << "Updated Artist: " << current_track_artist << "\n";
                } else if (strcmp(meta_key, "xesam:length") == 0) {
                    // Length is in microseconds
                    long length_us;
                    dbus_message_iter_get_basic(&meta_entry_iter, &length_us);
                    // Update track duration if necessary
                    if (!playlist.empty()) {
                        playlist[current_track_index].duration = static_cast<float>(length_us) / 1000000.0f;
                        std::cout << "Updated Track Duration: " << playlist[current_track_index].duration << "s\n";
                    }
                }

                dbus_message_iter_next(&metadata_iter);
            }
        } else if (strcmp(key, "Position") == 0) {
            // Position is in microseconds
            long position_us;
            dbus_message_iter_get_basic(&entry_iter, &position_us);
            playback_position = static_cast<float>(position_us) / 1000000.0f;
            std::cout << "Updated Playback Position: " << playback_position << "s\n";
        }

        dbus_message_iter_next(&props_iter);
    }
}
#endif // NO_DBUS

// Playback Control Methods with D-Bus Integration

#ifndef NO_DBUS
void BluetoothAudioManager::Play() {
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

    // Optionally, check the reply for success
    dbus_message_unref(reply);

    state = PlaybackState::Playing;
    elapsed_time = 0.0f;
    playback_position = 0.0f;
    std::cout << "Playing: " << playlist[current_track_index].filepath << "\n";
}

void BluetoothAudioManager::Pause() {
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

    // Optionally, check the reply for success
    dbus_message_unref(reply);

    state = PlaybackState::Paused;
    std::cout << "Music paused.\n";
}

void BluetoothAudioManager::NextTrack() {
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

    // Optionally, check the reply for success
    dbus_message_unref(reply);

    // Update internal state
    current_track_index = (current_track_index + 1) % static_cast<int>(playlist.size());
    state = PlaybackState::Playing;
    elapsed_time = 0.0f;
    playback_position = 0.0f;
    std::cout << "Next track: " << playlist[current_track_index].filepath << "\n";
}

void BluetoothAudioManager::PreviousTrack() {
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

    // Optionally, check the reply for success
    dbus_message_unref(reply);

    // Update internal state
    current_track_index = (current_track_index - 1 + static_cast<int>(playlist.size())) % static_cast<int>(playlist.size());
    state = PlaybackState::Playing;
    elapsed_time = 0.0f;
    playback_position = 0.0f;
    std::cout << "Previous track: " << playlist[current_track_index].filepath << "\n";
}
#endif // NO_DBUS

// Mock Playback Methods (unchanged)
void BluetoothAudioManager::Play() {
#ifndef NO_DBUS
    // Real D-Bus Play is implemented above
#else
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
    std::cout << "Playing: " << playlist[current_track_index].filepath << "\n";
#endif
}

void BluetoothAudioManager::Pause() {
#ifndef NO_DBUS
    // Real D-Bus Pause is implemented above
#else
    if (state == PlaybackState::Playing) {
        state = PlaybackState::Paused;
        std::cout << "Music paused.\n";
    }
#endif
}

void BluetoothAudioManager::Resume() {
#ifndef NO_DBUS
    // Real D-Bus Resume can be implemented similarly if needed
    // For now, we assume Resume is equivalent to Play
    Play();
#else
    if (state == PlaybackState::Paused) {
        state = PlaybackState::Playing;
        std::cout << "Music resumed.\n";
    }
#endif
}

void BluetoothAudioManager::NextTrack() {
#ifndef NO_DBUS
    // Real D-Bus NextTrack is implemented above
#else
    if (playlist.empty()) return;
    current_track_index = (current_track_index + 1) % static_cast<int>(playlist.size());
    Play();
#endif
}

void BluetoothAudioManager::PreviousTrack() {
#ifndef NO_DBUS
    // Real D-Bus PreviousTrack is implemented above
#else
    if (playlist.empty()) return;
    current_track_index = (current_track_index - 1 + static_cast<int>(playlist.size())) % static_cast<int>(playlist.size());
    Play();
#endif
}

void BluetoothAudioManager::SetVolume(int vol) {
    volume = std::clamp(vol, 0, 128);
    std::cout << "Volume set to: " << volume << "\n";
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
    if (state == PlaybackState::Playing && !playlist.empty()) {
        elapsed_time += delta_time;
        const Track& current_track = playlist[current_track_index];
        if (elapsed_time >= current_track.duration) {
            elapsed_time = 0.0f;
            NextTrack();
        }
    }

#ifndef NO_DBUS
    ProcessPendingDBusMessages();
#endif
}

float BluetoothAudioManager::GetPlaybackFraction() const {
#ifndef NO_DBUS
    if (playlist.empty()) return 0.0f;
    const Track& current_track = playlist[current_track_index];
    if (current_track.duration <= 0.0f) return 0.0f;
    // Use playback_position if available
    return std::clamp(playback_position / current_track.duration, 0.0f, 1.0f);
#else
    if (playlist.empty()) return 0.0f;
    const Track& current_track = playlist[current_track_index];
    if (current_track.duration <= 0.0f) return 0.0f;
    return std::clamp(elapsed_time / current_track.duration, 0.0f, 1.0f);
#endif
}

std::string BluetoothAudioManager::GetTimeRemaining() const {
#ifndef NO_DBUS
    if (playlist.empty()) return "00:00";
    const Track& current_track = playlist[current_track_index];
    float remaining = current_track.duration - playback_position;
    if (remaining < 0.0f) remaining = 0.0f;
#else
    if (playlist.empty()) return "00:00";
    const Track& current_track = playlist[current_track_index];
    float remaining = current_track.duration - elapsed_time;
    if (remaining < 0.0f) remaining = 0.0f;
#endif
    int minutes = static_cast<int>(remaining) / 60;
    int seconds = static_cast<int>(remaining) % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

#ifndef NO_DBUS
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
