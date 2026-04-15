#pragma once

#include "can_frame_types.h"
#include "shared_types.h"

inline constexpr uint8_t kTrackModeRequestOn = 0x01;

// ── Runtime feature flags (RUNTIME_HW_SWITCH path) ─────────────────────────
//
// On the Waveshare ESP32-S3-RS485-CAN env we compile every feature in
// unconditionally; the user toggles them at runtime via the web console and
// they are persisted in NVS. Build-time #defines (BYPASS_TLSSC_REQUIREMENT
// etc.) are still honoured so other ESP32 builds keep their
// "compile-time opt-in" behaviour.
//
// build_enabled = true  → feature shows up as a switch (always true on
//                          RUNTIME_HW_SWITCH builds; on legacy builds it
//                          mirrors whether the macro is defined)
// default_enabled       → first-boot value when NVS has nothing yet

#if defined(RUNTIME_HW_SWITCH) || defined(BYPASS_TLSSC_REQUIREMENT)
inline constexpr bool kBypassTlsscRequirementBuildEnabled = true;
#else
inline constexpr bool kBypassTlsscRequirementBuildEnabled = false;
#endif
#if defined(BYPASS_TLSSC_REQUIREMENT)
inline constexpr bool kBypassTlsscRequirementDefaultEnabled = true;
#else
inline constexpr bool kBypassTlsscRequirementDefaultEnabled = false;
#endif

#if defined(RUNTIME_HW_SWITCH) || defined(ISA_SPEED_CHIME_SUPPRESS)
inline constexpr bool kIsaSpeedChimeSuppressBuildEnabled = true;
#else
inline constexpr bool kIsaSpeedChimeSuppressBuildEnabled = false;
#endif
#if defined(ISA_SPEED_CHIME_SUPPRESS)
inline constexpr bool kIsaSpeedChimeSuppressDefaultEnabled = true;
#else
inline constexpr bool kIsaSpeedChimeSuppressDefaultEnabled = false;
#endif

#if defined(RUNTIME_HW_SWITCH) || defined(EMERGENCY_VEHICLE_DETECTION)
inline constexpr bool kEmergencyVehicleDetectionBuildEnabled = true;
#else
inline constexpr bool kEmergencyVehicleDetectionBuildEnabled = false;
#endif
#if defined(EMERGENCY_VEHICLE_DETECTION)
inline constexpr bool kEmergencyVehicleDetectionDefaultEnabled = true;
#else
inline constexpr bool kEmergencyVehicleDetectionDefaultEnabled = false;
#endif

#if defined(RUNTIME_HW_SWITCH) || defined(ENHANCED_AUTOPILOT)
inline constexpr bool kEnhancedAutopilotBuildEnabled = true;
#else
inline constexpr bool kEnhancedAutopilotBuildEnabled = false;
#endif
inline constexpr bool kEnhancedAutopilotDefaultEnabled = false;

// NAG_KILLER stays compile-time only — see handlers.h NagHandler for why
// (it requires a different CAN bus tap than the LEGACY/HW3/HW4 handlers).
#if defined(NAG_KILLER)
inline constexpr bool kNagKillerDefaultEnabled = true;
inline constexpr bool kNagKillerBuildEnabled = true;
#else
inline constexpr bool kNagKillerDefaultEnabled = false;
inline constexpr bool kNagKillerBuildEnabled = false;
#endif

// China Mode: skip the "Traffic Light & Stop Sign Control" UI flag check
// in isFSDSelectedInUI(). Needed for some China-region firmware builds where
// the option is not exposed in the UI.
inline constexpr bool kChinaModeBuildEnabled = true;
inline constexpr bool kChinaModeDefaultEnabled = false;

// Profile mode auto: when true the speed profile follows the follow-distance
// stalk; when false the profile is whatever the user picked in the web UI.
inline constexpr bool kProfileModeAutoDefaultEnabled = true;

inline Shared<bool> bypassTlsscRequirementRuntime{kBypassTlsscRequirementDefaultEnabled};
inline Shared<bool> isaSpeedChimeSuppressRuntime{kIsaSpeedChimeSuppressDefaultEnabled};
inline Shared<bool> emergencyVehicleDetectionRuntime{kEmergencyVehicleDetectionDefaultEnabled};
inline Shared<bool> enhancedAutopilotRuntime{kEnhancedAutopilotDefaultEnabled};
inline Shared<bool> nagKillerRuntime{kNagKillerDefaultEnabled};
inline Shared<bool> chinaModeRuntime{kChinaModeDefaultEnabled};
inline Shared<bool> profileModeAutoRuntime{kProfileModeAutoDefaultEnabled};

