// --- 平滑限速切换功能 ---
#include <cmath>
struct HW3Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {787, 1016, 1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 3; }

    // 平滑限速切换相关变量
    int lastSpeedLimit = 0;
    int lastTargetSpeed = 0;
    int smoothSpeedOffset = 0;
    unsigned long smoothTimestamp = 0;
    static constexpr int kSmoothStep = 5; // 每周期最大调整5kph
    static constexpr int kSmoothIntervalMs = 500; // 500ms调整一次

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id == 787)
        {
            if (frame.dlc < 8)
                return;
            setTrackModeRequest(frame, kTrackModeRequestOn);
            frame.data[7] = computeTeslaChecksum(frame);
            framesSent++;
            driver.send(frame);
            return;
        }
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            if (!profileModeAutoRuntime)
                return; // Manual mode: don't overwrite user's profile choice
            uint8_t followDistance = (frame.data[5] & 0b11100000) >> 5;
            switch (followDistance)
            {
            case 1:
                speedProfile = 2;
                break;
            case 2:
                speedProfile = 1;
                break;
            case 3:
                speedProfile = 0;
                break;
            // --- 平滑限速切换功能 ---
            #include <cmath>
            struct HW4Handler : public CarManagerBase
            {
                const uint32_t *filterIds() const override
                {
                    static constexpr uint32_t ids[] = {921, 1016, 1021};
                    return ids;
                }
                uint8_t filterIdCount() const override { return 3; }

                // 平滑限速切换相关变量
                int lastSpeedLimit = 0;
                int lastTargetSpeed = 0;
                int smoothSpeedOffset = 0;
                unsigned long smoothTimestamp = 0;
                static constexpr int kSmoothStep = 5; // 每周期最大调整5kph
                static constexpr int kSmoothIntervalMs = 500; // 500ms调整一次

                void handleMessage(CanFrame &frame, CanDriver &driver) override
                {
                    if (frame.id == 921)
                    {
                        if (frame.dlc < 8)
                            return;
                        if (!isaSpeedChimeSuppressRuntime)
                            return;
                        frame.data[1] |= 0x20;
                        uint8_t sum = 0;
                        for (int i = 0; i < 7; i++)
                            sum += frame.data[i];
                        sum += (921 & 0xFF) + (921 >> 8);
                        frame.data[7] = sum & 0xFF;
                        framesSent++;
                        driver.send(frame);
                        return;
                    }
                    if (frame.id == 1016)
                    {
                        if (frame.dlc < 6)
                            return;
                        if (!profileModeAutoRuntime)
                            return; // Manual mode: don't overwrite user's profile choice
                        auto fd = (frame.data[5] & 0b11100000) >> 5;
                        switch (fd)
                        {
                        case 1:
                        case 2:
                        case 3:
                            speedProfile = 3; // Max
                            break;
                        case 4:
                            speedProfile = 2; // Hurry
                            break;
                        case 5:
                            speedProfile = 1; // Normal
                            break;
                        case 6:
                            speedProfile = 0; // Chill
                            break;
                        case 7:
                            speedProfile = 4; // Sloth
                            break;
                        }
                        return;
                    }
                    if (frame.id == 1021)
                    {
                        if (frame.dlc < 8)
                            return;
                        auto index = readMuxID(frame);
                        if (index == 0)
                            FSDEnabled = isFSDSelectedInUI(frame);
                        if (index == 0 && FSDEnabled)
                        {
                            int speedLimit = 0;
                            // 假设 frame.data[2] 存储限速（如有专用信号请替换此处）
                            speedLimit = frame.data[2];
                            int raw = (frame.data[3] >> 1) & 0x3F;
                            int offset = std::max(std::min((raw - 30) * 5, 100), 0);
                            int targetSpeed = speedLimit + (speedLimit * offset) / 100;
                            unsigned long now = 0;
            #ifndef NATIVE_BUILD
                            now = millis();
            #endif
                            // 检测限速切换
                            if (lastSpeedLimit != 0 && speedLimit != lastSpeedLimit)
                            {
                                // 记录切换前目标速度
                                lastTargetSpeed = lastSpeedLimit + (lastSpeedLimit * smoothSpeedOffset) / 100;
                                // 计算新偏移使目标速度平滑
                                int desiredOffset = 0;
                                if (speedLimit > 0)
                                    desiredOffset = ((lastTargetSpeed - speedLimit) * 100) / speedLimit;
                                // 限定范围
                                desiredOffset = std::max(std::min(desiredOffset, 100), 0);
                                smoothSpeedOffset = desiredOffset;
                                smoothTimestamp = now;
                            }
                            // 平滑调整到新偏移
                            if (std::abs(smoothSpeedOffset - offset) > kSmoothStep)
                            {
                                if (now - smoothTimestamp > kSmoothIntervalMs)
                                {
                                    if (smoothSpeedOffset < offset)
                                        smoothSpeedOffset += kSmoothStep;
                                    else
                                        smoothSpeedOffset -= kSmoothStep;
                                    smoothTimestamp = now;
                                }
                            }
                            else
                            {
                                smoothSpeedOffset = offset;
                            }
                            speedOffset = smoothSpeedOffset;
                            lastSpeedLimit = speedLimit;
                            setBit(frame, 46, true);
                            setBit(frame, 60, true);
                            if (emergencyVehicleDetectionRuntime)
                                setBit(frame, 59, true);
                            framesSent++;
                            driver.send(frame);
                        }
                        if (index == 1)
                        {
                            bool modified = false;
                            if (enhancedAutopilotRuntime)
                            {
                                setBit(frame, 19, false);
                                setBit(frame, 47, true);
                                modified = true;
                            }
                            if (modified)
                            {
                                framesSent++;
                                driver.send(frame);
                            }
                        }
                        if (index == 2)
                        {
                            if (FSDEnabled)
                            {
                                frame.data[0] &= ~(0b11000000);
                                frame.data[1] &= ~(0b00111111);
                                frame.data[0] |= (speedOffset & 0x03) << 6;
                                frame.data[1] |= (speedOffset >> 2);
                            }
                            frame.data[7] &= ~(0x07 << 4);
                            frame.data[7] |= (speedProfile & 0x07) << 4;
                            framesSent++;
                            driver.send(frame);
                        }
                        if (index == 0 && enablePrint)
                        {
                            char buf[LogRingBuffer::kMaxMsgLen];
                            snprintf(buf, sizeof(buf), "HW4Handler: FSD: %d, Profile: %d, Offset: %d (Smooth)",
                                     (bool)FSDEnabled, (int)speedProfile, (int)speedOffset);
                            logRing.push(buf,
            #ifndef NATIVE_BUILD
                                         millis()
            #else
                                         0
            #endif
                            );
            #ifndef NATIVE_BUILD
                            Serial.println(buf);
            #endif
                        }
                    }
                }
            };
                break;
            case 3:
                speedProfile = 0;
                break;
            default:
                break;
            }
            return;
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
                FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled)
            {
                if (speedOffsetManualMode)
                    speedOffset = (int)manualSpeedOffset;
                else {
                    int raw = (frame.data[3] >> 1) & 0x3F;
                    speedOffset = std::max(std::min((raw - 30) * 5, 100), 0);
                }
                setBit(frame, 46, true);
                setSpeedProfileV12V13(frame, speedProfile);
                framesSent++;
                driver.send(frame);
            }
            if (index == 1)
            {
                bool modified = false;
                if (enhancedAutopilotRuntime)
                {
                    setBit(frame, 19, false);
                    setBit(frame, 46, true);
                    modified = true;
                }
                if (modified)
                {
                    framesSent++;
                    driver.send(frame);
                }
            }
            if (index == 2 && FSDEnabled)
            {
                frame.data[0] &= ~(0b11000000);
                frame.data[1] &= ~(0b00111111);
                frame.data[0] |= (speedOffset & 0x03) << 6;
                frame.data[1] |= (speedOffset >> 2);
                framesSent++;
                driver.send(frame);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW3Handler: FSD: %d, Profile: %d, Offset: %d",
                         (bool)FSDEnabled, (int)speedProfile, (int)speedOffset);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};

