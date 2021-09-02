#include <SPI.h>
#include <MFRC522.h>

#define USE_BLE
// #define USE_SERIAL

#if defined(USE_BLE)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "RFDIPlayingCard.h"
#endif

#include <cstring>
#include <cstdint>

namespace
{
    char const * const VERSION = "0.1";

    // ESP8266 LoLin
    // auto const PIN_MFRC522_SS  = byte {2};
    // auto const PIN_MFRC522_RST = byte {0};
    // ESP32 WROOM
    auto const PIN_MFRC522_SS  = byte {5};
    auto const PIN_MFRC522_RST = byte {17};

    auto const DATA_PAGE = byte {4};

    auto const EMPTY = std::uint32_t {0xff};

    auto mfrc522 = MFRC522 {PIN_MFRC522_SS, PIN_MFRC522_RST};

    auto card = EMPTY;

#if defined(USE_BLE)
    char const * const BLE_NAME = "ESP RFID CARD";

    struct
    {
        BLEServer * server;
        BLEService * service;
        BLEAdvertising * advertising;

        struct
        {
            BLECharacteristic * card;
        } characteristics;

        BLE2902 descriptor;

        struct : public BLEServerCallbacks
        {
            virtual void onDisconnect(BLEServer * server)
            {
                BLEDevice::startAdvertising();
            }
        } server_cb;
    } ble = {};
#endif
}

void setup()
{
#if defined(USE_SERIAL)
    Serial.begin(115200); while (!Serial);
    Serial.print("rfid-playing-card "); Serial.println(VERSION);
#endif
    SPI.begin();

    mfrc522.PCD_Init();

#if defined(USE_BLE)
    BLEDevice::init(BLE_NAME);

    ble.server = BLEDevice::createServer();
    ble.server->setCallbacks(&ble.server_cb);

    ble.service = ble.server->createService(RFDIPlayingCard::UUID_SERVICE);

    ble.characteristics.card = ble.service->createCharacteristic
    (
        RFDIPlayingCard::UUID_CARD,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    {
        auto v = EMPTY;
        ble.characteristics.card->setValue(v);
    }
    ble.characteristics.card->addDescriptor(&ble.descriptor);

    ble.service->start();

    ble.advertising = BLEDevice::getAdvertising();
    ble.advertising->addServiceUUID(RFDIPlayingCard::UUID_SERVICE);
    ble.advertising->setScanResponse(true);

    BLEDevice::startAdvertising();
#endif
}

// An active scan seems necessary for tag removal detection.
// If tag removal isn't required, irq should be a better choice.
void loop()
{
    byte atqa_buffer[2];
    byte atqa_size = sizeof (atqa_buffer);
    std::uint32_t c = EMPTY;
    if ((mfrc522.PICC_WakeupA(atqa_buffer, &atqa_size) == MFRC522::STATUS_OK) && mfrc522.PICC_ReadCardSerial())
    {
        if (MFRC522::PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_TYPE_MIFARE_UL)
        {
            byte read_buffer[18];
            byte read_size = sizeof (read_buffer);
            if (mfrc522.MIFARE_Read(DATA_PAGE, read_buffer, &read_size) == MFRC522::STATUS_OK)
            {
                c = std::uint32_t {read_buffer[0]};
            }
        }
        mfrc522.PICC_HaltA();
    }
    if (c != card)
    {
#if defined(USE_SERIAL)
        Serial.println(c);
#endif
#if defined(USE_BLE)
        ble.characteristics.card->setValue(c);
        ble.characteristics.card->notify();
#endif
        card = c;
    }

    delay(250);
}
