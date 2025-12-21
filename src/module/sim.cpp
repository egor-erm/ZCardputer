#include "sim.h"
#include <Arduino.h>

// Включить/выключить подробную отладку
#define SIM_DEBUG 1

SimModule::SimModule(HardwareSerial* serial) : simSerial(serial), commandCounter(0) {}

bool SimModule::begin() {
    Serial.println("=== Initializing SIM module ===");
    
    // Инициализация UART для SIM7670
    simSerial->begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);
    delay(100);
    
    // Проверка наличия модуля
    String response = sendCommand("AT", 10000);
    
    moduleDetected = (response.indexOf("OK") != -1);
    if (moduleDetected) {
        Serial.println("✓ SIM module detected and responding");
        setupModule();
    } else {
        Serial.println("✗ SIM module NOT found or not responding");
        Serial.println("Response was: " + response);
    }
    
    return moduleDetected;
}

String SimModule::sendCommand(const String& cmd, unsigned long timeout, bool waitForOK) {
    if (!moduleDetected && cmd != "AT") {
        Serial.println("WARNING: Module not detected, skipping command: " + cmd);
        return "";
    }
    
    commandCounter++;
    
    #if SIM_DEBUG
    Serial.print("[CMD #");
    Serial.print(commandCounter);
    Serial.print("] Sending: ");
    Serial.println(cmd);
    #endif
    
    // Очищаем буфер перед отправкой
    clearBuffer();
    
    // Отправляем команду
    simSerial->println(cmd);
    
    // Ждем ответа
    String response = readResponse(timeout, true);
    
    #if SIM_DEBUG
    Serial.print("[CMD #");
    Serial.print(commandCounter);
    Serial.print("] Response: ");
    // Выводим response с экранированием новых строк для читаемости
    String debugResponse = response;
    debugResponse.replace("\r", "\\r");
    debugResponse.replace("\n", "\\n");
    if (debugResponse.length() > 100) {
        debugResponse = debugResponse.substring(0, 100) + "...";
    }
    Serial.println(debugResponse);
    
    // Проверяем наличие OK/ERROR
    if (waitForOK) {
        if (response.indexOf("OK") != -1) {
            Serial.println("[CMD #" + String(commandCounter) + "] ✓ OK received");
        } else if (response.indexOf("ERROR") != -1) {
            Serial.println("[CMD #" + String(commandCounter) + "] ✗ ERROR received");
        } else {
            Serial.println("[CMD #" + String(commandCounter) + "] ? No OK/ERROR in response");
        }
    }
    #endif
    
    return response;
}

void SimModule::clearBuffer() {
    unsigned long start = millis();
    int cleared = 0;
    
    while (simSerial->available() && (millis() - start < 100)) {
        simSerial->read();
        cleared++;
        delay(1);
    }
    
    #if SIM_DEBUG
    if (cleared > 0) {
        Serial.println("[BUFFER] Cleared " + String(cleared) + " bytes from UART buffer");
    }
    #endif
}

String SimModule::readResponse(unsigned long timeout, bool debug) {
    String response;
    unsigned long start = millis();
    
    while (millis() - start < timeout) {
        if (simSerial->available()) {
            char c = simSerial->read();
            response += c;
        }
    }
    
    #if SIM_DEBUG
    if (debug && response.length() > 0) {
        Serial.print("[READ] Got ");
        Serial.print(response.length());
        Serial.println(" bytes");
    }
    #endif
    
    return response;
}

bool SimModule::sleep() {
    if (!moduleDetected || isSleeping) return true;
    
    Serial.println("=== SIM7600: Entering low power mode ===");
    
    // Для SIM7600 без звонков оптимально использовать CFUN=4 (airplane mode)
    // Отключает RF цепи, но модуль отвечает на AT команды
    String resp = sendCommand("AT+CFUN=4", 3000);
    
    if (resp.indexOf("OK") != -1) {
        isSleeping = true;
        Serial.println("✓ SIM module in airplane mode (low power)");
        
        #if SIM_DEBUG
        // Проверяем текущий режим
        String check = sendCommand("AT+CFUN?", 1000);
        Serial.println("Current CFUN mode: " + check);
        #endif
        
    } else {
        // Альтернатива: минимальный функционал
        Serial.println("Trying CFUN=0...");
        resp = sendCommand("AT+CFUN=0", 3000);
        
        if (resp.indexOf("OK") != -1) {
            isSleeping = true;
            Serial.println("✓ SIM module in minimum functionality mode");
        } else {
            Serial.println("✗ Failed to put SIM to sleep");
            return false;
        }
    }

    printDebugInfo();
    return true;
}

bool SimModule::wakeup() {
    if (!moduleDetected || !isSleeping) return true;
    
    Serial.println("=== Waking up SIM module ===");
    
    // Выходим из режима сна
    String resp = sendCommand("AT+CFUN=1", 5000); // Долгая инициализация
    
    // Ждем регистрации в сети
    unsigned long start = millis();
    bool networkReady = false;
    
    while (millis() - start < 15000) { // Ждем до 15 секунд
        String resp = sendCommand("AT+CREG?", 2000);
        
        if (resp.indexOf("+CREG: 0,1") != -1 || 
            resp.indexOf("+CREG: 0,5") != -1) {
            registered = true;
            networkReady = true;
            Serial.println("✓ Network registered successfully");
            break;
        }
        
        if (millis() - start > 5000 && !networkReady) {
            // Через 5 секунд пробуем принудительную регистрацию
            sendCommand("AT+COPS=0,0", 3000);
        }
        
        delay(1000);
    }
    
    if (!networkReady) {
        Serial.println("⚠ Network registration timeout");
    }
    
    isSleeping = false;
    Serial.println("✓ SIM module awake and ready");
    
    return true;
}

void SimModule::setupModule() {
    Serial.println("=== Setting up SIM module ===");
    
    // 1. Расширенные коды ошибок
    sendCommand("AT+CMEE=2", 1000);
    
    // 2. Выключаем эхо
    sendCommand("ATE0", 1000);
    
    // 3. Включаем полный функционал
    sendCommand("AT+CFUN=1", 2000);
    
    // 4. Проверяем базовую информацию
    sendCommand("ATI", 1000);

    // 5. Переводим модуль в режим полёта
    sendCommand("AT+CFUN=4", 2000);
    
    Serial.println("✓ SIM module setup complete");
}

void SimModule::printDebugInfo() {
    Serial.println("=== SIM Module Debug Info ===");
    Serial.println("Module detected: " + String(moduleDetected ? "YES" : "NO"));
    Serial.println("Module sleeping: " + String(isSleeping ? "YES" : "NO"));
    Serial.println("Network registered: " + String(registered ? "YES" : "NO"));
    Serial.println("Commands sent: " + String(commandCounter));
    Serial.println("UART available: " + String(simSerial->available()));
    Serial.println("=============================");
}