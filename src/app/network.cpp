#include "app.h"

NetworkApp::NetworkApp(NetworkManager* netManager) : network(netManager) {}

String NetworkApp::getAppName() {
    return "Сеть";
}

void NetworkApp::drawUI() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(5, 5);
    
    M5Cardputer.Display.println("Управление сетью");
    M5Cardputer.Display.println("================");
    
    if (network) {
        M5Cardputer.Display.println("Тип: " + network->getNetworkName());
        M5Cardputer.Display.println("Сигнал: " + String(network->getSignalStrength()) + " dBm");
        M5Cardputer.Display.println("Статус: " + String(network->isNetworkAvailable() ? "Подключено" : "Нет связи"));
    }
    
    M5Cardputer.Display.println("\nУправление:");
    M5Cardputer.Display.println("A - Переподключиться");
    M5Cardputer.Display.println("B - Сканировать WiFi");
    M5Cardputer.Display.println("ESC - Выход");
}

void NetworkApp::handleInput() {
    if (!M5Cardputer.Keyboard.isPressed()) return;
    
    if (M5Cardputer.Keyboard.isKeyPressed('a') && network) {
        network->reconnect();
        needRedraw = true;
        delay(300);
    } else if (M5Cardputer.Keyboard.isKeyPressed('b') && network) {
        // Здесь можно добавить сканирование WiFi
        needRedraw = true;
        delay(300);
    }
}

void NetworkApp::update() {
    if (needRedraw) {
        drawUI();
        needRedraw = false;
    }
    handleInput();
}

void NetworkApp::start() {
    needRedraw = true;
}

void NetworkApp::exit() {
    // Ничего не делаем
}