// hwModeRuntime: 0=LEGACY, 1=HW3, 2=HW4. Only meaningful on RUNTIME_HW_SWITCH
// builds; legacy builds use the compile-time SelectedHandler typedef in app.h.
inline Shared<uint8_t> hwModeRuntime{2};

// Preheat / Precondition: when ON, the main loop periodically injects
// UI_tripPlanning (0x082) frames every 500ms with byte[0]=0x05
// (bit0 tripPlanningActive + bit2 requestActiveBatteryHeating).
// This makes the BMS think a navigation route is active and starts
// battery / cabin preconditioning, even without a Supercharger destination.
// Source: flipper-tesla-fsd fsd_build_precondition_frame()
inline constexpr bool kPreheatDefaultEnabled = false;
inline Shared<bool> preheatRuntime{kPreheatDefaultEnabled};
inline volatile unsigned long preheatStartMs = 0;       // millis() when preheat was enabled
inline volatile int8_t preheatAutoStopTemp = 10;         // °C — auto-stop when bmsTempMin >= this
inline volatile unsigned long preheatMaxDurationMs = 30UL * 60 * 1000; // 30 min default

// Speed offset manual override (HW3 only).
// When true, handler uses manualSpeedOffset instead of the CAN-read value.
inline Shared<bool> speedOffsetManualMode{false};
inline Shared<int> manualSpeedOffset{0}; // CAN value (pct * 4)

// Smart offset: auto-adjust offset based on fusedSpeedLimit.
// Rules: if limit < threshold[i], use offsetPct[i].
inline Shared<bool> smartOffsetEnabled{false};

struct SmartOffsetRule {
    int maxSpeed;   // kph threshold (exclusive upper bound)
    int offsetPct;  // offset percentage (0-50)
};

inline constexpr int kMaxSmartRules = 8;

struct SmartOffsetRules {
    SmartOffsetRule rules[kMaxSmartRules] = {
        {41, 50},   // ≤ 40 kph → 50%
        {51, 30},   // 50 kph → 30%
        {61, 20},   // 60 kph → 20%
        {999, 10},  // 80-120 kph → 10%
    };
    int count = 4;

    int lookup(int speedLimit) const {
        for (int i = 0; i < count; i++) {
            if (speedLimit < rules[i].maxSpeed)
                return rules[i].offsetPct;
        }
        return 0;
    }
};

inline SmartOffsetRules smartOffsetRules;

inline uint8_t readMuxID(const CanFrame &frame)
{
    return frame.data[0] & 0x07;
}

inline bool isFSDSelectedInUI(const CanFrame &frame)
{
    // China Mode: bypass the UI selection check entirely. Some China-region
    // firmware does not expose "Traffic Light & Stop Sign Control" in the UI,
    // so the bit is never set even when the user wants FSD enabled.
    if (chinaModeRuntime)
        return true;
    if (bypassTlsscRequirementRuntime)
        return true;
    return (frame.data[4] >> 6) & 0x01;
}

inline void setSpeedProfileV12V13(CanFrame &frame, int profile)
{
    frame.data[6] &= ~0x06;
    frame.data[6] |= (profile << 1);
}

inline void setTrackModeRequest(CanFrame &frame, uint8_t request)
{
    frame.data[0] &= static_cast<uint8_t>(~0x03);
    frame.data[0] |= static_cast<uint8_t>(request & 0x03);
}

inline uint8_t computeTeslaChecksum(const CanFrame &frame, uint8_t checksumByteIndex = 7)
{
    if (checksumByteIndex >= frame.dlc)
        return 0;

    uint16_t sum = static_cast<uint16_t>(frame.id & 0xFF) +
                   static_cast<uint16_t>((frame.id >> 8) & 0xFF);
    for (uint8_t i = 0; i < frame.dlc; ++i)
    {
        if (i == checksumByteIndex)
            continue;
        sum += frame.data[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

inline void setBit(CanFrame &frame, int bit, bool value)
{
    if (bit < 0 || bit >= 64)
        return; // bounds guard: CanFrame.data is 8 bytes
    int byteIndex = bit / 8;
    int bitIndex = bit % 8;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if (value)
    {
        frame.data[byteIndex] |= mask;
    }
    else
    {
        frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}