/**
 * NagHandler — Autosteer nag suppression (counter+1 echo method)
 *
 * Replicates the Chinese TSL6P module behavior:
 * - Listens for CAN 880 (0x370) = EPAS3P_sysStatus
 * - When handsOnLevel = 0 (nag would trigger):
 *   1. Copies the real frame
 *   2. Sets byte 3 = 0xB6 (fixed torsionBarTorque = 1.80 Nm)
 *   3. Sets byte 4 |= 0x40 (handsOnLevel = 1)
 *   4. Increments counter (byte 6 lower nibble + 1)
 *   5. Recalculates checksum (byte 7)
 * - The real EPAS frame with the same counter arrives AFTER -> rejected as duplicate
 *
 * Tested: Model Y Performance 2022 HW3, Basic Autopilot
 * Bus: X179 pin 2/3 (CAN bus 4)
 *
 * Enable with build flag: -D NAG_KILLER
 */
struct NagHandler : public CarManagerBase
{
    Shared<bool> nagKillerActive{true};
    Shared<uint32_t> nagEchoCount{0};

    const uint32_t *filterIds() const override
    {
        static constexpr uint32_t ids[] = {880};
        return ids;
    }
    uint8_t filterIdCount() const override { return 1; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id != 880 || frame.dlc < 8)
            return;

        uint8_t handsOn = (frame.data[4] >> 6) & 0x03;

        if (!nagKillerActive || !nagKillerRuntime || handsOn != 0)
            return;

        CanFrame echo;
        echo.id = 880;
        echo.dlc = 8;

        echo.data[0] = frame.data[0];
        echo.data[1] = frame.data[1];
        echo.data[2] = (frame.data[2] & 0xF0) | 0x08;
        echo.data[5] = frame.data[5];

