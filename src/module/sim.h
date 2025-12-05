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
    
    String readResponse(unsigned long timeout);
    void setupModule();
    
public:
    SimModule(HardwareSerial* serial = &Serial2);
    
    bool begin();
    bool isDetected() const { return moduleDetected; }
    bool isRegistered() const { return registered; }
    bool isModuleSleeping() const { return isSleeping; }
    
    void update();
    bool sleep(); // Перевести модуль в режим энергосбережения
    bool wakeup(); // Разбудить модуль
    bool checkForCallOrSMS(); // Быстрая проверка событий (даже в режиме сна)
    
    bool checkWakeupByCall();
};

#endif