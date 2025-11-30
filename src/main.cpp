#include "app/app.h"
#include "font/rus.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

#define MAX_APPS 5
const lgfx::U8g2font FontRUS1 = { lgfx_font_rus_5x7 };
const lgfx::U8g2font FontRUS2 = { lgfx_font_rus_6x12 };

int currentAppIndex = 0;

App *activeApp = NULL;
App *apps[MAX_APPS];

void drawBatteryBarMinimal() {
  int batteryLevel = M5Cardputer.Power.getBatteryLevel();
  
  int barWidth = 1;
  int barHeight = M5Cardputer.Display.height(); // На всю высоту экрана
  int x = M5Cardputer.Display.width() - 1; // Самый правый пиксель
  
  // Заполненная часть
  int filledHeight = (barHeight * batteryLevel) / 100;
  if (filledHeight > 0) {
    uint16_t color;
    if (batteryLevel > 70) color = GREEN;
    else if (batteryLevel > 30) color = YELLOW;
    else color = RED;
    
    int fillY = barHeight - filledHeight; // Снизу вверх
    M5Cardputer.Display.drawFastVLine(x, fillY, filledHeight, color);
  }
}

void drawMainMenu() {
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(GREEN);
  
  M5Cardputer.Display.clear();
  
  int centerY = M5Cardputer.Display.height() / 2;
  int lineHeight = 20; // Высота строки
  int visibleItems = 5; // Количество видимых элементов
  
  int startIndex = max(0, min(currentAppIndex - visibleItems / 2, MAX_APPS - visibleItems));
  
  for (int i = 0; i < visibleItems; i++) {
    int appIndex = startIndex + i;
    if (appIndex >= 0 && appIndex < MAX_APPS) {
      int yPos = centerY - (lineHeight / 2) + (i - (currentAppIndex - startIndex)) * lineHeight;
      
      M5Cardputer.Display.setCursor(5, yPos);
      
      if (appIndex == currentAppIndex) {
        M5Cardputer.Display.print("> ");
      } else {
        M5Cardputer.Display.print("  ");
      }
      
      M5Cardputer.Display.println(apps[appIndex]->getAppName());
    }
  }

  drawBatteryBarMinimal();
}

void handleInput() {
  if (!M5Cardputer.Keyboard.isPressed()) return;

  if (M5Cardputer.Keyboard.isKeyPressed(';')) { // вверх
    currentAppIndex = (currentAppIndex == 0) ? MAX_APPS - 1 : currentAppIndex - 1;
    drawMainMenu();
  } else if (M5Cardputer.Keyboard.isKeyPressed('.')) { // вниз
    currentAppIndex = (currentAppIndex == MAX_APPS - 1) ? 0 : currentAppIndex + 1;
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

  M5Cardputer.Display.setFont(&FontRUS2);

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