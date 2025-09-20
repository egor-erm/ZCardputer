#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>

// AUDIO SETTINGS
#define SAMPLE_RATE 44100  // Используем ту же частоту, что и в рабочем примере
#define RECORD_TIME 60
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

namespace lgfx {
namespace v1 {

template<>
struct DataWrapperT<fs::SDFS> : public DataWrapper {
    fs::SDFS* _fs;
    fs::File _file;
    
    DataWrapperT(fs::SDFS* fs) : _fs(fs), _file() {}
    
    virtual ~DataWrapperT() { close(); }
    
    virtual int read(uint8_t* buf, uint32_t len) override {
        if (!_file) return 0;
        return _file.read(buf, len);
    }
    
    virtual void skip(int32_t offset) override {
        if (!_file) return;
        _file.seek(_file.position() + offset);
    }
    
    virtual bool seek(uint32_t offset) override {
        if (!_file) return false;
        return _file.seek(offset);
    }
    
    virtual void close() override {
        if (_file) {
            _file.close();
        }
    }
    
    virtual int32_t tell() override {
        if (!_file) return -1;
        return _file.position();
    }
    
    bool open(const char* path) {
        close();
        _file = _fs->open(path, "r");
        return _file;
    }
};

} // namespace v1
} // namespace lgfx