#include "app/app.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

#define MAX_APPS 5

int currentAppIndex = 0;

App *activeApp = NULL;
App *apps[MAX_APPS];

void drawMainMenu() {
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(GREEN);
  
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(5, 5);

  for (int i = 0; i < MAX_APPS; i++) {
    M5Cardputer.Display.println((i == currentAppIndex ? "> " : " ") + apps[i]->getAppName());
  }
}

void handleInput() {
  if (!M5Cardputer.Keyboard.isPressed()) return;

  if (M5Cardputer.Keyboard.isKeyPressed(';')) { // вверх
    currentAppIndex = (currentAppIndex == 0) ? MAX_APPS - 1 : currentAppIndex-1;
    drawMainMenu();
  } else if (M5Cardputer.Keyboard.isKeyPressed('.')) { // вниз
    currentAppIndex = (currentAppIndex == MAX_APPS - 1) ? 0 : currentAppIndex+1;
    drawMainMenu();
  } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // выбрать
    activeApp = apps[currentAppIndex];
    activeApp->start();
  }

  delay(200);
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);

  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    M5Cardputer.Display.println("SD card failed!");
    while (1)
      ;
  }

  if (SD.exists("/boot.png")) {
    if (M5Cardputer.Display.drawPngFile(SD, "/boot.png")) {
      delay(3000);
    }
  }
    
  apps[0] = new RecorderApp();
  apps[1] = new WifiApp();
  apps[2] = new MusicApp();
  apps[3] = new TamagotchiApp();
  apps[4] = new SettingsApp();

  drawMainMenu();
}

void loop() {
  M5Cardputer.update();

  if (activeApp != NULL) {
    activeApp->update();
        
    if (M5Cardputer.Keyboard.isKeyPressed('`')) {
      activeApp->exit();
      activeApp = NULL;
      drawMainMenu();
    }
  } else handleInput();
}