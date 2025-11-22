#include "app.h"

// Используем официальный API микрофона — I2S больше не нужен
static constexpr size_t MIC_BUFFER_SIZE = 512; // размер буфера для одного вызова record()
static int16_t micReadBuffer[MIC_BUFFER_SIZE];

String RecorderApp::getAppName() {
    return "Recorder";
}

void RecorderApp::writeString(File &file, const char *str) {
    file.write((const uint8_t*)str, strlen(str));
}

void RecorderApp::createWavHeader(File &file, uint32_t dataSize) {
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t numChannels = 1;
    uint32_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    uint32_t blockAlign = numChannels * bitsPerSample / 8;
    uint32_t chunkSize = 36 + dataSize;
    uint32_t subChunk2Size = dataSize;

    writeString(file, "RIFF");
    file.write((uint8_t*)&chunkSize, 4);
    writeString(file, "WAVE");

    writeString(file, "fmt ");
    uint32_t subChunk1Size = 16;
    file.write((uint8_t*)&subChunk1Size, 4);
    uint16_t audioFormat = 1;
    file.write((uint8_t*)&audioFormat, 2);
    file.write((uint8_t*)&numChannels, 2);
    file.write((uint8_t*)&sampleRate, 4);
    file.write((uint8_t*)&byteRate, 4);
    file.write((uint8_t*)&blockAlign, 2);
    file.write((uint8_t*)&bitsPerSample, 2);

    writeString(file, "data");
    file.write((uint8_t*)&subChunk2Size, 4);
}

String RecorderApp::getNextFilename() {
    for (int i = 0; i <= 999; i++) {
        String filename = String(FILENAME_PREFIX) + 
                        (i > 0 ? String(i) : "") + FILENAME_EXT;
        if (!SD.exists(filename)) {
            return filename;
        }
    }
    return String(FILENAME_PREFIX) + "999" + FILENAME_EXT;
}

void RecorderApp::flushBuffer() {
    if (bufferIndex > 0 && audioFile) {
        for (int i = 0; i < bufferIndex; i++) {
            audioBuffer[i] *= amplificationFactor;
            if (audioBuffer[i] > INT16_MAX) audioBuffer[i] = INT16_MAX;
            if (audioBuffer[i] < INT16_MIN) audioBuffer[i] = INT16_MIN;
        }
        audioFile.write((uint8_t*)audioBuffer, bufferIndex * sizeof(int16_t));
        data_size += bufferIndex * sizeof(int16_t);
        bufferIndex = 0;
    }
}

