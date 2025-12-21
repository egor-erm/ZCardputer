#include "network.h"

NetworkManager::NetworkManager(SimModule* sim) 
    : simModule(sim), simEnabled(sim != nullptr) {
}

void NetworkManager::setWiFiCredentials(const String& ssid, const String& pass) {
    wifiSSID = ssid;
    wifiPassword = pass;
}

bool NetworkManager::scanForWiFi() {
    int networks = WiFi.scanNetworks();
    for (int i = 0; i < networks; i++) {
        if (WiFi.SSID(i) == wifiSSID) {
            return true;
        }
    }

    return false;
}

bool NetworkManager::connectToWiFi() {
    if (wifiSSID.length() == 0) return false;
    
    Serial.println("Connecting to WiFi: " + wifiSSID);
    
    WiFi.disconnect();
    delay(100);
    
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.println("IP address: " + WiFi.localIP().toString());
        return true;
    }
    
    Serial.println("\nWiFi connection failed");
    return false;
}

bool NetworkManager::connectToSIM() {
    if (!simEnabled || simModule == nullptr) return false;
    
    Serial.println("Connecting via SIM...");
    
    // Проверяем, работает ли SIM модуль
    if (!simModule->isDetected()) {
        Serial.println("SIM module not detected");
        return false;
    }
    
    // Будим модуль если нужно
    if (simModule->isModuleSleeping()) {
        if (!simModule->wakeup()) {
            Serial.println("Failed to wakeup SIM module");
            return false;
        }
    }
    
    // Проверяем регистрацию в сети
    if (!simModule->isRegistered()) {
        Serial.println("SIM not registered to network");
        return false;
    }
    
    Serial.println("SIM network available");
    return true;
}

void NetworkManager::disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        Serial.println("WiFi disconnected");
    }
}

void NetworkManager::disconnectSIM() {
    if (simEnabled && simModule != nullptr) {
        // Переводим SIM в режим сна
        simModule->sleep();
        Serial.println("SIM module put to sleep");
    }
}

bool NetworkManager::connect() {
    isConnected = false;
    currentNetwork = NETWORK_NONE;
    
    switch (priority) {
        case PRIORITY_WIFI_FIRST:
            if (scanForWiFi()) {
                if (connectToWiFi()) {
                    currentNetwork = NETWORK_WIFI;
                    isConnected = true;
                    // Выключаем SIM для экономии энергии
                    disconnectSIM();
                } else if (simEnabled) {
                    if (connectToSIM()) {
                        currentNetwork = NETWORK_SIM;
                        isConnected = true;
                    }
                }
            } else if (simEnabled) {
                if (connectToSIM()) {
                    currentNetwork = NETWORK_SIM;
                    isConnected = true;
                }
            }
            break;
            
        case PRIORITY_SIM_FIRST:
            if (simEnabled && connectToSIM()) {
                currentNetwork = NETWORK_SIM;
                isConnected = true;
                disconnectWiFi();
            } else if (connectToWiFi()) {
                currentNetwork = NETWORK_WIFI;
                isConnected = true;
            }
            break;
            
        case PRIORITY_WIFI_ONLY:
            if (connectToWiFi()) {
                currentNetwork = NETWORK_WIFI;
                isConnected = true;
            }
            break;
            
        case PRIORITY_SIM_ONLY:
            if (simEnabled && connectToSIM()) {
                currentNetwork = NETWORK_SIM;
                isConnected = true;
            }
            break;
    }
    
    if (isConnected) {
        Serial.println("Connected via " + getNetworkName());
    } else {
        Serial.println("No network available");
    }
    
    return isConnected;
}

void NetworkManager::disconnect() {
    disconnectWiFi();
    disconnectSIM();
    isConnected = false;
    currentNetwork = NETWORK_NONE;
    Serial.println("Network disconnected");
}

bool NetworkManager::reconnect() {
    disconnect();
    return connect();
}

String NetworkManager::getNetworkName() const {
    switch (currentNetwork) {
        case NETWORK_WIFI: return "WiFi: " + wifiSSID;
        case NETWORK_SIM: return "SIM Mobile";
        default: return "No Network";
    }
}

int NetworkManager::getSignalStrength() const {
    switch (currentNetwork) {
        case NETWORK_WIFI:
            if (WiFi.status() == WL_CONNECTED) {
                return WiFi.RSSI();
            }
            break;
            
        case NETWORK_SIM:
            // Здесь можно добавить получение уровня сигнала от SIM модуля
            // Для примера возвращаем фиктивное значение
            return -75;
            
        default:
            return 0;
    }
    return 0;
}

void NetworkManager::sleep() {
    Serial.println("NetworkManager: Going to sleep");
    
    // Отключаем WiFi полностью
    WiFi.disconnect();
    
    // Переводим SIM в сон
    if (simEnabled && simModule != nullptr) {
        simModule->sleep();
    }
    
    isConnected = false;
    currentNetwork = NETWORK_NONE;
}

void NetworkManager::wakeup() {
    Serial.println("NetworkManager: Waking up");
    
    // Инициализируем WiFi
    WiFi.begin();
    
    // Пробуждаем SIM если нужно
    if (simEnabled && simModule != nullptr && simModule->isModuleSleeping()) {
        simModule->wakeup();
    }
    
    // Автоматически переподключаемся
    connect();
}

void NetworkManager::printStatus() {
    Serial.println("=== Network Status ===");
    Serial.println("Current network: " + getNetworkName());
    Serial.println("Connected: " + String(isConnected ? "YES" : "NO"));
    Serial.println("Signal: " + String(getSignalStrength()) + " dBm");
    Serial.println("SIM enabled: " + String(simEnabled ? "YES" : "NO"));
    Serial.println("Priority: " + String(priority));
    Serial.println("======================");
}