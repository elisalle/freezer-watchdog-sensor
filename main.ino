/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara
    then adapted for the Freezer Watchdog university project by Eli Sallé
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t oldPinState = 1;
uint8_t pinState;
const int read_pin = 23;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

const std::string service_uuid = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const std::string characteristic_uuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8";


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
    Serial.begin(115200);

    // Initialise the reed switch pin as an input
    pinMode(read_pin, INPUT);

    // Create the BLE Device
    BLEDevice::init("Freezer Watchdog Sensor");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(service_uuid);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        characteristic_uuid,
                        BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                        );

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(service_uuid);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // there's no point in checking more often
    delay(200);
    // notify changed value
    if (deviceConnected) {
        // read pin value
        pinState = digitalRead(read_pin);
        // if it changes
        if (pinState != oldPinState)
        {
            oldPinState = pinState;
            Serial.print("new pin state: ");
            Serial.println(pinState);
            pCharacteristic->setValue((uint8_t*)&pinState, 4);
            pCharacteristic->notify();
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("disconnecting");
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        Serial.println("connecting");
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}