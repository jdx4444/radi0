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
    // Always process pending DBus messages.
    ProcessPendingDBusMessages();

    // Periodic refresh: every 1 second, check for connection/disconnection
    // and update metadata if needed.
    static float refresh_timer = 0.0f;
    refresh_timer += delta_time;
    if (refresh_timer >= 1.0f) {
        refresh_timer = 0.0f;
        // Save current connection state.
        std::string old_player = current_player_path;
        // Clear current state so GetManagedObjects() sets it fresh.
        current_player_path = "";
        bool found = GetManagedObjects();
        if (!old_player.empty() && current_player_path.empty()) {
            // Device was connected but now is gone.
            std::cout << "DEBUG: MediaPlayer1 disconnected.\n";
            current_track_title = "";
            current_track_artist = "";
            current_track_duration = 0.0f;
            playback_position = 0.0f;
            state = PlaybackState::Stopped;
        } else if (old_player.empty() && !current_player_path.empty()) {
            std::cout << "DEBUG: New MediaPlayer1 found: " << current_player_path << "\n";
            // Immediately refresh metadata.
            RefreshMetadata();
        }
    }
    
    // If playing, update playback position.
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
    
    // Call DBus method "GetAll" on the Properties interface for the current MediaPlayer1.
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
        // Process the reply similar to a PropertiesChanged signal.
        HandlePropertiesChanged(reply);
        dbus_message_unref(reply);
    }
}

// -----------------------------------------------------------------------------
// DBus Helper Functions
// -----------------------------------------------------------------------------
bool BluetoothAudioManager::SetupDBus() {
    DBusError err;
    dbus_error_init(&err);
    dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: D-Bus Error in SetupDBus: " << err.message << "\n";
        dbus_error_free(&err);
    }
    if (!dbus_conn) {
        std::cerr << "DEBUG: Failed to connect to D-Bus.\n";
        return false;
    }
    std::cout << "DEBUG: SetupDBus() successful.\n";
    return true;
}

bool BluetoothAudioManager::GetManagedObjects() {
    // Clear current connection state before scanning.
    current_player_path = "";
    
    DBusMessage* reply = CallMethod(dbus_conn, "org.bluez", "/",
                                    "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    if (!reply) {
        std::cerr << "DEBUG: GetManagedObjects call failed.\n";
        return false;
    }
    DBusMessageIter iter;
    if (!dbus_message_iter_init(reply, &iter)) {
        std::cerr << "DEBUG: GetManagedObjects: no arguments in reply.\n";
        dbus_message_unref(reply);
        return false;
    }
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        std::cerr << "DEBUG: GetManagedObjects: expected an array.\n";
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
                dbus_message_iter_next(&iface_entry);
                if (iface_name && strcmp(iface_name, "org.bluez.MediaPlayer1") == 0) {
                    std::cout << "DEBUG: Found MediaPlayer1 at: " << object_path << "\n";
                    current_player_path = object_path;
                    found_player = true;
                }
                dbus_message_iter_next(&iface_array);
            }
        }
        dbus_message_iter_next(&outer_array);
    }
    if (!found_player) {
        std::cout << "DEBUG: No MediaPlayer1 found via GetManagedObjects.\n";
    }
    dbus_message_unref(reply);
    return true;
}

void BluetoothAudioManager::ListenForSignals() {
    const char* rule_props = "type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',sender='org.bluez'";
    dbus_bus_add_match(dbus_conn, rule_props, NULL);
    dbus_connection_flush(dbus_conn);
    std::cout << "DEBUG: Listening for DBus signals...\n";
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
                if (strcmp(interface, "org.freedesktop.DBus.Properties") == 0 &&
                    strcmp(member, "PropertiesChanged") == 0) {
                    std::cout << "DEBUG: Signal received: PropertiesChanged\n";
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
        std::cerr << "DEBUG: PropertiesChanged signal has no arguments.\n";
        return;
    }
    const char* iface_name = nullptr;
    dbus_message_iter_get_basic(&iter, &iface_name);
    dbus_message_iter_next(&iter);
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        std::cerr << "DEBUG: PropertiesChanged: expected an array of properties.\n";
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
                                std::cout << "DEBUG: Updated Title: " << current_track_title << "\n";
                            }
                            else if ((strcmp(meta_key, "xesam:artist") == 0 || strcmp(meta_key, "Artist") == 0)) {
                                if (basic_type == DBUS_TYPE_ARRAY) {
                                    DBusMessageIter artist_array;
                                    dbus_message_iter_recurse(&inner_variant, &artist_array);
                                    std::string artists;
                                    // Iterate over each string in the array.
                                    while (dbus_message_iter_get_arg_type(&artist_array) == DBUS_TYPE_STRING) {
                                        const char* artist = nullptr;
                                        dbus_message_iter_get_basic(&artist_array, &artist);
                                        if (artist) {
                                            if (!artists.empty()) artists += ", ";
                                            artists += artist;
                                        }
                                        dbus_message_iter_next(&artist_array);
                                    }
                                    current_track_artist = artists;
                                    std::cout << "DEBUG: Updated Artist: " << current_track_artist << "\n";
                                }
                                else if (basic_type == DBUS_TYPE_STRING) {
                                    const char* artist = nullptr;
                                    dbus_message_iter_get_basic(&inner_variant, &artist);
                                    current_track_artist = artist ? artist : "";
                                    std::cout << "DEBUG: Updated Artist: " << current_track_artist << "\n";
                                }
                            }
                            else if ((strcmp(meta_key, "xesam:length") == 0 || strcmp(meta_key, "Duration") == 0)) {
                                if (basic_type == DBUS_TYPE_INT64) {
                                    dbus_int64_t length_val;
                                    dbus_message_iter_get_basic(&inner_variant, &length_val);
                                    current_track_duration = (length_val < 1000000)
                                        ? static_cast<float>(length_val) / 1000.0f
                                        : static_cast<float>(length_val) / 1000000.0f;
                                    std::cout << "DEBUG: Updated Track Duration: " << current_track_duration << "s\n";
                                } else if (basic_type == DBUS_TYPE_UINT32) {
                                    uint32_t length_val;
                                    dbus_message_iter_get_basic(&inner_variant, &length_val);
                                    current_track_duration = static_cast<float>(length_val) / 1000.0f;
                                    std::cout << "DEBUG: Updated Track Duration: " << current_track_duration << "s\n";
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
                        if (status == "paused" && just_resumed) {
                            std::cout << "DEBUG: Ignoring paused status due to just_resumed flag.\n";
                        } else if (status == "paused") {
                            state = PlaybackState::Paused;
                            ignore_position_updates = true;
                            std::cout << "DEBUG: Status update: paused\n";
                        } else if (status == "playing") {
                            state = PlaybackState::Playing;
                            std::cout << "DEBUG: Status update: playing\n";
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
                    std::cout << "DEBUG: Updated Volume from DBus: " << volume << "\n";
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
                    if (just_resumed || std::abs(new_position - playback_position) > 0.05f) {
                        playback_position = new_position;
                        time_since_last_dbus_position = 0.0f;
                        if (just_resumed)
                            just_resumed = false;
                        std::cout << "DEBUG: Updated Playback Position: " << playback_position << "s\n";
                    }
                }
            }
        }
        dbus_message_iter_next(&props_iter);
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
