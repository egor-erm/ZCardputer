#include "app/app.h"

const int MAX_APPS = 4;

int currentAppIndex = 0;

App *activeApp;
App *apps[MAX_APPS];

void drawMainMenu() {
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
  } else if (M5Cardputer.Keyboard.isKeyPressed('.')) { // вниз
    currentAppIndex = (currentAppIndex == MAX_APPS - 1) ? 0 : currentAppIndex+1;
  }

  drawMainMenu();
  delay(200);
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
    
  apps[0] = new WifiApp();
  apps[1] = new MusicApp();
  apps[2] = new TamagotchiApp();
  apps[3] = new SettingsApp();

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(GREEN);

  drawMainMenu();
}

void loop() {
  M5Cardputer.update();

  if (activeApp != NULL) {

  } else handleInput();
}