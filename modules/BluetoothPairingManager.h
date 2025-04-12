#ifndef BLUETOOTH_PAIRING_MANAGER_H
#define BLUETOOTH_PAIRING_MANAGER_H

#include <dbus/dbus.h>
#include <string>

class BluetoothPairingManager {
public:
    BluetoothPairingManager();
    ~BluetoothPairingManager();

    // Initializes the DBus connection and registers the pairing agent
    bool Initialize();
    // Processes any pending DBus messages (call this periodically in your main loop)
    void Process();
    // Call this when the user presses "m" to confirm a pending pairing request
    void HandleMKey();

private:
    DBusConnection* dbus_conn;
    bool waitingForPairing;
    DBusMessage* pendingMessage; // Stores the pending pairing request message.
    std::string defaultPin;      // Default PIN to return (change if desired).

    // A static filter callback for handling incoming DBus messages for our agent.
    static DBusHandlerResult DBusMessageFilter(DBusConnection* connection, DBusMessage* message, void* user_data);
    // Registers the agent with BlueZ via AgentManager1.
    bool RegisterAgent();
};

#endif // BLUETOOTH_PAIRING_MANAGER_H
