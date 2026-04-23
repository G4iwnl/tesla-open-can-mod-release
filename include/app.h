#pragma once

#include <memory>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "handlers.h"
#include "can_monitor.h"
#include "can_live.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

#ifndef PIN_LED
#define PIN_LED 2
#endif

#if defined(RUNTIME_HW_SWITCH)
// On RUNTIME_HW_SWITCH builds (Waveshare ESP32-S3-RS485-CAN) the HW model is
// chosen at runtime from NVS (hwModeRuntime), so SelectedHandler is unused.
// All three HW handlers are compiled into the firmware and instantiated by
// createHandler() below.
#elif defined(NAG_KILLER)
using SelectedHandler = NagHandler;
#elif defined(HW4)
using SelectedHandler = HW4Handler;
#elif defined(HW3)
using SelectedHandler = HW3Handler;
#elif defined(LEGACY)
using SelectedHandler = LegacyHandler;
#else
#error "Define HW4, HW3, LEGACY, NAG_KILLER, or RUNTIME_HW_SWITCH in build_flags"
#endif

#if defined(RUNTIME_HW_SWITCH)
inline std::unique_ptr<CarManagerBase> createHandler(uint8_t hwMode)
{
    switch (hwMode)
    {
    case 0:
        return std::unique_ptr<CarManagerBase>(new LegacyHandler());
    case 1:
        return std::unique_ptr<CarManagerBase>(new HW3Handler());
    case 2:
    default:
        return std::unique_ptr<CarManagerBase>(new HW4Handler());
    }
}
#endif

static std::unique_ptr<CanDriver> appDriver;
static std::unique_ptr<CarManagerBase> appHandler;

// Precomputed bitmask for O(1) handler filter ID lookup (covers 0-1023)
static uint32_t handlerFilterMask[32] = {};

static void buildFilterMask()
{
    memset(handlerFilterMask, 0, sizeof(handlerFilterMask));
    if (!appHandler)
        return;
    const uint32_t *ids = appHandler->filterIds();
    uint8_t count = appHandler->filterIdCount();
    for (uint8_t i = 0; i < count; i++)
    {
        if (ids[i] < 1024)
            handlerFilterMask[ids[i] >> 5] |= (1u << (ids[i] & 31));
    }
}

static volatile bool frameReady = true;
static bool vehicleOtaPaused = false;
static void canISR() { frameReady = true; }

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
#include "web/web_server.h"
#endif

template <typename Driver>
static void appSetup(std::unique_ptr<Driver> drv, const char *readyMsg)
{
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000)
    {
    }

#if defined(RUNTIME_HW_SWITCH) && defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    // Load all persisted runtime settings (hwMode + features + wifi) from NVS
    // BEFORE we create the handler, so the right HW class is instantiated and
    // feature flags reflect the user's last choice from the very first frame.
    nvsLoadAllSettings();
    {
        char bootBuf[LogRingBuffer::kMaxMsgLen];
        snprintf(bootBuf, sizeof(bootBuf), "[BOOT] Tesla CAN Mod fw started, NVS loaded");
        logRing.push(bootBuf, millis());
        Serial.println(bootBuf);
    }
    appHandler = createHandler((uint8_t)hwModeRuntime);
    buildFilterMask();
    {
        const char *hwName = (uint8_t)hwModeRuntime == 0 ? "LEGACY" : (uint8_t)hwModeRuntime == 1 ? "HW3"
                                                                                                  : "HW4";
        char hwBuf[LogRingBuffer::kMaxMsgLen];
        snprintf(hwBuf, sizeof(hwBuf), "[BOOT] Handler instantiated: %s (hwMode=%d)",
                 hwName, (int)(uint8_t)hwModeRuntime);
        logRing.push(hwBuf, millis());
        Serial.println(hwBuf);
    }
#elif defined(RUNTIME_HW_SWITCH)
    // Native test path: just use HW4 by default
    appHandler = createHandler(2);
    buildFilterMask();
