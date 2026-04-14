/*
    PlatformIO entry point.
    ESP32 / ESP32-S3 TWAI (CAN) only.
    Logic is in the shared headers under include/.
*/

#include <Arduino.h>
#include "app.h"
#include "drivers/twai_driver.h"

#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN GPIO_NUM_5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN GPIO_NUM_4
#endif

void setup()
{
    appSetup<TWAIDriver>(std::make_unique<TWAIDriver>(TWAI_TX_PIN, TWAI_RX_PIN), "ESP32 TWAI ready @ 500k");
}

void loop()
{
    appLoop<TWAIDriver>();
}
