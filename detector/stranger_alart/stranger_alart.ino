#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Configuration structure for easy management of settings
struct Config {
    static const int IR_PIN = 13;      // GPIO for IR sensor input
    static const int LIGHT_PIN = 12;   // GPIO for controlling the LED
    static const int BUZZER_PIN = 14;  // GPIO for controlling the buzzer
    static const char* DEVICE_NAME;
};

const char* Config::DEVICE_NAME = "ESP32_Security";

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Callback class for handling BLE server events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        pServer->getAdvertising()->start(); // Restart advertising to allow reconnection
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(Config::IR_PIN, INPUT);
    pinMode(Config::LIGHT_PIN, OUTPUT);
    pinMode(Config::BUZZER_PIN, OUTPUT);

    // Initialize BLE device with a name
    BLEDevice::init(Config::DEVICE_NAME);

    // Create a BLE server
    BLEServer *pServer = BLEDevice::createServer();

    // Set the callback functions for server events
    pServer->setCallbacks(new MyServerCallbacks());

    // Create a BLE service
    BLEService *pService = pServer->createService(BLEUUID((uint16_t)0xFFE0));

    // Create a BLE characteristic for the service
    pCharacteristic = pService->createCharacteristic(
        BLEUUID((uint16_t)0xFFE1),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    // Add a descriptor to the characteristic
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising the service to make it discoverable
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client connection to notify...");
}

void loop() {
    // Check if the device connection state has changed
    if (deviceConnected != oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Read the IR sensor value
    if (digitalRead(Config::IR_PIN) == LOW) { // Assuming the sensor outputs LOW when an object is detected
        Serial.println("Object detected!");

        // Turn on the buzzer for 5 seconds
        digitalWrite(Config::BUZZER_PIN, HIGH);

        // Blink the LED for 6 seconds at 1-second intervals
        for (int i = 0; i < 6; i++) {
            digitalWrite(Config::LIGHT_PIN, HIGH);
            delay(500);
            digitalWrite(Config::LIGHT_PIN, LOW);
            delay(500);
        }

        // If a BLE device is connected, send a notification
        if (deviceConnected) {
            pCharacteristic->setValue("Object detected!");
            pCharacteristic->notify();
            Serial.println("Notification sent");
        }

        // Turn off the buzzer after 5 seconds
        digitalWrite(Config::BUZZER_PIN, LOW);
    }

    // Small delay to reduce CPU usage and debounce
    delay(100);
}