#else
    appHandler = std::make_unique<SelectedHandler>();
    buildFilterMask();
#endif

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    appDriver = std::move(drv);
    if (!appDriver->init())
    {
        Serial.println("CAN init failed");
#if defined(RUNTIME_HW_SWITCH) && defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
        logRing.push("[BOOT] CAN driver init FAILED", millis());
#endif
    }

    // Use ACCEPT_ALL so canLive sees every CAN ID on the bus.
    // Handler frames are software-filtered in appLoop().
    appDriver->setAcceptAll();
    canLive.setEnabled(true);

    if constexpr (Driver::kSupportsISR)
    {
        appDriver->enableInterrupt(canISR);
    }

#if defined(BOARD_HAS_PSRAM) && !defined(NATIVE_BUILD)
    if (canMonitor.init())
    {
        Serial.printf("CAN Monitor: PSRAM buffer ready, %u entries\n",
                       (unsigned)canMonitor.capacity());
        logRing.push("[BOOT] CAN Monitor PSRAM buffer ready", millis());
    }
    else
    {
        Serial.println("CAN Monitor: PSRAM init failed");
        logRing.push("[BOOT] CAN Monitor PSRAM init failed", millis());
    }
#endif

    Serial.println(readyMsg);
#if defined(RUNTIME_HW_SWITCH) && defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    {
        char rdyBuf[LogRingBuffer::kMaxMsgLen];
        snprintf(rdyBuf, sizeof(rdyBuf), "[BOOT] %s, %d filter IDs",
                 readyMsg, (int)appHandler->filterIdCount());
        logRing.push(rdyBuf, millis());
    }
#endif

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
    // Delay WiFi init to let CAN stabilize
    delay(2000);
    webServerInit();
#endif
}

