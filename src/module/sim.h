#ifndef SIM_MODULE_H
#define SIM_MODULE_H

#include <HardwareSerial.h>

#define SIM_RX 2
#define SIM_TX 1

class SimModule {
private:
    HardwareSerial* simSerial;
    bool moduleDetected = false;
    bool registered = false;
    bool isSleeping = false;
    unsigned long lastCheck = 0;
    unsigned long commandCounter = 0;  // Счетчик команд для отладки
    
    String readResponse(unsigned long timeout, bool debug = false);
    void setupModule();
    bool checkSMS();
    
public:
    SimModule(HardwareSerial* serial = &Serial2);
    
    bool begin();
    bool isDetected() const { return moduleDetected; }
    bool isRegistered() const { return registered; }
    bool isModuleSleeping() const { return isSleeping; }
    
    bool update();
    bool sleep();      // Перевести модуль в режим энергосбережения
    bool wakeup();     // Разбудить модуль
    
    // МЕТОДЫ для отладки и надежной отправки команд
    String sendCommand(const String& cmd, unsigned long timeout = 2000, bool waitForOK = true);
    void clearBuffer();  // Очистка буфера UART
    void printDebugInfo(); // Отладочная информация
    
    // Вспомогательные методы
    bool checkWakeupByCall();
};

#endif