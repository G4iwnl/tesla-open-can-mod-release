#pragma once

#include <cstdint>
#include <cstring>
#include "can_frame_types.h"
#include "shared_types.h"

// ── CAN Live Signal Table ───────────────────────────────────────────────────
//
// Maintains the latest decoded value of every CAN ID seen on the bus.
// Updated on every incoming frame. Queried via /api/can-live.
// Works with ACCEPT_ALL filter — sees everything on X179.

struct CanIdEntry
{
    uint32_t id = 0;
    uint8_t data[8] = {};
    uint8_t dlc = 0;
    uint32_t last_ms = 0;   // last seen timestamp
    uint32_t count = 0;     // total frames seen
    bool seen = false;
};

class CanLive
{
public:
    // Standard CAN = 11-bit ID = 0-2047, Tesla uses mostly < 0x500
    static constexpr size_t kMaxIds = 128;

    void update(const CanFrame &frame, uint32_t now_ms)
    {
        if (!enabled_)
            return;

        // Find existing slot or allocate new one
        for (size_t i = 0; i < slotCount_; i++)
        {
            if (slots_[i].id == frame.id)
            {
                slots_[i].dlc = frame.dlc;
                memcpy(slots_[i].data, frame.data, 8);
                slots_[i].last_ms = now_ms;
                slots_[i].count++;
                return;
            }
        }
        // New ID
        if (slotCount_ < kMaxIds)
        {
            auto &s = slots_[slotCount_++];
            s.id = frame.id;
            s.dlc = frame.dlc;
            memcpy(s.data, frame.data, 8);
            s.last_ms = now_ms;
            s.count = 1;
            s.seen = true;
        }
    }

    void setEnabled(bool en) { enabled_ = en; }
    bool isEnabled() const { return enabled_; }
    size_t slotCount() const { return slotCount_; }

    // Sort slots by ID for consistent output
    void sortById()
    {
        for (size_t i = 1; i < slotCount_; i++)
        {
            CanIdEntry tmp = slots_[i];
            size_t j = i;
            while (j > 0 && slots_[j - 1].id > tmp.id)
            {
                slots_[j] = slots_[j - 1];
                j--;
            }
            slots_[j] = tmp;
        }
    }

    const CanIdEntry &slot(size_t idx) const { return slots_[idx]; }

    void clear()
    {
        slotCount_ = 0;
        memset(slots_, 0, sizeof(slots_));
    }

private:
    CanIdEntry slots_[kMaxIds] = {};
    volatile size_t slotCount_ = 0;
    Shared<bool> enabled_{false};
};

// ── Known signal decoders (inline, called from web handler) ─────────────────

struct DecodedSignals
{
    // 0x118 DI_systemStatus
    uint8_t di_gear = 0;         // 0=INV,1=P,2=R,3=N,4=D,5=CREEP
    float di_accelPedal = 0;     // 0-100%

    // 0x145 ESP_status
    float espBrakeTorque = 0;    // Nm

    // 0x185 lighting
    uint8_t lightState = 0;      // 0x00=off,0x02=low,0x03=flash,0x06/07=high
    uint8_t lightRaw_b3 = 0;
    uint8_t lightRaw_b4 = 0;

    // 0x238 UI_driverAssistMapData
    int mapSpeedLimit = 0;

    // 0x257 DI_speed (vehicle speed + UI speed)
    float vehicleSpeed = 0;      // kph
    float uiSpeed = 0;           // kph or mph (display speed)
    bool uiSpeedUnitsMph = false;

    // 0x261 motor torque
    float motorTorque = 0;       // Nm (signed)

    // 0x2B9 DAS_control
    float dasSetSpeed = 0;
    uint8_t dasAccState = 0;

    // 0x318 GTW_carState
    uint8_t otaInProgress = 0;

    // 0x334 UI_powertrainControl
    int vehicleSpeedLimit = 0;

    // 0x370 EPAS3P_sysStatus
    uint8_t handsOnLevel = 0;    // 0=none,1=light,2=medium,3=firm
    float torsionBarTorque = 0;  // Nm (steering effort)
    uint8_t epasCounter = 0;

    // 0x389 DAS_status2
    float accSpeedLimit = 0;

    // 0x399 DAS_status
    int fusedSpeedLimit = 0;
    int visionSpeedLimit = 0;
    bool suppressSpeedWarning = false;

    // 0x3D9 UI_gpsVehicleSpeed
    float gpsSpeed = 0;
    int userSpeedOffset = 0;
    bool userSpeedOffsetUnitsKph = false;
    int mppSpeedLimit = 0;

    // 0x3F8 UI_driverAssistControl
    uint8_t followDistance = 0;

    // 0x3FD UI_autopilotControl
    uint8_t apc_mux = 0;
    bool fsdSelectedInUI = false;
    bool fsdEnabled = false;        // bit 46
    bool fsdHw4Lock = false;        // bit 60
    uint8_t speedProfileHw3 = 0;
    uint8_t speedProfileHw4 = 0;
    int smartSetSpeedOffsetRaw = 0;
    uint8_t speedOffsetInjected = 0;
    bool eceR79Nag = false;         // bit 19
};

