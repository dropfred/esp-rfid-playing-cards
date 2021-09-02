#ifndef RFID_PLAYING_CARD_H
#define RFID_PLAYING_CARD_H

#include <BLEDevice.h>

namespace RFDIPlayingCard
{
    BLEUUID const UUID_SERVICE("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    BLEUUID const UUID_CARD   ("beb5483e-36e1-4688-b7f5-ea07361b26a8");
    BLEUUID const UUID_BATTERY("4ea60680-747d-44b6-9978-0b044da1d28e");
}

#endif
