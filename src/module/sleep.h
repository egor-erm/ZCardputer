#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <M5Cardputer.h>

// Предварительное объявление (чтобы не включать весь sim.h)
class SimModule;

class SleepManager {
private:
    SimModule* simModule;  // Указатель на SIM модуль
    unsigned long lastActivity = 0;
    unsigned long sleepTimeout = 30000; // 30 секунд
    bool isSleeping = false;
    
    void goToSleep();
    void wakeUp();
    void updateInSleep();
    void updateWhileAwake();
    
public:
    // Конструктор с передачей SIM модуля
    SleepManager(SimModule* sim = nullptr) : simModule(sim) {}
    
    void updateActivity() {
        lastActivity = millis();
        if (isSleeping) {
            wakeUp();
        }
    }
    
    void update();
    bool isDeviceSleeping() const { return isSleeping; }
    void setSleepTimeout(unsigned long timeout) {
        sleepTimeout = timeout;
    }
    unsigned long getLastActivity() const { return lastActivity; }
};

#endif