template <typename Driver>
static void appLoop()
{
    if constexpr (Driver::kSupportsISR)
    {
        if (!frameReady)
            return;
        frameReady = false;
    }

    CanFrame frame;
    while (appDriver->read(frame))
    {
        digitalWrite(PIN_LED, LOW);

        // Feed live signal table (always active, no filter needed)
        canLive.update(frame, millis());

        // Feed monitor (passive, no modification)
#if defined(BOARD_HAS_PSRAM) && !defined(NATIVE_BUILD)
        canMonitor.record(frame, millis());
#endif

        // Feed handler (only for IDs it cares about) — O(1) bitmask check
        // OTA protection: skip handler when vehicle OTA is in progress
        if (!vehicleOtaPaused &&
            frame.id < 1024 &&
            (handlerFilterMask[frame.id >> 5] & (1u << (frame.id & 31))))
        {
            appHandler->frameCount++;
#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
            uint32_t t0 = (uint32_t)esp_timer_get_time();
#endif
            appHandler->handleMessage(frame, *appDriver);
#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)
            uint32_t dt = (uint32_t)esp_timer_get_time() - t0;
            handlerLatencyUs = dt;
            if (dt > handlerLatencyMax) handlerLatencyMax = dt;
            handlerCallCount++;
#endif
        }
    }
    digitalWrite(PIN_LED, HIGH);

#if defined(RUNTIME_HW_SWITCH) && !defined(NATIVE_BUILD)
    // ─── Shared decoded signals for OTA check + smart offset ───
    DecodedSignals _loopSig;
    decodeSignals(canLive, _loopSig);

    // ─── OTA protection: pause CAN mods when vehicle OTA is detected ───
    {
        bool otaNow = (_loopSig.otaInProgress > 0);
        if (otaNow != vehicleOtaPaused)
        {
            vehicleOtaPaused = otaNow;
            char obuf[LogRingBuffer::kMaxMsgLen];
            snprintf(obuf, sizeof(obuf), "[OTA] Vehicle OTA %s — CAN mods %s",
                     otaNow ? "detected" : "ended",
                     otaNow ? "PAUSED" : "RESUMED");
            logRing.push(obuf, millis());
            Serial.println(obuf);
        }
    }

    // ─── Smart offset ─────────────────────────────────────────────────────
    // Smart offset is now computed inline inside HW3/HW4 handlers via
    // computeSpeedOffset() using lastFusedSpeedLimit (decoded from frame 921
    // in the same handler). The previous implementation here hijacked
    // speedOffsetManualMode=true as a side channel, which left manual mode
    // "stuck on" after the user disabled smart offset. Removed in v1.8.2.

    // ─── Preheat / Precondition trigger ───
    // Periodically inject UI_tripPlanning (0x082) frames to make the BMS
    // think a navigation trip is active and start battery preconditioning.
    // 500ms cadence matches flipper-tesla-fsd PRECOND_INTERVAL_MS.
    //
    // We bypass appDriver->send() and call twai_transmit() directly with
    // TWAI_MSG_FLAG_SS (Single Shot) so the frame is NOT retransmitted on
    // ACK failure. Without single-shot, the ESP32-S3 TWAI controller would
    // retry a failed frame thousands of times in 500ms, exploding the TX
    // error counter from 0 to 128 (ERROR_PASSIVE) on a single inject when
    // the bus has no other ACK source.
    // Preheat also paused during vehicle OTA to avoid bus interference.
    if (!vehicleOtaPaused && preheatRuntime && appDriver)
    {
        static unsigned long lastPreheatMs = 0;
        unsigned long now = millis();
        if (now - lastPreheatMs >= 500)
        {
            lastPreheatMs = now;

            // --- Auto-stop: duration limit ---
            if (preheatStartMs > 0 && preheatMaxDurationMs > 0 &&
                (now - preheatStartMs) >= preheatMaxDurationMs)
            {
                preheatRuntime = false;
                char ebuf[LogRingBuffer::kMaxMsgLen];
                snprintf(ebuf, sizeof(ebuf),
                         "[PREHEAT] Auto-stopped: max duration %lu min reached",
                         preheatMaxDurationMs / 60000UL);
                logRing.push(ebuf, millis());
                Serial.println(ebuf);
                return;
            }

            // --- Auto-stop: battery temp threshold ---
            {
                if (_loopSig.bmsTempMin > -40 && _loopSig.bmsTempMin >= preheatAutoStopTemp)
                {
                    preheatRuntime = false;
                    char ebuf[LogRingBuffer::kMaxMsgLen];
                    snprintf(ebuf, sizeof(ebuf),
                             "[PREHEAT] Auto-stopped: battery temp %d°C >= target %d°C",
                             (int)_loopSig.bmsTempMin, (int)preheatAutoStopTemp);
                    logRing.push(ebuf, millis());
                    Serial.println(ebuf);
                    return;
                }
            }

            // Bus-health guard: auto-disable if TX errors climb past the
            // ERROR_PASSIVE threshold. With single-shot mode this should
            // grow at +8 per failed frame, so the user gets ~16 cycles of
            // grace before we auto-disable.
            twai_status_info_t st;
            if (twai_get_status_info(&st) == ESP_OK && st.tx_error_counter > 96)
            {
                preheatRuntime = false;
                char ebuf[LogRingBuffer::kMaxMsgLen];
                snprintf(ebuf, sizeof(ebuf),
                         "[PREHEAT] Auto-disabled: TX errors=%d (no CAN ACK, check wiring)",
                         (int)st.tx_error_counter);
                logRing.push(ebuf, millis());
                Serial.println(ebuf);
                return;
            }

            twai_message_t msg = {};
            msg.identifier = 0x082;       // UI_tripPlanning
            msg.data_length_code = 8;
            msg.flags = TWAI_MSG_FLAG_SS; // Single-shot: no retry on ACK fail
            msg.data[0] = 0x05;           // bit0 tripPlanningActive + bit2 requestActiveBatteryHeating
            // Other 7 bytes already zero from {} init
            twai_transmit(&msg, pdMS_TO_TICKS(2));
            if (appHandler)
                appHandler->framesSent++;
        }
    }
#endif
}
