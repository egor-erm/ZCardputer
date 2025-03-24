#include <M5Cardputer.h>

class App {
public:
    virtual ~App() {} // Виртуальный деструктор (важно!)
    virtual String getAppName() = 0; // Чисто виртуальная функция
};

class WifiApp : public App {
public:
    WifiApp();
    String getAppName();
};

class MusicApp : public App {
public:
    MusicApp();
    String getAppName();
};

class SettingsApp : public App {
public:
    SettingsApp();
    String getAppName();
};

class TamagotchiApp : public App {
public:
    TamagotchiApp();
    String getAppName();
};