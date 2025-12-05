#include "sim.h"
#include <Arduino.h>

SimModule::SimModule(HardwareSerial* serial) : simSerial(serial) {}

bool SimModule::begin() {
    // Инициализация UART для SIM7670
    simSerial->begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);
    delay(2000);

    while (simSerial->available()) {
        simSerial->read();
    }
    
    // Проверка наличия модуля
    simSerial->println("AT");
    delay(100);
    String response = readResponse(3000);
    
    moduleDetected = (response.indexOf("OK") != -1);
    if (moduleDetected) {
        Serial.println("SIM module detected");
        setupModule();
    } else {
        Serial.println("SIM module NOT found");
    }
    
    return moduleDetected;
}

bool SimModule::sleep() {
    if (!moduleDetected || isSleeping) return true;
    
    Serial.println("Putting SIM module to deep sleep (with paging)...");

    // === ШАГ 1: Настраиваем Power Saving Mode (PSM) ===
    // T3324 (Active Time): 00100001 = 20 секунд
    // T3412 (Periodic TAU): 00100001 = 2 часа
    simSerial->println("AT+CPSMS=1,,,\"00100001\",\"00100001\"");
    delay(500);
    
    // === ШАГ 2: Активируем eDRX (дополнительная экономия) ===
    // Requested eDRX value: 0001 = 20.48 секунд (для 2G)
    simSerial->println("AT+CEDRXS=1,4,\"0001\"");
    delay(500);
    
    // === ШАГ 3: ГЛАВНАЯ КОМАНДА - Сон с пейджингом! ===
    // AT+CFUN=9 - Minimum functionality with paging
    // AT+CFUN=0 - самый глубокий сон (но не принимает вызовы!)
    // AT+CFUN=1 - полный функционал
    // AT+CFUN=4 - airplane mode
    // AT+CFUN=9 - sleep with paging (оптимально для приема вызовов)
    simSerial->println("AT+CFUN=9");
    delay(1000);
    
    // Альтернативный вариант (для SIM7670 может работать лучше):
    // simSerial->println("AT+CSCLK=2"); // Sleep with UART wakeup
    
    // === ШАГ 4: Проверяем, что модуль перешел в режим сна ===
    simSerial->println("AT+CFUN?");
    delay(200);
    String response = readResponse(1000);
    
    if (response.indexOf("+CFUN: 9") != -1 || 
        response.indexOf("+CFUN: 0") != -1) {
        Serial.println("SIM module successfully entered sleep mode");
    } else {
        Serial.println("Warning: SIM module may not be sleeping");
        // Пробуем альтернативный метод
        simSerial->println("AT+CSCLK=2");
        delay(500);
    }
    
    // === ШАГ 5: Отключаем все ненужные функции ===
    simSerial->println("AT+CGATT?");        // Проверяем GPRS attachment
    
    // === ШАГ 6: Настраиваем RI (Ring Indicator) сигнал ===
    // Важно: RI будет срабатывать при входящем звонке даже в режиме сна
    simSerial->println("AT+CRISET=1");
    delay(100);
    simSerial->println("AT+CRFPINS=1,1,0,0"); // Настраиваем выводы
    
    isSleeping = true;
    Serial.println("SIM module in deep sleep (CFUN=9) - ready for calls!");
    
    return true;
}

