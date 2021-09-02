#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Org_01.h>
#include <BLEDevice.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "RFDIPlayingCard.h"
#include "Battery.h"

namespace
{
    char const * const VERSION = "0.1";

    auto card = std::string {};

    char const * const SUITS[] = {"CLUB", "HEART", "SPADE", "DIAMOND"};
    char const * const INDICES[] = {"JOCKER", "ACE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "HEIGHT", "NINE", "TEN", "JACK", "QUEEN", "KING"};

    void set_card(std::uint8_t c)
    {
        auto s = (c >> 4) & 3;
        auto v = c & 0x0f;

        if ((v < 14) && ((v == 0) || (s < 4)))
        {
            card = INDICES[v];
            if (v != 0)
            {
                card += " of ";
                card += SUITS[s];
            }
        }
        else
        {
            card.clear();
        }
    }

    auto SCREEN_WIDTH  = std::uint8_t {128};
    auto SCREEN_HEIGHT = std::uint8_t {64};

    // ESP NodeMcu LoLin
    // auto PIN_SSD1306_SDA = std::uint8_t {4};
    // auto PIN_SSD1306_SCL = std::uint8_t {5};
    // ESP32-CAM
    auto const PIN_SSD1306_SDA = std::uint8_t {14};
    auto const PIN_SSD1306_SCL = std::uint8_t {15};

    Adafruit_SSD1306 ssd1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

    GFXfont const * const FONT_STATUS = &Org_01;
    GFXfont const * const FONT_MAIN   = nullptr;

    void cb_card(BLERemoteCharacteristic * characteristic, std::uint8_t const * data, std::size_t size, bool notify)
    {
        set_card(data[0]);
    }

    struct : public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks
    {
        virtual void onConnect(BLEClient * client)
        {
        }

        virtual void onDisconnect(BLEClient * client)
        {
            // Serial.println(F("onDisconnect"));
            do_scan = true;
        }

        virtual void onResult(BLEAdvertisedDevice advertisedDevice)
        {
            // Serial.println(advertisedDevice.toString().c_str());
            if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(RFDIPlayingCard::UUID_SERVICE))
            {
                // Serial.println(F("found"));
                BLEDevice::getScan()->stop();
                device = new BLEAdvertisedDevice(advertisedDevice);
                do_scan = false;
                do_connect = true;
            }
        }

        bool connect()
        {
            // Serial.println(F("connect"));

            client = BLEDevice::createClient();
            client->setClientCallbacks(this);

            if (client->connect(device))
            {
                BLERemoteService * service = client->getService(RFDIPlayingCard::UUID_SERVICE);
                if (service != nullptr)
                {
                    card = service->getCharacteristic(RFDIPlayingCard::UUID_CARD);
                }
                if (card == nullptr)
                {
                    client->disconnect();
                }
            }

            if (client->isConnected())
            {
                if (card->canRead())
                {
                    set_card(card->readUInt8());
                }

                if (card->canNotify())
                {
                    card->registerForNotify(cb_card);
                }

                connected = true;
            }
            else
            {
                delete client;
                client = nullptr;
            }

            // Serial.println(connected ? F("connection success") : F("connection failure"));
            return connected;
        }

        BLEAdvertisedDevice * device;
        BLEClient * client;
        BLERemoteCharacteristic * card;

        bool connected;
        bool do_connect;
        bool do_scan;
    } ble = {};

    void cb_scan_complete(BLEScanResults r)
    {
        // Serial.println(F("cb_scan_complete"));
        ble.do_scan = !ble.connected;
    }

    void error()
    {
        while (true);
    }
}

void setup()
{
    // Serial.begin(115200); while (!Serial);
    // Serial.println(F("\nBLE client"));

    Wire.begin(PIN_SSD1306_SDA, PIN_SSD1306_SCL);

    if (!ssd1306.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        error();
    }
    ssd1306.clearDisplay();
    ssd1306.display();

    BLEDevice::init("");
    BLEScan * scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(&ble);
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);

    ble.do_scan = true;
}

void loop()
{
    ssd1306.clearDisplay();

    ssd1306.setTextColor(WHITE);

    // ssd1306.setFont(&Org_01);
    ssd1306.setFont(FONT_STATUS);
    ssd1306.setTextSize(1, 1);
    ssd1306.setCursor(0, 6);
    ssd1306.print(ble.connected ? F("CONNECTED") : F("SCAN"));
    if (!ble.connected)
    {
        for (auto i = ((millis() / 1000) % 3) + 1; i > 0; --i) ssd1306.print('.');
    }

    // TODO
    BM_BATTERY::draw(ssd1306, ssd1306.width() - BM_BATTERY::width, 0, 100);

    if (ble.connected)
    {
        // TODO
        BM_BATTERY::draw(ssd1306, ssd1306.width() - BM_BATTERY::width, 16, 100);

        // ssd1306.setFont(nullptr);
        ssd1306.setFont(FONT_MAIN);
        ssd1306.setTextSize(1, 2);
        auto const * txt = card.c_str();
        // center text
        {
            std::int16_t x, y; std::uint16_t w, h;
            ssd1306.getTextBounds(txt, 0, 0, &x, &y, &w, &h);
            ssd1306.setCursor((SCREEN_WIDTH - w) / 2, 36);
        }
        ssd1306.print(txt);
    }

    ssd1306.display();

    if (ble.do_scan)
    {
        ble.do_scan = false;
        delete ble.device;
        delete ble.client;
        ble.device = nullptr;
        ble.client = nullptr;
        ble.connected = false;
        auto * scan = BLEDevice::getScan();
        scan->clearResults();
        scan->start(10, &cb_scan_complete, true);
        // Serial.println(F("scan"));
    }
    else if (ble.do_connect)
    {
        ble.do_connect = false;
        ble.connect();
    }

    delay(250);
}
