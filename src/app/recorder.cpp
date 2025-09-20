#include "app.h"

// Конфигурация I2S (как в рабочем примере)
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
};

i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PIN_NO_CHANGE,
    .ws_io_num = 43,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 46,
};

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

    // RIFF header
    writeString(file, "RIFF");
    file.write((uint8_t*)&chunkSize, 4);
    writeString(file, "WAVE");

    // fmt subchunk
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

    // data subchunk
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
        // Применяем усиление к данным в буфере
        for (int i = 0; i < bufferIndex; i++) {
            audioBuffer[i] *= amplificationFactor;
            // Предотвращаем клиппинг
            if (audioBuffer[i] > INT16_MAX) audioBuffer[i] = INT16_MAX;
            if (audioBuffer[i] < INT16_MIN) audioBuffer[i] = INT16_MIN;
        }
            
        audioFile.write((uint8_t*)audioBuffer, bufferIndex * sizeof(int16_t));
        data_size += bufferIndex * sizeof(int16_t);
        bufferIndex = 0;
    }
}

void RecorderApp::playWavFromSD(const char* filename) {

}

RecorderApp::RecorderApp() {
    
}

RecorderApp::~RecorderApp() {
    
}

String RecorderApp::getAppName() {
    return "Recorder";
}

void RecorderApp::startRecording() {
    if (isRecording || isPlaying) return;
    
    // Инициализируем I2S
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("I2S driver install failed");
        return;
    }

    if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
        Serial.println("I2S set pin failed");
        return;
    }
        
    currentFilename = getNextFilename();
    audioFile = SD.open(currentFilename, FILE_WRITE);
        
    if (audioFile) {
        // Записываем заглушку для WAV заголовка
        createWavHeader(audioFile, 0);
        isRecording = true;
        recordStartTime = millis();
        bufferIndex = 0;
        data_size = 0;
        needRedraw = true;
    }
}

void RecorderApp::stopRecording() {
    if (!isRecording) return;
    
    isRecording = false;
    flushBuffer(); // Сбрасываем оставшиеся данные
    
    // Обновляем WAV заголовок с правильным размером данных
    audioFile.seek(4);
    uint32_t chunkSize = 36 + data_size;
    audioFile.write((uint8_t*)&chunkSize, 4);
    audioFile.seek(40);
    audioFile.write((uint8_t*)&data_size, 4);
    
    audioFile.close();
    
    // Останавливаем I2S
    i2s_driver_uninstall(I2S_NUM_0);
    
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
    needRedraw = true;
}

void RecorderApp::update() {
    handleInput();

    unsigned long currentTime = millis() / 1000;
    
    // Обновляем интерфейс раз в секунду для отображения времени
    if ((isRecording || isPlaying) && currentTime - lastUpdateTime >= 1) {
        needRedraw = true;
        lastUpdateTime = currentTime;
    }

    if (isRecording) {
        // Читаем данные с I2S
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, (void*)&audioBuffer[bufferIndex], 
                (BUFFER_SIZE - bufferIndex) * sizeof(int16_t), 
                &bytes_read, 0);
        
        if (bytes_read > 0) {
            bufferIndex += bytes_read / sizeof(int16_t);
                
            // Если буфер заполнен, сбрасываем его в файл
            if (bufferIndex >= BUFFER_SIZE) {
                flushBuffer();
            }
        }
            
        // Автостоп по времени
        if (millis() - recordStartTime > RECORD_TIME * 1000) {
            stopRecording();
        }
    }

    // Проверяем статус воспроизведения
    if (isPlaying && !M5Cardputer.Speaker.isPlaying()) {
        stopPlayback();
    }

    // Перерисовываем интерфейс если нужно
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
        M5Cardputer.Display.printf("Time: %ds\n", (millis() - playStartTime) / 1000);
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

    if (M5Cardputer.Keyboard.isKeyPressed('a')) {
        if (isRecording) {
            stopRecording();
        } else if (isPlaying) {
            stopPlayback();
        } else {
            startRecording();
        }
    } else if (M5Cardputer.Keyboard.isKeyPressed('b')) {
        if (!isRecording && currentFilename != "") {
            if (isPlaying) {
                stopPlayback();
            } else {
                playRecording();
            }
        }
    }

    delay(500);
}

void RecorderApp::start() {
    needRedraw = true;
    lastUpdateTime = 0;
}

void RecorderApp::exit() {
    if (isRecording) stopRecording();
    if (isPlaying) stopPlayback();
}