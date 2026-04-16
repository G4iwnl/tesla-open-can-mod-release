#pragma once

#include <cstdint>
#include <cstring>
#include "can_frame_types.h"
#include "shared_types.h"

#ifndef NATIVE_BUILD
#include <esp_heap_caps.h>
#endif

// ── CAN Monitor — PSRAM ring buffer for FSD-related frame logging ───────────
//
// Records all FSD-relevant CAN frames into a PSRAM-backed ring buffer.
// Purely passive: never transmits, never modifies frames.
// Toggled at runtime via Web API.

struct __attribute__((packed)) CanLogEntry
{
    uint32_t timestamp_ms;
    uint16_t can_id;
    uint8_t dlc;
    uint8_t data[8];
    // Total: 15 bytes per entry
};

class CanMonitor
{
public:
    // 21 FSD-related CAN IDs to capture
    static constexpr uint16_t kWatchIds[] = {
        0x045, //  69  STW_ACTN_RQ (Legacy stalk)
        0x082, // 130  UI_tripPlanning (precondition)
        0x0D5, // 213  UI_cruiseControl (speedLimitTick)
        0x118, // 280  DI_systemStatus
        0x132, // 306  BMS_hvBusStatus
        0x238, // 568  UI_driverAssistMapData (mapSpeedLimit)
        0x286, // 646  DI_locStatus (cruiseSetSpeed)
        0x2B9, // 697  DAS_control (setSpeed)
        0x312, // 786  BMS_thermalStatus
        0x313, // 787  UI_trackModeSettings
        0x318, // 792  GTW_carState (OTA detect)
        0x334, // 820  UI_powertrainControl (speedLimit)
        0x370, // 880  EPAS3P_sysStatus (nag)
        0x389, // 905  DAS_status2 (accSpeedLimit)
        0x292, // 658  BMS_socStatus
        0x398, // 920  GTW_carConfig (HW detect)
        0x399, // 921  DAS_status (ISA + fusedSpeedLimit)
        0x3D9, // 985  UI_gpsVehicleSpeed (userSpeedOffset)
        0x3EE, // 1006 DAS_autopilot (Legacy)
        0x3F8, // 1016 UI_driverAssistControl (followDist)
        0x3FD, // 1021 UI_autopilotControl (FSD main)
    };
    static constexpr size_t kWatchIdCount = sizeof(kWatchIds) / sizeof(kWatchIds[0]);

    // Target: ~6MB of PSRAM for the ring buffer (~400K entries, ~50 min at 140 fps)
    static constexpr size_t kTargetBytes = 6 * 1024 * 1024;
    static constexpr size_t kEntrySize = sizeof(CanLogEntry);

    bool init()
    {
        // Build O(1) bitmask from watch list (covers IDs 0-1023)
        memset(watchMask_, 0, sizeof(watchMask_));
        for (size_t i = 0; i < kWatchIdCount; i++)
        {
            uint16_t id = kWatchIds[i];
            if (id < 1024)
                watchMask_[id >> 5] |= (1u << (id & 31));
        }
#ifndef NATIVE_BUILD
        size_t avail = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
        size_t want = kTargetBytes;
        if (want > avail)
            want = (avail > 64 * 1024) ? (avail - 32 * 1024) : 0;
        if (want < kEntrySize * 1024)
            return false; // need at least 1K entries

        buffer_ = static_cast<CanLogEntry *>(
            heap_caps_malloc(want, MALLOC_CAP_SPIRAM));
        if (!buffer_)
            return false;

        capacity_ = want / kEntrySize;
        memset(buffer_, 0, capacity_ * kEntrySize);
        head_ = 0;
        count_ = 0;
        inited_ = true;
        return true;
#else
        return false;
#endif
    }

    bool isWatchedId(uint32_t id) const
    {
        if (id < 1024)
            return (watchMask_[id >> 5] & (1u << (id & 31))) != 0;
        return false;
    }

    void record(const CanFrame &frame, uint32_t timestamp_ms)
    {
        if (!enabled_ || !inited_ || !buffer_)
            return;
        if (!isWatchedId(frame.id))
            return;

        CanLogEntry &e = buffer_[head_ % capacity_];
        e.timestamp_ms = timestamp_ms;
        e.can_id = static_cast<uint16_t>(frame.id);
        e.dlc = frame.dlc;
        memcpy(e.data, frame.data, 8);

        head_++;
        if (count_ < capacity_)
            count_++;
    }

    void clear()
    {
        head_ = 0;
        count_ = 0;
    }

    void setEnabled(bool en) { enabled_ = en; }
    bool isEnabled() const { return enabled_; }
    bool isInited() const { return inited_; }
    uint32_t entryCount() const { return count_; }
    uint32_t capacity() const { return capacity_; }
    uint32_t head() const { return head_; }

    // Read a contiguous range of entries for export.
    // Returns number of entries copied.
    // startIdx is absolute ring index; caller should start from
    // (head_ - count_) to get the oldest available entry.
    uint32_t readEntries(CanLogEntry *out, uint32_t maxOut, uint32_t startIdx) const
    {
        if (!inited_ || !buffer_ || count_ == 0)
            return 0;

        uint32_t oldest = (head_ > count_) ? (head_ - count_) : 0;
        if (startIdx < oldest)
            startIdx = oldest;

        uint32_t n = 0;
        for (uint32_t i = startIdx; i < head_ && n < maxOut; i++, n++)
        {
            out[n] = buffer_[i % capacity_];
        }
        return n;
    }

    // Direct access to underlying buffer for chunked HTTP streaming
    const CanLogEntry *entryAt(uint32_t absIdx) const
    {
        if (!inited_ || !buffer_)
            return nullptr;
        return &buffer_[absIdx % capacity_];
    }

    uint32_t oldestIdx() const
    {
        return (head_ > count_) ? (head_ - count_) : 0;
    }

private:
    CanLogEntry *buffer_ = nullptr;
    uint32_t capacity_ = 0;
    Shared<uint32_t> head_{0};
    Shared<uint32_t> count_{0};
    bool inited_ = false;
    Shared<bool> enabled_{false};
    uint32_t watchMask_[32] = {};  // bitmask for O(1) ID lookup (IDs 0-1023)
};

inline CanMonitor canMonitor;