inline void decodeSignals(const CanLive &live, DecodedSignals &out)
{
    for (size_t i = 0; i < live.slotCount(); i++)
    {
        const auto &s = live.slot(i);
        const uint8_t *d = s.data;

        switch (s.id)
        {
        case 0x118: // DI_systemStatus
            out.di_gear = (d[2] >> 5) & 0x07;  // bit 21|3 = byte2 bit5-7
            out.di_accelPedal = (d[4]) * 0.4f;
            break;

        case 0x145: // ESP_status — encoding unverified on X179
            // Disabled: no confirmed byte position for brake torque
            break;

        case 0x185: // Lighting state
            out.lightState = d[2];
            out.lightRaw_b3 = d[3];
            out.lightRaw_b4 = d[4];
            break;

        case 0x238: { // UI_driverAssistMapData
            uint8_t raw = d[1] & 0x1F;
            static const int mapTbl[] = {
                0,5,7,10,15,20,25,30,35,40,45,50,55,60,65,70,
                75,80,85,90,95,100,105,110,115,120,130,140,150,160,-1,-2};
            out.mapSpeedLimit = (raw < 32) ? mapTbl[raw] : 0;
            break;
        }

        case 0x257: { // DI_speed — vehicle speed + UI display speed
            // DI_vehicleSpeed: bit 12, 12 bits, factor 0.08, offset -40
            uint16_t rawVs = ((uint16_t)d[2] << 4) | ((d[1] >> 4) & 0x0F);
            out.vehicleSpeed = rawVs * 0.08f - 40.0f;
            if (out.vehicleSpeed < 0) out.vehicleSpeed = 0;
            // DI_uiSpeed: bit 24, 9 bits LE
            out.uiSpeed = (float)(((d[4] & 0x01) << 8) | d[3]);
            // DI_uiSpeedUnits: bit 33 = byte 4 bit 1
            out.uiSpeedUnitsMph = (d[4] >> 1) & 0x01;
            break;
        }

        case 0x261: // Motor torque — encoding unverified on X179
            // Disabled: no confirmed byte position
            break;

        case 0x2B9: // DAS_control
            out.dasSetSpeed = (((d[1] & 0x0F) << 8) | d[0]) * 0.1f;
            out.dasAccState = (d[1] >> 4) & 0x0F;
            break;

        case 0x318: // GTW_carState
            out.otaInProgress = d[6] & 0x03;
            break;

        case 0x334: // UI_powertrainControl
            out.vehicleSpeedLimit = (d[2] == 255) ? -1 : (d[2] + 50);
            break;

        case 0x370: { // EPAS3P_sysStatus
            out.handsOnLevel = (d[4] >> 6) & 0x03;
            // torsionBarTorque: bytes 2-3, 12-bit signed, factor ~0.01 Nm
            uint16_t rawTq = ((uint16_t)(d[3] & 0x0F) << 8) | d[2];
            // Sign extend 12-bit
            if (rawTq & 0x800) rawTq |= 0xF000;
            out.torsionBarTorque = (int16_t)rawTq * 0.01f;
            out.epasCounter = d[6] & 0x0F;
            break;
        }

        case 0x389: // DAS_status2
            out.accSpeedLimit = (((d[1] & 0x03) << 8) | d[0]) * 0.2f;
            break;

        case 0x399: // DAS_status
            out.fusedSpeedLimit = (d[1] & 0x1F) * 5;
            out.suppressSpeedWarning = (d[1] >> 5) & 0x01;
            out.visionSpeedLimit = (d[2] & 0x1F) * 5;
            break;

        case 0x3D9: // UI_gpsVehicleSpeed
            out.gpsSpeed = (((d[4] & 0xFF) << 8) | (d[3] & 0xFF)) * 0.00390625f;
            out.userSpeedOffset = (d[5] & 0x3F) - 30;
            out.userSpeedOffsetUnitsKph = (d[5] >> 7) & 0x01;
            out.mppSpeedLimit = (d[6] & 0x1F) * 5;
            break;

        case 0x3F8: // UI_driverAssistControl
            out.followDistance = (d[5] & 0xE0) >> 5;
            break;

        case 0x3FD: { // UI_autopilotControl (multiplexed)
            uint8_t mux = d[0] & 0x07;
            out.apc_mux = mux;
            if (mux == 0) {
                out.fsdSelectedInUI = (d[4] >> 6) & 0x01;
                out.fsdEnabled = (d[5] >> 6) & 0x01;
                out.fsdHw4Lock = (d[7] >> 4) & 0x01;
                out.smartSetSpeedOffsetRaw = (d[3] >> 1) & 0x3F;
                out.speedProfileHw3 = (d[6] >> 1) & 0x03;
            }
            if (mux == 1) {
                out.eceR79Nag = (d[2] >> 3) & 0x01;
            }
            if (mux == 2) {
                out.speedOffsetInjected = ((d[0] >> 6) & 0x03) | ((d[1] & 0x3F) << 2);
                out.speedProfileHw4 = (d[7] >> 4) & 0x07;
            }
            break;
        }
        default:
            break;
        }
    }
}

inline CanLive canLive;