void RecorderApp::playWavFromSD(const char* filename) {
    File file = SD.open(filename);
    if (!file) return;

    struct __attribute__((packed)) wav_header_t {
        char RIFF[4];
        uint32_t chunk_size;
        char WAVEfmt[8];
        uint32_t fmt_chunk_size;
        uint16_t audiofmt;
        uint16_t channel;
        uint32_t sample_rate;
        uint32_t byte_per_sec;
        uint16_t block_size;
        uint16_t bit_per_sample;
    };

    struct __attribute__((packed)) sub_chunk_t {
        char identifier[4];
        uint32_t chunk_size;
        uint8_t data[1];
    };

    wav_header_t wav_header;
    file.read((uint8_t*)&wav_header, sizeof(wav_header));

    if (memcmp(wav_header.RIFF, "RIFF", 4) ||
        memcmp(wav_header.WAVEfmt, "WAVEfmt ", 8) ||
        wav_header.audiofmt != 1 ||
        wav_header.bit_per_sample < 8 ||
        wav_header.bit_per_sample > 16 ||
        wav_header.channel == 0 ||
        wav_header.channel > 2) {
        file.close();
        return;
    }

    file.seek(offsetof(wav_header_t, audiofmt) + wav_header.fmt_chunk_size);
    sub_chunk_t sub_chunk;
    file.read((uint8_t*)&sub_chunk, 8);

    while (memcmp(sub_chunk.identifier, "data", 4)) {
        if (!file.seek(sub_chunk.chunk_size, SeekCur)) break;
        file.read((uint8_t*)&sub_chunk, 8);
    }

    if (memcmp(sub_chunk.identifier, "data", 4)) {
        file.close();
        return;
    }

    // Отключаем микрофон и включаем динамик
    if (M5Cardputer.Mic.isEnabled()) {
        M5Cardputer.Mic.end();
    }
    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(255);

    drawUI(); // Обновляем интерфейс: "PLAYING"

    uint8_t wav_data[BUFFER_SIZE];
    int32_t data_len = sub_chunk.chunk_size;
    bool flg_16bit = (wav_header.bit_per_sample == 16);

    // Стримим данные без ожидания
    while (data_len > 0 && isPlaying) {
        // Проверяем ввод (остановка по 'B')
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isKeyPressed('b')) {
            isPlaying = false;
            break;
        }

        size_t len = (data_len < (int32_t)BUFFER_SIZE) ? data_len : BUFFER_SIZE;
        len = file.read(wav_data, len);
        if (len == 0) break;
        data_len -= len;

        if (flg_16bit) {
            M5Cardputer.Speaker.playRaw((const int16_t*)wav_data, len / 2, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
        } else {
            M5Cardputer.Speaker.playRaw((const uint8_t*)wav_data, len, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
        }

        // Небольшая задержка, чтобы не перегружать очередь динамика
        delay(1);
    }

    file.close();
    stopPlayback(); // Это вернёт микрофон, если нужно
}

void RecorderApp::startRecording() {
    if (isRecording || isPlaying) return;

    // Отключаем динамик, включаем микрофон
    if (M5Cardputer.Speaker.isEnabled()) {
        M5Cardputer.Speaker.end();
    }
    M5Cardputer.Mic.begin();

    currentFilename = getNextFilename();
    audioFile = SD.open(currentFilename, FILE_WRITE);

    if (audioFile) {
        createWavHeader(audioFile, 0);
        isRecording = true;
        recordStartTime = millis();
        bufferIndex = 0;
        data_size = 0;
        needRedraw = true;
    } else {
        // Если не удалось открыть файл — отключаем микрофон
        M5Cardputer.Mic.end();
    }
}

void RecorderApp::stopRecording() {
    if (!isRecording) return;

    isRecording = false;
    flushBuffer();

    // Обновляем заголовок
    audioFile.seek(4);
    uint32_t chunkSize = 36 + data_size;
    audioFile.write((uint8_t*)&chunkSize, 4);
    audioFile.seek(40);
    audioFile.write((uint8_t*)&data_size, 4);
    audioFile.close();

    // Отключаем микрофон
    M5Cardputer.Mic.end();
    needRedraw = true;
}

void RecorderApp::playRecording() {
    if (isRecording || isPlaying || currentFilename == "") return;
    isPlaying = true;
    playWavFromSD(currentFilename.c_str());
    needRedraw = true;
}

void RecorderApp::stopPlayback() {
    if (!isPlaying) return;
    isPlaying = false;
    M5Cardputer.Speaker.stop();
    
    if (!isRecording) {
        M5Cardputer.Mic.begin();
    }
    needRedraw = true;
}

void RecorderApp::update() {
    handleInput();

    unsigned long currentTime = millis() / 1000;
    if ((isRecording || isPlaying) && currentTime - lastUpdateTime >= 1) {
        needRedraw = true;
        lastUpdateTime = currentTime;
    }

    if (isRecording) {
        // Читаем аудиоданные через официальный API
        if (M5Cardputer.Mic.record(micReadBuffer, MIC_BUFFER_SIZE, SAMPLE_RATE)) {
            for (size_t i = 0; i < MIC_BUFFER_SIZE; i++) {
                if (bufferIndex >= BUFFER_SIZE) {
                    flushBuffer();
                }
                audioBuffer[bufferIndex++] = micReadBuffer[i];
            }

            
            if (millis() - recordStartTime > RECORD_TIME * 1000) {
                stopRecording();
            }
        }
    }

    if (needRedraw) {
        drawUI();
        needRedraw = false;
    }
}

void RecorderApp::drawUI() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(5, 5);

    if (isRecording) {
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.println("RECORDING");
        M5Cardputer.Display.printf("Time: %ds\n", (millis() - recordStartTime) / 1000);
        M5Cardputer.Display.printf("File: %s\n", currentFilename.c_str());
        M5Cardputer.Display.printf("Size: %d KB\n", data_size / 1024);
    } else if (isPlaying) {
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.println("PLAYING");
        M5Cardputer.Display.printf("File: %s\n", currentFilename.c_str());
    } else {
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.println("DICTAPHONE");
        M5Cardputer.Display.println("A - Record/Stop");
        M5Cardputer.Display.println("B - Play/Stop");
        M5Cardputer.Display.println("Exit - ESC");

        if (currentFilename != "") {
            M5Cardputer.Display.printf("Last: %s\n", currentFilename.c_str());
        }
    }
}

void RecorderApp::handleInput() {
    if (!M5Cardputer.Keyboard.isPressed()) return;

    if (M5Cardputer.Keyboard.isKeyPressed('a') && !isPlaying) {
        if (isRecording) {
            stopRecording();
        } else {
            startRecording();
        }
        delay(300);
    } else if (M5Cardputer.Keyboard.isKeyPressed('b') && !isRecording && currentFilename != "") {
        if (isPlaying) {
            stopPlayback();
        } else {
            playRecording();
        }
        delay(300);
    }
}

void RecorderApp::start() {
    needRedraw = true;
    lastUpdateTime = 0;
    
    if (!M5Cardputer.Mic.isEnabled()) {
        M5Cardputer.Mic.begin();
    }
}

void RecorderApp::exit() {
    if (isRecording) stopRecording();
    if (isPlaying) stopPlayback();
}

RecorderApp::RecorderApp() {
    
}

RecorderApp::~RecorderApp() {
    
}