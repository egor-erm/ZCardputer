#include "sleep.h"
#include "sim.h"
#include <SPI.h>
#include <SD.h>

// Пины SD карты
#define SD_SPI_SCK_PIN 40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN 12

void SleepManager::update() {
    if (isSleeping) {
        // В режиме сна
        updateInSleep();
    } else {
        // В режиме бодрствования
        updateWhileAwake();
    }
}

void SleepManager::updateWhileAwake() {
    // Проверяем таймаут бездействия
    if (millis() - lastActivity > sleepTimeout) {
        goToSleep();
    }
}

void SleepManager::updateInSleep() {
    // 1. Проверяем SIM модуль на входящие звонки
    if (simModule != nullptr && simModule->isDetected()) {
        // Эта проверка ДОЛЖНА работать даже если SIM модуль спит
        if (simModule->checkForCallOrSMS()) {
            // Обнаружен входящий звонок или SMS - ПРОСЫПАЕМСЯ!
            wakeUp();
            return;
        }
    }
    
    // 2. Проверяем нажатие клавиш для пробуждения
    if (M5Cardputer.Keyboard.isPressed()) {
        wakeUp();
        return;
    }
    
    // 3. В режиме сна делаем небольшую задержку
    delay(100);
}

void SleepManager::goToSleep() {
    Serial.println("Going to deep sleep...");
    isSleeping = true;
    
    // 1. Выключаем дисплей
    M5Cardputer.Display.sleep();
    
    // 2. Отключаем SD карту
    SPI.end();
    
    // 3. ПЕРЕВОДИМ SIM МОДУЛЬ В РЕЖИМ СНА
    if (simModule != nullptr && simModule->isDetected()) {
        simModule->sleep();  // Переводим SIM в режим энергосбережения
    }
    
    // 4. Снижаем частоту процессора
    setCpuFrequencyMhz(10);
    
    Serial.println("System sleeping. Press any key or incoming call to wake up.");
}

void SleepManager::wakeUp() {
    Serial.println("Waking up system...");
    isSleeping = false;
    
    // 1. Восстанавливаем частоту процессора
    setCpuFrequencyMhz(240);
    
    // 2. БУДИМ SIM МОДУЛЬ
    if (simModule != nullptr && simModule->isDetected()) {
        simModule->wakeup();  // Будим SIM модуль
    }
    
    // 3. Включаем SD карту
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    SD.begin(SD_SPI_CS_PIN, SPI, 25000000);
    
    // 4. Включаем дисплей
    M5Cardputer.Display.wakeup();
    M5Cardputer.Display.setBrightness(100);
    
    // 5. Восстанавливаем таймер активности
    lastActivity = millis();
    
    Serial.println("System awake and ready");
}