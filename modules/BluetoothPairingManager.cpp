#include "BluetoothPairingManager.h"
#include <iostream>
#include <cstring>

// Constructor: Initialize members.
BluetoothPairingManager::BluetoothPairingManager()
    : dbus_conn(nullptr),
      waitingForPairing(false),
      pendingMessage(nullptr),
      defaultPin("0000") // Default PIN code; adjust as needed.
{
}

// Destructor: Clean up any pending message and the DBus connection.
BluetoothPairingManager::~BluetoothPairingManager() {
    if (pendingMessage) {
        dbus_message_unref(pendingMessage);
        pendingMessage = nullptr;
    }
    if (dbus_conn) {
        dbus_connection_unref(dbus_conn);
        dbus_conn = nullptr;
    }
    std::cout << "DEBUG: BluetoothPairingManager shutdown.\n";
}

// Initialize the DBus connection and register the pairing agent.
bool BluetoothPairingManager::Initialize() {
    DBusError err;
    dbus_error_init(&err);
    dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: DBus Error in BluetoothPairingManager::Initialize: " << err.message << "\n";
        dbus_error_free(&err);
        return false;
    }
    if (!dbus_conn) {
        std::cerr << "DEBUG: Failed to get DBus connection in BluetoothPairingManager::Initialize.\n";
        return false;
    }
    std::cout << "DEBUG: BluetoothPairingManager DBus connection established.\n";

    if (!RegisterAgent()) {
        std::cerr << "DEBUG: Failed to register Bluetooth pairing agent.\n";
        return false;
    }

    // Add a filter so that our callback is called for messages to our agent.
    dbus_connection_add_filter(dbus_conn, DBusMessageFilter, this, nullptr);
    return true;
}

// Registers the pairing agent with BlueZ via the AgentManager1 interface.
bool BluetoothPairingManager::RegisterAgent() {
    const char* agent_path = "/com/yourapp/bluetooth/agent";
    // Use a capability that requires user confirmation (here, KeyboardDisplay).
    const char* capabilities = "KeyboardDisplay";
    DBusMessage* msg = dbus_message_new_method_call("org.bluez",
                                                    "/org/bluez",
                                                    "org.bluez.AgentManager1",
                                                    "RegisterAgent");
    if (!msg) {
        std::cerr << "DEBUG: Failed to create DBus message for RegisterAgent.\n";
        return false;
    }

    dbus_message_append_args(msg,
                             DBUS_TYPE_OBJECT_PATH, &agent_path,
                             DBUS_TYPE_STRING, &capabilities,
                             DBUS_TYPE_INVALID);

    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        std::cerr << "DEBUG: Error registering agent: " << err.message << "\n";
        dbus_error_free(&err);
        return false;
    }

    if (reply) {
        dbus_message_unref(reply);
    }
    std::cout << "DEBUG: Bluetooth pairing agent registered successfully.\n";
    return true;
}

// Process pending DBus messages (call this from your main loop).
void BluetoothPairingManager::Process() {
    if (dbus_conn) {
        // Read and dispatch messages so that our filter is triggered.
        dbus_connection_read_write(dbus_conn, 0);
        dbus_connection_dispatch(dbus_conn);
    }
}

// This method is called when the user presses the "m" key.
// It checks if there is a pending pairing request (such as a PIN code request)
// and, if so, replies with the default PIN or confirms the request.
void BluetoothPairingManager::HandleMKey() {
    if (!waitingForPairing || !pendingMessage) {
        std::cout << "DEBUG: No pending pairing request to confirm.\n";
        return;
    }

    const char* method = dbus_message_get_member(pendingMessage);
    DBusMessage* reply = nullptr;

    if (strcmp(method, "RequestPinCode") == 0) {
        std::cout << "DEBUG: Handling RequestPinCode, returning PIN: " << defaultPin << "\n";
        reply = dbus_message_new_method_return(pendingMessage);
        const char* pin = defaultPin.c_str();
        dbus_message_append_args(reply,
                                 DBUS_TYPE_STRING, &pin,
                                 DBUS_TYPE_INVALID);
    } else if (strcmp(method, "RequestConfirmation") == 0) {
        std::cout << "DEBUG: Handling RequestConfirmation, accepting passkey confirmation.\n";
        reply = dbus_message_new_method_return(pendingMessage);
        // RequestConfirmation returns no arguments.
    } else {
        std::cout << "DEBUG: Received unhandled pairing method: " << method << "\n";
    }

    if (reply) {
        dbus_connection_send(dbus_conn, reply, nullptr);
        dbus_connection_flush(dbus_conn);
        dbus_message_unref(reply);
    }

    dbus_message_unref(pendingMessage);
    pendingMessage = nullptr;
    waitingForPairing = false;
    std::cout << "DEBUG: Pairing request handled and confirmed via 'm' key.\n";
}

// Static DBus filter callback.
// It checks incoming DBus messages for our agent object path and for pairing requests.
DBusHandlerResult BluetoothPairingManager::DBusMessageFilter(DBusConnection* connection, DBusMessage* message, void* user_data) {
    BluetoothPairingManager* pairingManager = static_cast<BluetoothPairingManager*>(user_data);

    // Only handle messages sent to our agent object path.
    const char* path = dbus_message_get_path(message);
    if (!path || strcmp(path, "/com/yourapp/bluetooth/agent") != 0) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    // Process method calls from BlueZ to our agent.
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_METHOD_CALL) {
        const char* interface = dbus_message_get_interface(message);
        const char* member = dbus_message_get_member(message);
        if (interface && strcmp(interface, "org.bluez.Agent1") == 0) {
            if ((strcmp(member, "RequestPinCode") == 0 || strcmp(member, "RequestConfirmation") == 0)) {
                if (!pairingManager->waitingForPairing) {
                    // Save the message so that it can be replied to when the user presses "m".
                    pairingManager->pendingMessage = dbus_message_ref(message);
                    pairingManager->waitingForPairing = true;
                    std::cout << "DEBUG: Pairing request received (" << member << "). Press 'm' to confirm.\n";
                    // We do not reply immediately; we wait for user confirmation.
                    return DBUS_HANDLER_RESULT_HANDLED;
                } else {
                    std::cout << "DEBUG: Already waiting for a pairing confirmation. Ignoring additional request.\n";
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
            }
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