bool SimModule::wakeup() {
    if (!moduleDetected || !isSleeping) return true;
    
    Serial.println("Waking up SIM module from CFUN=9 sleep...");
    
    // === ШАГ 1: Выходим из режима сна ===
    // Для выхода из CFUN=9 нужна команда AT+CFUN=1
    simSerial->println("AT+CFUN=1");
    delay(2000); // Даем время на полную инициализацию
    
    // === ШАГ 2: Отключаем sleep mode если использовался CSCLK ===
    simSerial->println("AT+CSCLK=0");
    delay(500);
    
    // === ШАГ 3: Выходим из PSM режима ===
    simSerial->println("AT+CPSMS=0");
    delay(500);
    
    // === ШАГ 4: Включаем необходимые функции ===
    simSerial->println("ATE0");           // Выключаем эхо
    delay(100);
    simSerial->println("AT+CLIP=1");      // Включаем определение номера
    delay(100);
    
    // === ШАГ 5: Ждем регистрации в сети ===
    unsigned long start = millis();
    bool networkReady = false;
    
    while (millis() - start < 10000) { // Ждем до 10 секунд
        simSerial->println("AT+CREG?");
        delay(200);
        String response = readResponse(1000);
        
        if (response.indexOf("+CREG: 0,1") != -1 || 
            response.indexOf("+CREG: 0,5") != -1) {
            registered = true;
            networkReady = true;
            Serial.println("Network registered successfully");
            break;
        }
        delay(1000);
    }
    
    if (!networkReady) {
        Serial.println("Warning: Network registration timeout");
        // Можно попробовать принудительный поиск сети
        simSerial->println("AT+COPS=0,0"); // Автоматический выбор сети
    }

    isSleeping = false;
    Serial.println("SIM module awake");

    return true;
}

bool SimModule::checkWakeupByCall() {
    if (!moduleDetected) return false;
    
    // Быстрая проверка буфера
    if (simSerial->available()) {
        String response = readResponse(50);  // Очень быстро
        
        if (response.indexOf("RING") != -1 || 
            response.indexOf("+CLIP:") != -1) {
            Serial.println("Woke up by incoming call!");
            return true;
        }
    }
    
    return false;
}

bool SimModule::checkForCallOrSMS() {
    if (!moduleDetected) return false;
    
    // Если модуль спит - используем специальную проверку
    if (isSleeping) {
        return checkWakeupByCall();
    }
    
    // Если модуль не спит - обычная проверка
    if (simSerial->available()) {
        String response = readResponse(100);
        
        if (response.indexOf("RING") != -1 || 
            response.indexOf("+CLIP:") != -1) {
            Serial.println("Incoming call detected!");
            return true;
        }
        
        if (response.indexOf("+CMTI:") != -1) {
            Serial.println("New SMS detected!");
            return true;
        }
    }
    
    return false;
}

void SimModule::update() {
    if (!moduleDetected) return;
    
    // Если модуль спит - только быстрая проверка
    if (isSleeping) {
        checkForCallOrSMS();
        return;
    }
    
    // Проверка регистрации в сети каждые 30 секунд
    if (millis() - lastCheck > 30000) {
        simSerial->println("AT+CREG?");
        String response = readResponse(2000);
        
        if (response.indexOf("+CREG: 0,1") != -1 || 
            response.indexOf("+CREG: 0,5") != -1) {
            registered = true;
        } else {
            registered = false;
            // Пытаемся перерегистрироваться
            simSerial->println("AT+COPS?");
        }
        
        lastCheck = millis();
    }
}

String SimModule::readResponse(unsigned long timeout) {
    String response;
    unsigned long start = millis();
    
    while (millis() - start < timeout) {
        if (simSerial->available()) {
            char c = simSerial->read();
            response += c;
        }
    }

    Serial.println("New response: " + response);
    return response;
}

void SimModule::setupModule() {
    simSerial->println("AT+CMEE=2"); // Расширенные коды ошибок
    delay(100);

    simSerial->println("ATE0"); // Выключить эхо
    delay(100);

    simSerial->println("AT+CFUN=1"); // Полный функционал
    delay(1000);

    simSerial->println("AT+CGNSPWR=0"); // Выключаем GPS
    delay(100);

    simSerial->println("AT+CGNSURC=0"); // Выключаем периодические GPS отчеты
    delay(100);
        
    // Настраиваем автоматические уведомления
    simSerial->println("AT+CLIP=1"); // Определение номера
    delay(100);

    simSerial->println("AT+CNMI=2,1,0,0,0"); // Уведомления о SMS
    delay(100);

    simSerial->println("AT+CRISET=1"); // Включить RI сигнал
    delay(100);
}