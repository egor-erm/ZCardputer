#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include "sim.h"

enum NetworkType {
    NETWORK_NONE,
    NETWORK_WIFI,
    NETWORK_SIM
};

enum NetworkPriority {
    PRIORITY_WIFI_FIRST,   // Сначала пробуем WiFi, потом SIM
    PRIORITY_SIM_FIRST,    // Сначала SIM, потом WiFi
    PRIORITY_WIFI_ONLY,    // Только WiFi
    PRIORITY_SIM_ONLY      // Только SIM
};

class NetworkManager {
private:
    SimModule* simModule;
    NetworkType currentNetwork = NETWORK_NONE;
    NetworkPriority priority = PRIORITY_WIFI_FIRST;
    
    // WiFi credentials
    String wifiSSID = "";
    String wifiPassword = "";
    
    // Connection state
    bool isConnected = false;
    bool simEnabled = false;
    unsigned long lastScanTime = 0;
    unsigned long scanInterval = 60000; // Сканировать WiFi каждые 60 секунд
    
    // Internal methods
    bool connectToWiFi();
    bool connectToSIM();
    void disconnectWiFi();
    void disconnectSIM();
    bool scanForWiFi();
    
public:
    NetworkManager(SimModule* sim = nullptr);
    
    // Configuration
    void setPriority(NetworkPriority prio) { priority = prio; }
    void setWiFiCredentials(const String& ssid, const String& pass);
    void enableSIM(bool enable) { simEnabled = enable; }
    
    // Connection management
    bool connect();
    void disconnect();
    bool reconnect();
    
    // Status information
    bool isNetworkAvailable() const { return isConnected; }
    NetworkType getCurrentNetwork() const { return currentNetwork; }
    String getNetworkName() const;
    int getSignalStrength() const;
    
    // Power management
    void sleep();     // Перевести в режим энергосбережения
    void wakeup();    // Проснуться
    
    // Debug
    void printStatus();
};

#endif