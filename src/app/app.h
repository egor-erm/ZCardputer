#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>

// AUDIO SETTINGS
#define SAMPLE_RATE 44100  // Используем ту же частоту, что и в рабочем примере
#define RECORD_TIME 30
#define BUFFER_SIZE 1024   // Увеличим буфер для лучшей производительности
#define FILENAME_PREFIX "/note"
#define FILENAME_EXT ".wav"

class App {
public:
    virtual ~App() {} // Виртуальный деструктор (важно!)
    virtual String getAppName() = 0;
    virtual void update() {}
    virtual void drawUI() {}
    virtual void handleInput() {}
    virtual void start() {}
    virtual void exit() {}
};

class RecorderApp : public App {
private:
    bool isRecording = false;
    bool isPlaying = false;
    bool needRedraw = true;
    File audioFile;
    String currentFilename;
    unsigned long recordStartTime = 0;
    unsigned long playStartTime = 0;
    unsigned long lastUpdateTime = 0;
    
    int16_t audioBuffer[BUFFER_SIZE];
    size_t bufferIndex = 0;
    uint32_t data_size = 0;
    const int amplificationFactor = 3; // Усиление звука
    
    void writeString(File &file, const char *str);
    void createWavHeader(File &file, uint32_t dataSize);
    String getNextFilename();
    void flushBuffer();
    void playWavFromSD(const char* filename);

public:
    RecorderApp();
    ~RecorderApp();
    String getAppName();
    void startRecording();
    void stopRecording();
    void playRecording();
    void stopPlayback();
    void update();
    void drawUI();
    void handleInput();
    void start();
    void exit();
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