        // Fixed torque = 1.80 Nm (tRaw = 0x08B6)
        echo.data[3] = 0xB6;

        // handsOnLevel = 1
        echo.data[4] = frame.data[4] | 0x40;

        // Counter + 1
        uint8_t cnt = (frame.data[6] & 0x0F);
        cnt = (cnt + 1) & 0x0F;
        echo.data[6] = (frame.data[6] & 0xF0) | cnt;

        // Checksum: sum(byte0..byte6) + 0x73
        uint16_t sum = echo.data[0] + echo.data[1] + echo.data[2] + echo.data[3] + echo.data[4] + echo.data[5] + echo.data[6];
        echo.data[7] = static_cast<uint8_t>((sum + 0x73) & 0xFF);

        framesSent++;
        nagEchoCount++;
        driver.send(echo);

        if (enablePrint && (nagEchoCount % 500 == 1))
        {
            char buf[LogRingBuffer::kMaxMsgLen];
            snprintf(buf, sizeof(buf), "NagHandler: echo=%u",
                     (unsigned int)(uint32_t)nagEchoCount);
            logRing.push(buf,
#ifndef NATIVE_BUILD
                         millis()
#else
                         0
#endif
            );
#ifndef NATIVE_BUILD
            Serial.println(buf);
#endif
        }
    }
};

struct HW4Handler : public CarManagerBase
{
    const uint32_t *filterIds() const override
    {
        // 921 is always in the filter so the runtime ISA suppress switch
        // can take effect without re-flashing or re-initializing CAN.
        static constexpr uint32_t ids[] = {921, 1016, 1021};
        return ids;
    }
    uint8_t filterIdCount() const override { return 3; }

    void handleMessage(CanFrame &frame, CanDriver &driver) override
    {
        if (frame.id == 921)
        {
            if (frame.dlc < 8)
                return;
            if (!isaSpeedChimeSuppressRuntime)
                return;
            frame.data[1] |= 0x20;
            uint8_t sum = 0;
            for (int i = 0; i < 7; i++)
                sum += frame.data[i];
            sum += (921 & 0xFF) + (921 >> 8);
            frame.data[7] = sum & 0xFF;
            framesSent++;
            driver.send(frame);
            return;
        }
        if (frame.id == 1016)
        {
            if (frame.dlc < 6)
                return;
            if (!profileModeAutoRuntime)
                return; // Manual mode: don't overwrite user's profile choice
            auto fd = (frame.data[5] & 0b11100000) >> 5;
            switch (fd)
            {
            case 1:
            case 2:
            case 3:
                speedProfile = 3; // Max
                break;
            case 4:
                speedProfile = 2; // Hurry
                break;
            case 5:
                speedProfile = 1; // Normal
                break;
            case 6:
                speedProfile = 0; // Chill
                break;
            case 7:
                speedProfile = 4; // Sloth
                break;
            }
            return;
        }
        if (frame.id == 1021)
        {
            if (frame.dlc < 8)
                return;
            auto index = readMuxID(frame);
            if (index == 0)
                FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled)
            {
                if (speedOffsetManualMode)
                    speedOffset = (int)manualSpeedOffset;
                else {
                    int raw = (frame.data[3] >> 1) & 0x3F;
                    speedOffset = std::max(std::min((raw - 30) * 5, 100), 0);
                }
                setBit(frame, 46, true);
                setBit(frame, 60, true);
                if (emergencyVehicleDetectionRuntime)
                    setBit(frame, 59, true);
                framesSent++;
                driver.send(frame);
            }
            if (index == 1)
            {
                bool modified = false;
                if (enhancedAutopilotRuntime)
                {
                    setBit(frame, 19, false);
                    setBit(frame, 47, true);
                    modified = true;
                }
                if (modified)
                {
                    framesSent++;
                    driver.send(frame);
                }
            }
            if (index == 2)
            {
                if (FSDEnabled)
                {
                    frame.data[0] &= ~(0b11000000);
                    frame.data[1] &= ~(0b00111111);
                    frame.data[0] |= (speedOffset & 0x03) << 6;
                    frame.data[1] |= (speedOffset >> 2);
                }
                frame.data[7] &= ~(0x07 << 4);
                frame.data[7] |= (speedProfile & 0x07) << 4;
                framesSent++;
                driver.send(frame);
            }
            if (index == 0 && enablePrint)
            {
                char buf[LogRingBuffer::kMaxMsgLen];
                snprintf(buf, sizeof(buf), "HW4Handler: FSD: %d, Profile: %d, Offset: %d",
                         (bool)FSDEnabled, (int)speedProfile, (int)speedOffset);
                logRing.push(buf,
#ifndef NATIVE_BUILD
                             millis()
#else
                             0
#endif
                );
#ifndef NATIVE_BUILD
                Serial.println(buf);
#endif
            }
        }
    }
};
