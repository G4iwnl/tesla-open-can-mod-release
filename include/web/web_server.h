// include/web_server.h
#pragma once

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)

#include <algorithm>
#include <cstring>
#include <WiFi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <lwip/sockets.h>
#include <cJSON.h>
#include <Update.h>
#include <esp_system.h>
#include <driver/twai.h>
#if !defined(CONFIG_IDF_TARGET_ESP32)
#include <driver/temp_sensor.h>
#define HAS_TEMP_SENSOR 1
#endif

#include "shared_types.h"
#include "can_helpers.h"
#include "log_buffer.h"
#include "version.h"
#include "web/web_ui.h"

static const char *AP_SSID = "TeslaCAN";
static const char *AP_PASS = "12345678"; // WPA2, min 8 chars
static constexpr char kNvsNamespace[] = "canmod";
static constexpr char kNvsKeyIsaSpeedChime[] = "isa_speed_chime";
static constexpr char kNvsKeyEmergencyVehicleDetection[] = "emerg_veh_det";
static constexpr char kNvsKeyEnhancedAutopilot[] = "enh_autopilot";
static constexpr char kNvsKeyNagKiller[] = "nag_killer";
static constexpr char kNvsKeyHwMode[] = "hw_mode";
static constexpr char kNvsKeyChinaMode[] = "china_mode";
static constexpr char kNvsKeyProfileAuto[] = "profile_auto";
static constexpr char kNvsKeyManualProfile[] = "man_profile";
static constexpr char kNvsKeyWifiSsid[] = "wifi_ssid";
static constexpr char kNvsKeyWifiPass[] = "wifi_pass";
static constexpr char kNvsKeyPreheat[] = "preheat";
static constexpr char kNvsKeySmartOffsetEn[] = "smart_off_en";
static constexpr char kNvsKeySmartOffsetN[] = "smart_off_n";
static constexpr char kNvsKeySmartOffsetR[] = "smart_off_r";
static constexpr char kNvsKeyManualOffMode[] = "man_off_mode";
static constexpr char kNvsKeyManualOffVal[] = "man_off_val";

static_assert(sizeof(kNvsKeySmartOffsetEn) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeySmartOffsetN) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeySmartOffsetR) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyManualOffMode) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyManualOffVal) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyIsaSpeedChime) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyEmergencyVehicleDetection) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyEnhancedAutopilot) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyNagKiller) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyHwMode) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyChinaMode) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyProfileAuto) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyManualProfile) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyWifiSsid) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyWifiPass) - 1 <= 15, "NVS key too long");
static_assert(sizeof(kNvsKeyPreheat) - 1 <= 15, "NVS key too long");

// Handler latency tracking
static volatile uint32_t handlerLatencyUs = 0;  // last handler call duration
static volatile uint32_t handlerLatencyMax = 0;  // peak latency
static volatile uint32_t handlerCallCount = 0;

// ESP32 chip temperature
static float chipTempC = 0;
static bool tempSensorInited = false;

// Build-flag fallbacks for STA WiFi credentials. Used as the first-boot
// defaults; once the user saves new credentials they live in NVS.
#ifdef WIFI_STA_SSID
static const char *kDefaultStaSsid = WIFI_STA_SSID;
#else
static const char *kDefaultStaSsid = "";
#endif
#ifdef WIFI_STA_PASS
static const char *kDefaultStaPass = WIFI_STA_PASS;
#else
static const char *kDefaultStaPass = "";
#endif

// Runtime mutable WiFi STA credentials (loaded from NVS, fallback to build flags)
static char gStaSsid[33] = {0};
static char gStaPass[64] = {0};

// On RUNTIME_HW_SWITCH builds (Waveshare ESP32-S3-RS485-CAN) every feature
// is compiled in, so it is "supported" regardless of the active HW mode.
// The handler skips features that don't apply to its CAN ID set.
#if defined(RUNTIME_HW_SWITCH) || (defined(HW4) && defined(ISA_SPEED_CHIME_SUPPRESS))
static constexpr bool kWebSupportsIsaSpeedChimeSuppress = true;
#else
static constexpr bool kWebSupportsIsaSpeedChimeSuppress = false;
#endif

#if defined(RUNTIME_HW_SWITCH) || (defined(HW4) && defined(EMERGENCY_VEHICLE_DETECTION))
static constexpr bool kWebSupportsEmergencyVehicleDetection = true;
#else
static constexpr bool kWebSupportsEmergencyVehicleDetection = false;
#endif

#if defined(RUNTIME_HW_SWITCH) || defined(ENHANCED_AUTOPILOT)
static constexpr bool kWebSupportsEnhancedAutopilot = true;
#else
static constexpr bool kWebSupportsEnhancedAutopilot = false;
#endif

// NagKiller stays compile-time only; on RUNTIME_HW_SWITCH builds it stays
// false because no NagHandler is reachable through the runtime factory.
#if defined(NAG_KILLER)
static constexpr bool kWebSupportsNagKiller = true;
#else
static constexpr bool kWebSupportsNagKiller = false;
#endif

// --- NVS helpers ---

static bool nvsInit()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        Serial.printf("NVS: corruption detected (err=%d), erasing and re-init\n", (int)err);
        logRing.push("[NVS] Corruption detected, factory reset", millis());
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        Serial.printf("NVS: init failed with err=%d\n", (int)err);
        logRing.push("[NVS] Init failed, using defaults", millis());
    }
    return err == ESP_OK;
}

static bool nvsReadBool(const char *key, bool fallback)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return fallback;
    uint8_t val = 0;
    if (nvs_get_u8(handle, key, &val) != ESP_OK)
    {
        nvs_close(handle);
        return fallback;
    }
    nvs_close(handle);
    return val != 0;
}

static bool nvsWriteBool(const char *key, bool enabled)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: open failed for %s (%ld)\n", key, static_cast<long>(err));
        return false;
    }

    err = nvs_set_u8(handle, key, enabled ? 1 : 0);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: set failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        Serial.printf("NVS: commit failed for %s (%ld)\n", key, static_cast<long>(err));
        nvs_close(handle);
        return false;
    }
    nvs_close(handle);
    return true;
}

static uint8_t nvsReadU8(const char *key, uint8_t fallback)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return fallback;
    uint8_t val = fallback;
    nvs_get_u8(handle, key, &val);
    nvs_close(handle);
    return val;
}

static bool nvsWriteU8(const char *key, uint8_t value)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    bool ok = (nvs_set_u8(handle, key, value) == ESP_OK) && (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);
    return ok;
}

static int16_t nvsReadI16(const char *key, int16_t fallback)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return fallback;
    int16_t val = fallback;
    nvs_get_i16(handle, key, &val);
    nvs_close(handle);
    return val;
}

static bool nvsWriteI16(const char *key, int16_t value)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    bool ok = (nvs_set_i16(handle, key, value) == ESP_OK) && (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);
    return ok;
}

static bool nvsWriteBlob(const char *key, const void *data, size_t len)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    bool ok = (nvs_set_blob(handle, key, data, len) == ESP_OK) && (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);
    return ok;
}

static bool nvsReadBlob(const char *key, void *data, size_t maxLen, size_t *outLen)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
        return false;
    *outLen = maxLen;
    bool ok = (nvs_get_blob(handle, key, data, outLen) == ESP_OK);
    nvs_close(handle);
    return ok;
}

static void nvsReadString(const char *key, char *out, size_t outSize, const char *fallback)
{
    if (outSize == 0)
        return;
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READONLY, &handle) != ESP_OK)
    {
        strncpy(out, fallback, outSize - 1);
        out[outSize - 1] = '\0';
        return;
    }
    size_t required = outSize;
    if (nvs_get_str(handle, key, out, &required) != ESP_OK)
    {
        strncpy(out, fallback, outSize - 1);
        out[outSize - 1] = '\0';
    }
    nvs_close(handle);
}

static bool nvsWriteString(const char *key, const char *value)
{
    nvs_handle_t handle;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    bool ok = (nvs_set_str(handle, key, value) == ESP_OK) && (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);
    return ok;
}

// Load all persisted runtime settings from NVS into the global Shared<> vars
// and the WiFi credential buffers. Idempotent — safe to call multiple times.
// MUST be called early in setup() before the first handler frame is processed.
static void nvsLoadAllSettings()
{
    if (!nvsInit())
    {
        Serial.println("NVS: init failed, using defaults");
        strncpy(gStaSsid, kDefaultStaSsid, sizeof(gStaSsid) - 1);
        strncpy(gStaPass, kDefaultStaPass, sizeof(gStaPass) - 1);
        return;
    }
    bypassTlsscRequirementRuntime = nvsReadBool("bypass_tlssc", kBypassTlsscRequirementDefaultEnabled);
    isaSpeedChimeSuppressRuntime = nvsReadBool(kNvsKeyIsaSpeedChime, kIsaSpeedChimeSuppressDefaultEnabled);
    emergencyVehicleDetectionRuntime = nvsReadBool(kNvsKeyEmergencyVehicleDetection, kEmergencyVehicleDetectionDefaultEnabled);
    enhancedAutopilotRuntime = nvsReadBool(kNvsKeyEnhancedAutopilot, kEnhancedAutopilotDefaultEnabled);
    nagKillerRuntime = nvsReadBool(kNvsKeyNagKiller, kNagKillerDefaultEnabled);
    chinaModeRuntime = nvsReadBool(kNvsKeyChinaMode, kChinaModeDefaultEnabled);
    profileModeAutoRuntime = nvsReadBool(kNvsKeyProfileAuto, kProfileModeAutoDefaultEnabled);
    // Preheat is intentionally NOT loaded from NVS. It's a "test region"
    // feature that injects CAN frames; auto-resuming it on boot would be
    // dangerous (e.g. parking the car, leaving preheat on, next boot it
    // silently starts injecting again). The user must explicitly enable
    // preheat after every reboot.
    preheatRuntime = false;

    uint8_t hw = nvsReadU8(kNvsKeyHwMode, 2);
    if (hw > 2)
        hw = 2;
    hwModeRuntime = hw;

    nvsReadString(kNvsKeyWifiSsid, gStaSsid, sizeof(gStaSsid), kDefaultStaSsid);
    nvsReadString(kNvsKeyWifiPass, gStaPass, sizeof(gStaPass), kDefaultStaPass);

    // Smart offset: restore enabled flag + rules blob
    smartOffsetEnabled = nvsReadBool(kNvsKeySmartOffsetEn, false);
    {
        uint8_t ruleCount = nvsReadU8(kNvsKeySmartOffsetN, 0);
        if (ruleCount > 0 && ruleCount <= kMaxSmartRules)
        {
            size_t blobLen = 0;
            SmartOffsetRule rules[kMaxSmartRules];
            if (nvsReadBlob(kNvsKeySmartOffsetR, rules, sizeof(rules), &blobLen) &&
                blobLen == ruleCount * sizeof(SmartOffsetRule))
            {
                for (int i = 0; i < ruleCount; i++)
                    smartOffsetRules.rules[i] = rules[i];
                smartOffsetRules.count = ruleCount;
                Serial.printf("NVS: smart offset rules loaded (%d rules)\n", ruleCount);
            }
        }
    }

    // Manual speed offset: restore mode + value
    speedOffsetManualMode = nvsReadBool(kNvsKeyManualOffMode, false);
    {
        int16_t v = nvsReadI16(kNvsKeyManualOffVal, 0);
        if (v >= 0 && v <= 200)
            manualSpeedOffset = v;
    }

    // Chip temperature sensor init
#ifdef HAS_TEMP_SENSOR
    temp_sensor_config_t tempConf = TSENS_CONFIG_DEFAULT();
    if (temp_sensor_set_config(tempConf) == ESP_OK && temp_sensor_start() == ESP_OK)
    {
        tempSensorInited = true;
        Serial.println("NVS: chip temp sensor initialized");
    }
#endif

    Serial.printf("NVS loaded: hw=%d cn=%d auto=%d ssid=\"%s\"\n",
                  (int)hw, (int)(bool)chinaModeRuntime, (int)(bool)profileModeAutoRuntime, gStaSsid);
}

// --- Rate limiter ---

static unsigned long lastToggleMs = 0;
static const unsigned long kToggleMinIntervalMs = 500;

static bool rateLimitOk()
{
    unsigned long now = millis();
    if (now - lastToggleMs < kToggleMinIntervalMs)
        return false;
    lastToggleMs = now;
    return true;
}

static bool parseToggleBody(httpd_req_t *req, bool &enabledOut)
{
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return false;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return false;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (!cJSON_IsBool(enabled))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing enabled");
        return false;
    }

    enabledOut = cJSON_IsTrue(enabled);
    cJSON_Delete(json);
    return true;
}

static esp_err_t featureToggleHandler(httpd_req_t *req, Shared<bool> &target, bool supported,
                                      const char *nvsKey, const char *logName)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (!supported)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Feature not available");
        return ESP_FAIL;
    }

    bool enabled = false;
    if (!parseToggleBody(req, enabled))
        return ESP_FAIL;

    if (!nvsWriteBool(nvsKey, enabled))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    target = enabled;
    Serial.printf("Web: %s set to %d\n", logName, enabled);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void restartTask(void *param)
{
    (void)param;
    vTaskDelay(pdMS_TO_TICKS(750));
    ESP.restart();
}

// --- HTTP handlers ---

static esp_err_t rootHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WEB_UI_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t statusHandler(httpd_req_t *req)
{
    // Parse log_since from query string
    uint32_t logSince = 0;
    char queryBuf[32];
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK)
    {
        char val[16];
        if (httpd_query_key_value(queryBuf, "log_since", val, sizeof(val)) == ESP_OK)
        {
            logSince = strtoul(val, nullptr, 10);
        }
    }

    // Read handler state (atomic reads -- no lock needed)
    bool fsdEnabled = appHandler ? (bool)appHandler->FSDEnabled : false;
    bool bypassTlssc = (bool)bypassTlsscRequirementRuntime;
    int speedProfile = appHandler ? (int)appHandler->speedProfile : 0;
    int speedOffset = appHandler ? (int)appHandler->speedOffset : 0;
    bool enablePrint = appHandler ? (bool)appHandler->enablePrint : true;
    bool isaSuppress = kWebSupportsIsaSpeedChimeSuppress ? (bool)isaSpeedChimeSuppressRuntime : false;
    bool emergencyVehicleDetection =
        kWebSupportsEmergencyVehicleDetection ? (bool)emergencyVehicleDetectionRuntime : false;
    bool enhancedAutopilot =
        kWebSupportsEnhancedAutopilot ? (bool)enhancedAutopilotRuntime : false;
    bool nagKiller = kWebSupportsNagKiller ? (bool)nagKillerRuntime : false;
    bool chinaMode = (bool)chinaModeRuntime;
    bool profileAuto = (bool)profileModeAutoRuntime;
    bool preheat = (bool)preheatRuntime;
    int hwMode = (int)(uint8_t)hwModeRuntime;

    // Build JSON with direct snprintf — heap buffer (safe for concurrent requests)
    static constexpr size_t kBufSize = 8192;
    char *buf = (char *)malloc(kBufSize);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    size_t pos = 0;

#define JS_APPEND(fmt, ...) \
    do { \
        int _n = snprintf(buf + pos, kBufSize - pos, fmt, ##__VA_ARGS__); \
        if (_n > 0 && pos + (size_t)_n < kBufSize) pos += (size_t)_n; \
    } while (0)

#define JS_BOOL(v) ((v) ? "true" : "false")

#define JS_FEAT(name, sup, en, bld) \
    JS_APPEND("\"" name "\":{\"supported\":%s,\"enabled\":%s,\"build_enabled\":%s}", \
              JS_BOOL(sup), JS_BOOL((sup) && (en)), JS_BOOL(bld))

    // Scalar fields
    IPAddress sta = WiFi.localIP();
    JS_APPEND("{\"fsd_enabled\":%s,"
              "\"bypass_tlssc_requirement\":%s,"
              "\"isa_speed_chime_suppress\":%s,"
              "\"emergency_vehicle_detection\":%s,"
              "\"enhanced_autopilot\":%s,"
              "\"nag_killer\":%s,"
              "\"china_mode\":%s,"
              "\"profile_mode_auto\":%s,"
              "\"preheat\":%s,"
              "\"hw_mode\":%d,"
              "\"speed_profile\":%d,"
              "\"speed_offset\":%d,"
              "\"speed_offset_manual\":%s,"
              "\"smart_offset\":%s,"
              "\"enable_print\":%s,"
              "\"uptime_s\":%lu,"
              "\"log_head\":%u,"
              "\"version\":\"%s\","
              "\"ap_ssid\":\"%s\","
              "\"sta_ssid\":\"%s\","
              "\"sta_ip\":\"%u.%u.%u.%u\","
              "\"sta_connected\":%s,",
              JS_BOOL(fsdEnabled), JS_BOOL(bypassTlssc), JS_BOOL(isaSuppress),
              JS_BOOL(emergencyVehicleDetection), JS_BOOL(enhancedAutopilot),
              JS_BOOL(nagKiller), JS_BOOL(chinaMode), JS_BOOL(profileAuto),
              JS_BOOL(preheat), hwMode, speedProfile, speedOffset,
              JS_BOOL((bool)speedOffsetManualMode), JS_BOOL((bool)smartOffsetEnabled),
              JS_BOOL(enablePrint), millis() / 1000, logRing.currentHead(),
              FIRMWARE_VERSION, AP_SSID, gStaSsid,
              sta[0], sta[1], sta[2], sta[3],
              JS_BOOL(WiFi.status() == WL_CONNECTED));

    // Features object
    JS_APPEND("\"features\":{");
    JS_FEAT("bypass_tlssc_requirement", kBypassTlsscRequirementBuildEnabled, bypassTlssc, kBypassTlsscRequirementBuildEnabled);
    JS_APPEND(",");
    JS_FEAT("isa_speed_chime_suppress", kWebSupportsIsaSpeedChimeSuppress, isaSuppress, kIsaSpeedChimeSuppressBuildEnabled);
    JS_APPEND(",");
    JS_FEAT("emergency_vehicle_detection", kWebSupportsEmergencyVehicleDetection, emergencyVehicleDetection, kEmergencyVehicleDetectionBuildEnabled);
    JS_APPEND(",");
    JS_FEAT("enhanced_autopilot", kWebSupportsEnhancedAutopilot, enhancedAutopilot, kEnhancedAutopilotBuildEnabled);
    JS_APPEND(",");
    JS_FEAT("nag_killer", kWebSupportsNagKiller, nagKiller, kNagKillerBuildEnabled);
    JS_APPEND(",");
    JS_FEAT("china_mode", true, chinaMode, true);
    JS_APPEND(",");
    JS_FEAT("profile_mode_auto", true, profileAuto, true);
    JS_APPEND(",");
    JS_FEAT("preheat", true, preheat, true);
    JS_APPEND(",");
    JS_FEAT("ota", true, false, true);
    JS_APPEND("},");

    // Log entries since last poll
    LogRingBuffer::Entry logEntries[LogRingBuffer::kCapacity];
    int logCount = logRing.readSince(logSince, logEntries, LogRingBuffer::kCapacity);
    JS_APPEND("\"logs\":[");
    for (int i = 0; i < logCount; i++)
    {
        if (i > 0)
            JS_APPEND(",");
        // Escape any quotes in log messages for JSON safety
        JS_APPEND("{\"msg\":\"");
        // Safe append: copy msg char-by-char, escaping special chars
        const char *msg = logEntries[i].msg;
        for (; *msg && pos + 6 < kBufSize; msg++)
        {
            if (*msg == '"')
            {
                buf[pos++] = '\\';
                buf[pos++] = '"';
            }
            else if (*msg == '\\')
            {
                buf[pos++] = '\\';
                buf[pos++] = '\\';
            }
            else if (*msg == '\n')
            {
                buf[pos++] = '\\';
                buf[pos++] = 'n';
            }
            else
            {
                buf[pos++] = *msg;
            }
        }
        JS_APPEND("\",\"ts\":%lu}", (unsigned long)logEntries[i].timestamp_ms);
    }
    JS_APPEND("],");

    // CAN bus diagnostics
    twai_status_info_t twaiStatus;
    JS_APPEND("\"can\":{");
    if (twai_get_status_info(&twaiStatus) == ESP_OK)
    {
        const char *stateStr = "UNKNOWN";
        switch (twaiStatus.state)
        {
        case TWAI_STATE_STOPPED:
            stateStr = "STOPPED";
            break;
        case TWAI_STATE_RUNNING:
            stateStr = "RUNNING";
            break;
        case TWAI_STATE_BUS_OFF:
            stateStr = "BUS_OFF";
            break;
        case TWAI_STATE_RECOVERING:
            stateStr = "RECOVERING";
            break;
        }
        JS_APPEND("\"state\":\"%s\","
                   "\"rx_errors\":%d,"
                   "\"tx_errors\":%d,"
                   "\"bus_errors\":%lu,"
                   "\"rx_missed\":%lu,"
                   "\"rx_queued\":%lu,",
                   stateStr,
                   (int)twaiStatus.rx_error_counter,
                   (int)twaiStatus.tx_error_counter,
                   (unsigned long)twaiStatus.bus_error_count,
                   (unsigned long)twaiStatus.rx_missed_count,
                   (unsigned long)twaiStatus.msgs_to_rx);
    }
    else
    {
        JS_APPEND("\"state\":\"NO_DRIVER\",");
    }
    JS_APPEND("\"frames_received\":%lu,\"frames_sent\":%lu},",
              (unsigned long)(appHandler ? (uint32_t)appHandler->frameCount : 0),
              (unsigned long)(appHandler ? (uint32_t)appHandler->framesSent : 0));

    // CAN Monitor status
    JS_APPEND("\"monitor\":{\"inited\":%s,\"enabled\":%s,\"entries\":%lu,\"capacity\":%lu},",
              JS_BOOL(canMonitor.isInited()), JS_BOOL(canMonitor.isEnabled()),
              (unsigned long)canMonitor.entryCount(), (unsigned long)canMonitor.capacity());

    // Chip temperature
#ifdef HAS_TEMP_SENSOR
    if (tempSensorInited)
    {
        temp_sensor_read_celsius(&chipTempC);
    }
#endif
    JS_APPEND("\"chip_temp_c\":%.1f,", chipTempC);

    // Handler latency
    JS_APPEND("\"handler\":{\"latency_us\":%lu,\"latency_max_us\":%lu,\"calls\":%lu},",
              (unsigned long)handlerLatencyUs, (unsigned long)handlerLatencyMax,
              (unsigned long)handlerCallCount);

    // Free heap
    JS_APPEND("\"free_heap\":%lu,\"free_psram\":%lu}",
              (unsigned long)ESP.getFreeHeap(),
              (unsigned long)ESP.getFreePsram());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, pos);
    free(buf);

#undef JS_APPEND
#undef JS_BOOL
#undef JS_FEAT

    return ESP_OK;
}

static esp_err_t bypassTlsscRequirementHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, bypassTlsscRequirementRuntime, true, "bypass_tlssc", "BYPASS_TLSSC_REQUIREMENT");
}

static esp_err_t isaSpeedChimeSuppressHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, isaSpeedChimeSuppressRuntime,
                                kWebSupportsIsaSpeedChimeSuppress,
                                kNvsKeyIsaSpeedChime, "ISA_SPEED_CHIME_SUPPRESS");
}

static esp_err_t emergencyVehicleDetectionHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, emergencyVehicleDetectionRuntime,
                                kWebSupportsEmergencyVehicleDetection,
                                kNvsKeyEmergencyVehicleDetection, "EMERGENCY_VEHICLE_DETECTION");
}

static esp_err_t enhancedAutopilotHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, enhancedAutopilotRuntime,
                                kWebSupportsEnhancedAutopilot,
                                kNvsKeyEnhancedAutopilot, "ENHANCED_AUTOPILOT");
}

static esp_err_t nagKillerHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, nagKillerRuntime, kWebSupportsNagKiller, kNvsKeyNagKiller, "NAG_KILLER");
}

static esp_err_t chinaModeHandler(httpd_req_t *req)
{
    return featureToggleHandler(req, chinaModeRuntime, true, kNvsKeyChinaMode, "CHINA_MODE");
}

static esp_err_t profileModeAutoHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    bool enabled = false;
    if (!parseToggleBody(req, enabled))
        return ESP_FAIL;
    if (!nvsWriteBool(kNvsKeyProfileAuto, enabled))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist setting");
        return ESP_FAIL;
    }
    profileModeAutoRuntime = enabled;
    // When switching to manual mode at runtime, restore the previously saved
    // manual profile so the profile doesn't stay at whatever the stalk last set.
    if (!enabled && appHandler)
    {
        uint8_t saved = nvsReadU8(kNvsKeyManualProfile, 1);
        if (saved > 4)
            saved = 4;
        appHandler->speedProfile = saved;
        Serial.printf("Web: PROFILE_MODE_AUTO disabled, restored profile=%d\n", (int)saved);
    }
    else
    {
        Serial.printf("Web: PROFILE_MODE_AUTO set to %d\n", (int)enabled);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t preheatHandler(httpd_req_t *req)
{
    // Pre-check: if user wants to enable preheat, make sure CAN bus is in
    // a healthy state. If TX errors are already high, the bus has no ACK
    // source and enabling preheat will only make it worse.
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (!cJSON_IsBool(enabled))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing enabled");
        return ESP_FAIL;
    }
    bool wantOn = cJSON_IsTrue(enabled);
    cJSON_Delete(json);

    if (wantOn)
    {
        twai_status_info_t st;
        if (twai_get_status_info(&st) == ESP_OK)
        {
            if (st.state != TWAI_STATE_RUNNING || st.tx_error_counter > 64)
            {
                char err[160];
                snprintf(err, sizeof(err),
                         "{\"ok\":false,\"err\":\"CAN bus not healthy (state=%d tx_err=%d). Reset by power-cycling the board.\"}",
                         (int)st.state, (int)st.tx_error_counter);
                httpd_resp_set_status(req, "422 Unprocessable");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, err, HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
        }
    }

    // Preheat state is in-memory only (not persisted) — see nvsLoadAllSettings()
    preheatRuntime = wantOn;
    if (wantOn)
        preheatStartMs = millis();
    else
        preheatStartMs = 0;
    Serial.printf("Web: PREHEAT set to %d (in-memory only)\n", (int)wantOn);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t preheatStatusHandler(httpd_req_t *req)
{
    DecodedSignals sig;
    decodeSignals(canLive, sig);

    bool active = (bool)preheatRuntime;
    unsigned long elapsed = (active && preheatStartMs > 0) ? (millis() - preheatStartMs) / 1000 : 0;

    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"active\":%s,\"elapsed_s\":%lu,\"bat_temp_min\":%d,\"bat_temp_max\":%d,"
             "\"bat_soc\":%.1f,\"auto_stop_temp\":%d,\"max_duration_min\":%lu}",
             active ? "true" : "false",
             elapsed,
             (int)sig.bmsTempMin, (int)sig.bmsTempMax,
             sig.bmsSoc,
             (int)preheatAutoStopTemp,
             preheatMaxDurationMs / 60000UL);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t preheatConfigHandler(httpd_req_t *req)
{
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *temp = cJSON_GetObjectItem(json, "auto_stop_temp");
    if (cJSON_IsNumber(temp))
    {
        int v = temp->valueint;
        if (v >= -10 && v <= 30)
            preheatAutoStopTemp = (int8_t)v;
    }
    cJSON *dur = cJSON_GetObjectItem(json, "max_duration_min");
    if (cJSON_IsNumber(dur))
    {
        int v = dur->valueint;
        if (v >= 5 && v <= 120)
            preheatMaxDurationMs = (unsigned long)v * 60UL * 1000UL;
    }
    cJSON_Delete(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t hwModeHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *modeItem = cJSON_GetObjectItem(json, "mode");
    if (!cJSON_IsNumber(modeItem))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing mode");
        return ESP_FAIL;
    }
    int mode = modeItem->valueint;
    cJSON_Delete(json);
    if (mode < 0 || mode > 2)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "mode must be 0,1,2");
        return ESP_FAIL;
    }
    if (!nvsWriteU8(kNvsKeyHwMode, (uint8_t)mode))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist hw_mode");
        return ESP_FAIL;
    }
    Serial.printf("Web: hw_mode set to %d, restarting in 1s\n", mode);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot_hw", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

static esp_err_t speedProfileHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *valItem = cJSON_GetObjectItem(json, "value");
    if (!cJSON_IsNumber(valItem))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
        return ESP_FAIL;
    }
    int v = valItem->valueint;
    cJSON_Delete(json);
    if (v < 0)
        v = 0;
    if (v > 4)
        v = 4;
    if (appHandler)
        appHandler->speedProfile = v;
    nvsWriteU8(kNvsKeyManualProfile, (uint8_t)v);
    // Manually setting a profile implies the user wants manual mode.
    // Ensure auto mode is off so the next follow-distance stalk frame (1016)
    // does not immediately overwrite the profile the user just set.
    if ((bool)profileModeAutoRuntime)
    {
        profileModeAutoRuntime = false;
        nvsWriteBool(kNvsKeyProfileAuto, false);
        Serial.printf("Web: profile_mode_auto disabled (manual profile set to %d)\n", v);
    }
    else
    {
        Serial.printf("Web: manual speed profile = %d\n", v);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t speedOffsetHandler(httpd_req_t *req)
{
    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *valItem = cJSON_GetObjectItem(json, "value");
    cJSON *manItem = cJSON_GetObjectItem(json, "manual");
    bool manualMode = cJSON_IsBool(manItem) ? cJSON_IsTrue(manItem) : true;
    if (cJSON_IsNumber(valItem))
    {
        int v = valItem->valueint;
        if (v < 0) v = 0;
        if (v > 200) v = 200; // max 50% (50*4=200)
        manualSpeedOffset = v;
        if (appHandler)
            appHandler->speedOffset = v;
        nvsWriteI16(kNvsKeyManualOffVal, (int16_t)v);
    }
    speedOffsetManualMode = manualMode;
    nvsWriteBool(kNvsKeyManualOffMode, manualMode);
    cJSON_Delete(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t smartOffsetHandler(httpd_req_t *req)
{
    char body[512];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Toggle smart mode
    cJSON *enItem = cJSON_GetObjectItem(json, "enabled");
    if (cJSON_IsBool(enItem))
    {
        bool en = cJSON_IsTrue(enItem);
        smartOffsetEnabled = en;
        nvsWriteBool(kNvsKeySmartOffsetEn, en);
    }

    // Update rules — build in temporary copy, then swap atomically
    cJSON *rulesArr = cJSON_GetObjectItem(json, "rules");
    if (cJSON_IsArray(rulesArr))
    {
        int n = cJSON_GetArraySize(rulesArr);
        if (n > kMaxSmartRules) n = kMaxSmartRules;
        SmartOffsetRule tmpRules[kMaxSmartRules] = {};
        for (int i = 0; i < n; i++)
        {
            cJSON *item = cJSON_GetArrayItem(rulesArr, i);
            cJSON *ms = cJSON_GetObjectItem(item, "maxSpeed");
            cJSON *op = cJSON_GetObjectItem(item, "offsetPct");
            if (cJSON_IsNumber(ms) && cJSON_IsNumber(op))
            {
                int msv = ms->valueint;
                int opv = op->valueint;
                if (msv < 1) msv = 1;
                if (msv > 999) msv = 999;
                if (opv < 0) opv = 0;
                if (opv > 50) opv = 50;
                tmpRules[i].maxSpeed = msv;
                tmpRules[i].offsetPct = opv;
            }
        }
        // Swap count to 0 first so CAN loop sees no rules during update,
        // then copy rules, then set the real count.
        smartOffsetRules.count = 0;
        memcpy(smartOffsetRules.rules, tmpRules, sizeof(tmpRules));
        smartOffsetRules.count = n;
        // Persist rules to NVS
        nvsWriteU8(kNvsKeySmartOffsetN, (uint8_t)n);
        nvsWriteBlob(kNvsKeySmartOffsetR, smartOffsetRules.rules, n * sizeof(SmartOffsetRule));
    }

    cJSON_Delete(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t smartOffsetGetHandler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "enabled", (bool)smartOffsetEnabled);
    cJSON_AddNumberToObject(root, "current_pct", (int)manualSpeedOffset / 4);
    cJSON *arr = cJSON_AddArrayToObject(root, "rules");
    for (int i = 0; i < smartOffsetRules.count; i++)
    {
        cJSON *r = cJSON_CreateObject();
        cJSON_AddNumberToObject(r, "maxSpeed", smartOffsetRules.rules[i].maxSpeed);
        cJSON_AddNumberToObject(r, "offsetPct", smartOffsetRules.rules[i].offsetPct);
        cJSON_AddItemToArray(arr, r);
    }
    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t wifiConfigHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    char body[256];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';
    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON *ssidItem = cJSON_GetObjectItem(json, "ssid");
    cJSON *passItem = cJSON_GetObjectItem(json, "pass");
    if (!cJSON_IsString(ssidItem) || ssidItem->valuestring[0] == '\0')
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ssid required");
        return ESP_FAIL;
    }
    const char *newSsid = ssidItem->valuestring;
    const char *newPass = (cJSON_IsString(passItem) && passItem->valuestring[0]) ? passItem->valuestring : nullptr;

    if (strlen(newSsid) >= sizeof(gStaSsid))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ssid too long");
        return ESP_FAIL;
    }
    if (newPass && strlen(newPass) < 8)
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "password must be >= 8 chars");
        return ESP_FAIL;
    }
    if (newPass && strlen(newPass) >= sizeof(gStaPass))
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "password too long");
        return ESP_FAIL;
    }

    bool ok = nvsWriteString(kNvsKeyWifiSsid, newSsid);
    if (newPass)
        ok = ok && nvsWriteString(kNvsKeyWifiPass, newPass);
    cJSON_Delete(json);

    if (!ok)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist WiFi");
        return ESP_FAIL;
    }
    Serial.printf("Web: wifi saved (ssid=%s), restarting in 1s\n", newSsid);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot_wifi", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

static esp_err_t enablePrintHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (cJSON_IsBool(enabled) && appHandler)
    {
        appHandler->enablePrint = cJSON_IsTrue(enabled);
    }
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t otaHandler(httpd_req_t *req)
{
    // --- Authentication: require X-OTA-Token header matching AP password ---
    char tokenBuf[64] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-OTA-Token", tokenBuf, sizeof(tokenBuf)) != ESP_OK
        || strcmp(tokenBuf, AP_PASS) != 0)
    {
        httpd_resp_set_status(req, "403 Forbidden");
        httpd_resp_send(req, "Invalid or missing OTA token", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    if (req->content_len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing firmware payload");
        return ESP_FAIL;
    }

    // Check for MD5 header for integrity verification
    char md5Header[33] = {0};
    bool hasMd5 = false;
    if (httpd_req_get_hdr_value_str(req, "X-Firmware-MD5", md5Header, sizeof(md5Header)) == ESP_OK && md5Header[0])
    {
        hasMd5 = true;
        Update.setMD5(md5Header);
        Serial.printf("OTA: MD5 verification enabled: %s\n", md5Header);
    }

    if (!Update.begin(req->content_len))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, Update.errorString());
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    int timeoutCount = 0;
    uint8_t buffer[1024];
    while (remaining > 0)
    {
        int received = httpd_req_recv(req, reinterpret_cast<char *>(buffer),
                                      std::min(remaining, (int)sizeof(buffer)));
        if (received == HTTPD_SOCK_ERR_TIMEOUT)
        {
            if (++timeoutCount >= 5)
            {
                Update.abort();
                httpd_resp_set_status(req, "408 Request Timeout");
                httpd_resp_send(req, "Upload timed out", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
            continue;
        }
        if (received <= 0)
        {
            Update.abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failed");
            return ESP_FAIL;
        }
        timeoutCount = 0;
        if (Update.write(buffer, received) != (size_t)received)
        {
            Update.abort();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, Update.errorString());
            return ESP_FAIL;
        }
        remaining -= received;
    }

    if (!Update.end(true))
    {
        char errMsg[128];
        snprintf(errMsg, sizeof(errMsg), "%s%s", 
                 hasMd5 ? "MD5 mismatch or " : "", Update.errorString());
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, errMsg);
        return ESP_FAIL;
    }

    Serial.println("Web: OTA upload complete, restarting");
    logRing.push("[OTA] Firmware verified and written, rebooting", millis());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"restarting\":true}", HTTPD_RESP_USE_STRLEN);
    xTaskCreatePinnedToCore(restartTask, "reboot", 2048, NULL, 1, NULL, 0);
    return ESP_OK;
}

// Captive portal: redirect connectivity checks to root
static esp_err_t captiveRedirectHandler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// --- DNS captive portal task ---

static Shared<bool> dnsTaskStop{false};

static void dnsTask(void *param)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        Serial.println("DNS: socket failed");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        Serial.println("DNS: bind failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[512];
    struct sockaddr_in client;
    socklen_t clientLen;

    // Set receive timeout so we can check the stop flag periodically
    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (!dnsTaskStop)
    {
        clientLen = sizeof(client);
        int len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *)&client, &clientLen);
        if (len < 12)
            continue;

        // Build DNS response: copy query, set response flags, append answer
        buf[2] = 0x81; // QR=1, Opcode=0, AA=1, TC=0, RD=1
        buf[3] = 0x80; // RA=1, RCODE=0
        buf[6] = 0x00; // ANCOUNT = 1
        buf[7] = 0x01;

        // Append answer after the query section
        int pos = len;
        buf[pos++] = 0xC0; // Name pointer to offset 12 (query name)
        buf[pos++] = 0x0C;
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Type A
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Class IN
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x3C; // TTL 60s
        buf[pos++] = 0x00;
        buf[pos++] = 0x04; // RDLENGTH 4
        buf[pos++] = 192;
        buf[pos++] = 168; // 192.168.4.1
        buf[pos++] = 4;
        buf[pos++] = 1;

        sendto(sock, buf, pos, 0,
               (struct sockaddr *)&client, clientLen);
    }
    close(sock);
    vTaskDelete(NULL);
}

// --- CAN Monitor log API ---

#include "can_monitor.h"
#include "can_live.h"

static esp_err_t canLogStatusHandler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "inited", canMonitor.isInited());
    cJSON_AddBoolToObject(root, "enabled", canMonitor.isEnabled());
    cJSON_AddNumberToObject(root, "entries", canMonitor.entryCount());
    cJSON_AddNumberToObject(root, "capacity", canMonitor.capacity());
    float pct = canMonitor.capacity() > 0
                    ? (float)canMonitor.entryCount() / canMonitor.capacity() * 100.0f
                    : 0;
    cJSON_AddNumberToObject(root, "fill_pct", pct);
    cJSON_AddNumberToObject(root, "entry_size", (int)sizeof(CanLogEntry));
    cJSON_AddNumberToObject(root, "watch_ids", (int)CanMonitor::kWatchIdCount);

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t canLogStartHandler(httpd_req_t *req)
{
    if (!canMonitor.isInited())
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "PSRAM not initialized");
        return ESP_FAIL;
    }
    canMonitor.setEnabled(true);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"monitor\":\"started\"}");
    logRing.push("[MONITOR] CAN log started", millis());
    return ESP_OK;
}

static esp_err_t canLogStopHandler(httpd_req_t *req)
{
    canMonitor.setEnabled(false);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"monitor\":\"stopped\"}");
    logRing.push("[MONITOR] CAN log stopped", millis());
    return ESP_OK;
}

static esp_err_t canLogClearHandler(httpd_req_t *req)
{
    canMonitor.clear();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"monitor\":\"cleared\"}");
    logRing.push("[MONITOR] CAN log cleared", millis());
    return ESP_OK;
}

// Binary dump: streams the ring buffer as raw CanLogEntry structs.
// Each entry is 15 bytes: 4B timestamp + 2B can_id + 1B dlc + 8B data.
// Uses chunked transfer to avoid allocating a huge contiguous buffer.
static esp_err_t canLogDumpHandler(httpd_req_t *req)
{
    if (!canMonitor.isInited() || canMonitor.entryCount() == 0)
    {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"entries\":0}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition",
                       "attachment; filename=\"can_log.bin\"");

    // Stream entries in chunks of 256
    static constexpr uint32_t kChunkEntries = 256;
    uint32_t oldest = canMonitor.oldestIdx();
    uint32_t newest = canMonitor.head();

    for (uint32_t i = oldest; i < newest; i += kChunkEntries)
    {
        uint32_t batchEnd = (i + kChunkEntries < newest) ? (i + kChunkEntries) : newest;
        uint32_t batchSize = batchEnd - i;

        for (uint32_t j = i; j < batchEnd; j++)
        {
            const CanLogEntry *e = canMonitor.entryAt(j);
            if (e)
            {
                httpd_resp_send_chunk(req, reinterpret_cast<const char *>(e),
                                     sizeof(CanLogEntry));
            }
        }
    }

    // End chunked transfer
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ── Drive Data Recording — signal snapshot ring buffer + CSV export ──────────

struct DriveDataEntry
{
    uint32_t timestamp_ms;
    float speed_kph;
    float accel_pct;
    uint8_t gear;
    float steerTorque_Nm;
    int fusedLimit;
    int mapLimit;
    float bmsSoc;
    float bmsVoltage;
    float bmsCurrent;
    int8_t bmsTempMin;
    int8_t bmsTempMax;
};

static constexpr size_t kDriveDataMax = 4096; // ~80 KB in RAM
#if defined(BOARD_HAS_PSRAM)
static DriveDataEntry *driveDataBuf = nullptr;
#else
static constexpr size_t kDriveDataSmall = 256; // ~5 KB for non-PSRAM boards
static DriveDataEntry driveDataBufSmall[kDriveDataSmall];
static DriveDataEntry *driveDataBuf = driveDataBufSmall;
#endif
static volatile size_t driveDataHead = 0;
static volatile size_t driveDataCount = 0;
static volatile bool driveRecording = false;

static size_t driveDataCapacity()
{
#if defined(BOARD_HAS_PSRAM)
    return driveDataBuf ? kDriveDataMax : 0;
#else
    return kDriveDataSmall;
#endif
}

// Called from the CAN live polling path — not ISR safe, but only called from
// the web polling timer which runs on the main core.
static void driveDataRecord(const DecodedSignals &sig)
{
    if (!driveRecording || !driveDataBuf) return;
    size_t cap = driveDataCapacity();
    if (cap == 0) return;
    DriveDataEntry &e = driveDataBuf[driveDataHead % cap];
    e.timestamp_ms = millis();
    e.speed_kph    = sig.vehicleSpeed;
    e.accel_pct    = sig.di_accelPedal;
    e.gear         = sig.di_gear;
    e.steerTorque_Nm = sig.torsionBarTorque;
    e.fusedLimit   = sig.fusedSpeedLimit;
    e.mapLimit     = sig.mapSpeedLimit;
    e.bmsSoc       = sig.bmsSoc;
    e.bmsVoltage   = sig.bmsVoltage;
    e.bmsCurrent   = sig.bmsCurrent;
    e.bmsTempMin   = sig.bmsTempMin;
    e.bmsTempMax   = sig.bmsTempMax;
    driveDataHead++;
    if (driveDataCount < cap) driveDataCount++;
}

static esp_err_t driveDataToggleHandler(httpd_req_t *req)
{
    driveRecording = !driveRecording;
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"recording\":%s}", driveRecording ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

static esp_err_t driveDataClearHandler(httpd_req_t *req)
{
    driveDataHead = 0;
    driveDataCount = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

static esp_err_t driveDataStatusHandler(httpd_req_t *req)
{
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"recording\":%s,\"rows\":%u,\"max\":%u}",
             driveRecording ? "true" : "false",
             (unsigned)driveDataCount, (unsigned)driveDataCapacity());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

static esp_err_t driveDataCsvHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"drive_data.csv\"");

    const char *header = "timestamp_ms,speed_kph,accel_pct,gear,steer_Nm,fused_limit,map_limit,soc_pct,voltage_V,current_A,temp_min_C,temp_max_C\r\n";
    httpd_resp_send_chunk(req, header, strlen(header));

    size_t count = driveDataCount;
    size_t head  = driveDataHead;
    size_t cap   = driveDataCapacity();
    size_t start = (count < cap) ? 0 : (head % cap);

    char line[192];
    for (size_t i = 0; i < count; i++)
    {
        size_t idx = (start + i) % cap;
        const DriveDataEntry &e = driveDataBuf[idx];
        int len = snprintf(line, sizeof(line),
                           "%u,%.1f,%.1f,%u,%.2f,%d,%d,%.1f,%.1f,%.1f,%d,%d\r\n",
                           (unsigned)e.timestamp_ms, e.speed_kph, e.accel_pct,
                           (unsigned)e.gear, e.steerTorque_Nm,
                           e.fusedLimit, e.mapLimit,
                           e.bmsSoc, e.bmsVoltage, e.bmsCurrent,
                           (int)e.bmsTempMin, (int)e.bmsTempMax);
        httpd_resp_send_chunk(req, line, len);
    }

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// --- CAN Live signal API ---

static esp_err_t canLiveHandler(httpd_req_t *req)
{
    DecodedSignals sig;
    decodeSignals(canLive, sig);

    // Record drive data snapshot if recording is active
    driveDataRecord(sig);

    // Build JSON with snprintf — zero cJSON allocation overhead
    static constexpr size_t kBufSize = 8192;
    char *buf = (char *)malloc(kBufSize);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    size_t pos = 0;

#define CL_APPEND(fmt, ...) \
    do { \
        int _n = snprintf(buf + pos, kBufSize - pos, fmt, ##__VA_ARGS__); \
        if (_n > 0 && pos + (size_t)_n < kBufSize) pos += (size_t)_n; \
    } while (0)

    CL_APPEND("{\"ids_seen\":%u,\"uptime_ms\":%lu,\"signals\":{", (unsigned)canLive.slotCount(), (unsigned long)millis());

    // Decoded signals — inline
    CL_APPEND("\"vehicleSpeed_kph\":%.2f,\"uiSpeed\":%.1f,\"uiSpeedUnitsMph\":%s,",
              sig.vehicleSpeed, sig.uiSpeed, sig.uiSpeedUnitsMph ? "true" : "false");
    CL_APPEND("\"di_gear\":%u,\"di_accelPedal_pct\":%.1f,\"motorTorque_Nm\":%.1f,\"espBrakeTorque_Nm\":%.1f,\"gpsSpeed_kph\":%.2f,",
              sig.di_gear, sig.di_accelPedal, sig.motorTorque, sig.espBrakeTorque, sig.gpsSpeed);
    CL_APPEND("\"handsOnLevel\":%u,\"torsionBarTorque_Nm\":%.2f,", sig.handsOnLevel, sig.torsionBarTorque);
    CL_APPEND("\"dasSetSpeed_kph\":%.1f,\"dasAccState\":%u,\"accSpeedLimit_mph\":%.1f,",
              sig.dasSetSpeed, sig.dasAccState, sig.accSpeedLimit);
    CL_APPEND("\"fusedSpeedLimit_kph\":%d,\"visionSpeedLimit_kph\":%d,\"mapSpeedLimit_kph\":%d,",
              sig.fusedSpeedLimit, sig.visionSpeedLimit, sig.mapSpeedLimit);
    CL_APPEND("\"vehicleSpeedLimit_kph\":%d,\"mppSpeedLimit_kph\":%d,\"userSpeedOffset\":%d,\"userSpeedOffsetUnits\":\"%s\",",
              sig.vehicleSpeedLimit, sig.mppSpeedLimit, sig.userSpeedOffset, sig.userSpeedOffsetUnitsKph ? "KPH" : "MPH");
    CL_APPEND("\"followDistance\":%u,", sig.followDistance);
    CL_APPEND("\"fsdSelectedInUI\":%s,\"fsdEnabled\":%s,\"fsdHw4Lock\":%s,",
              sig.fsdSelectedInUI ? "true" : "false", sig.fsdEnabled ? "true" : "false", sig.fsdHw4Lock ? "true" : "false");
    CL_APPEND("\"speedProfileHw3\":%u,\"speedProfileHw4\":%u,\"smartSetSpeedOffset_raw\":%d,\"speedOffsetInjected\":%u,",
              sig.speedProfileHw3, sig.speedProfileHw4, sig.smartSetSpeedOffsetRaw, sig.speedOffsetInjected);
    CL_APPEND("\"suppressSpeedWarning\":%s,\"eceR79Nag\":%s,", sig.suppressSpeedWarning ? "true" : "false", sig.eceR79Nag ? "true" : "false");
    CL_APPEND("\"lightState\":%u,", sig.lightState);
    CL_APPEND("\"bmsVoltage_V\":%.2f,\"bmsCurrent_A\":%.1f,\"bmsSoc_pct\":%.1f,\"bmsTempMin_C\":%d,\"bmsTempMax_C\":%d,",
              sig.bmsVoltage, sig.bmsCurrent, sig.bmsSoc, (int)sig.bmsTempMin, (int)sig.bmsTempMax);
    CL_APPEND("\"otaInProgress\":%u}}", sig.otaInProgress);

#undef CL_APPEND

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, pos);
    free(buf);
    return ESP_OK;
}

// --- Public init function ---

static httpd_handle_t webServer = NULL;

static void webServerInit()
{
    // NVS settings were already loaded by appSetup() via nvsLoadAllSettings().
    // Calling it again here is harmless (idempotent) and protects against the
    // legacy build paths that don't go through the runtime-switch appSetup.
    nvsLoadAllSettings();

#if defined(BOARD_HAS_PSRAM)
    // Allocate drive data ring buffer in PSRAM
    driveDataBuf = (DriveDataEntry *)heap_caps_calloc(
        kDriveDataMax, sizeof(DriveDataEntry), MALLOC_CAP_SPIRAM);
    if (driveDataBuf)
        Serial.printf("Drive data: PSRAM buffer ready (%u entries)\n", (unsigned)kDriveDataMax);
    else
        Serial.println("Drive data: PSRAM alloc failed, recording disabled");
#endif

    // Restore manual speed profile if non-auto mode
    if (!profileModeAutoRuntime && appHandler)
    {
        uint8_t saved = nvsReadU8(kNvsKeyManualProfile, 1);
        if (saved > 4)
            saved = 4;
        appHandler->speedProfile = saved;
    }

    // WiFi: AP for direct configuration; optional STA to join an upstream router
#ifdef WIFI_STA_ENABLE
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false); // Disable modem sleep for AP stability
#ifdef WIFI_STA_HOSTNAME
    WiFi.setHostname(WIFI_STA_HOSTNAME);
#endif
    if (gStaSsid[0] != '\0')
    {
        IPAddress staIp(WIFI_STA_STATIC_IP);
        IPAddress staGw(WIFI_STA_GATEWAY);
        IPAddress staMask(WIFI_STA_SUBNET);
        IPAddress staDns(WIFI_STA_DNS);
        if (!WiFi.config(staIp, staGw, staMask, staDns))
        {
            Serial.println("WiFi STA: static IP config failed");
        }
        WiFi.setAutoReconnect(true);
        WiFi.begin(gStaSsid, gStaPass);
        Serial.printf("WiFi STA: connecting to \"%s\" with static IP ", gStaSsid);
        Serial.println(staIp);
        // Non-blocking probe: wait up to ~6s, then proceed regardless.
        // ESP32 Arduino auto-reconnects in the background.
        for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; ++i)
        {
            delay(200);
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("WiFi STA: connected, IP=");
            Serial.println(WiFi.localIP());
            char wbuf[LogRingBuffer::kMaxMsgLen];
            IPAddress ip = WiFi.localIP();
            snprintf(wbuf, sizeof(wbuf), "[BOOT] WiFi STA connected: %s @ %u.%u.%u.%u",
                     gStaSsid, ip[0], ip[1], ip[2], ip[3]);
            logRing.push(wbuf, millis());
        }
        else
        {
            Serial.println("WiFi STA: not connected yet, will keep retrying in background");
            char wbuf[LogRingBuffer::kMaxMsgLen];
            snprintf(wbuf, sizeof(wbuf), "[BOOT] WiFi STA: not connected to '%s', retrying", gStaSsid);
            logRing.push(wbuf, millis());
        }
    }
    else
    {
        Serial.println("WiFi STA: no SSID configured, AP only");
        logRing.push("[BOOT] WiFi STA: no SSID, AP only", millis());
    }
#else
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false); // Disable modem sleep for AP stability
#endif
    IPAddress apIp(192, 168, 4, 1);
    IPAddress apGw(192, 168, 4, 1);
    IPAddress apMask(255, 255, 255, 0);
    WiFi.softAPConfig(apIp, apGw, apMask);
    WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4); // ch 1, not hidden, max 4 clients
    delay(300); // let AP DHCP stabilize
    Serial.printf("WiFi AP \"%s\" started (pass: %s): ", AP_SSID, AP_PASS);
    Serial.println(WiFi.softAPIP());

    // DNS captive portal on Core 0
    xTaskCreatePinnedToCore(dnsTask, "dns", 4096, NULL, 2, NULL, 0);

    // HTTP server on Core 0
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    config.max_uri_handlers = 40;
    config.lru_purge_enable = true;
    config.stack_size = 8192;

    if (httpd_start(&webServer, &config) != ESP_OK)
    {
        Serial.println("HTTP: server start failed");
        return;
    }

    // Routes
    httpd_uri_t uriRoot = {
        .uri = "/", .method = HTTP_GET, .handler = rootHandler, .user_ctx = NULL};
    httpd_uri_t uriStatus = {
        .uri = "/api/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = NULL};
    httpd_uri_t uriBypassTlsscRequirement = {
        .uri = "/api/bypass-tlssc", .method = HTTP_POST, .handler = bypassTlsscRequirementHandler, .user_ctx = NULL};
    httpd_uri_t uriIsaSpeedChime = {
        .uri = "/api/isa-speed-chime-suppress", .method = HTTP_POST, .handler = isaSpeedChimeSuppressHandler, .user_ctx = NULL};
    httpd_uri_t uriEmergencyVehicleDetection = {
        .uri = "/api/emergency-vehicle-detection", .method = HTTP_POST, .handler = emergencyVehicleDetectionHandler, .user_ctx = NULL};
    httpd_uri_t uriEnhancedAutopilot = {
        .uri = "/api/enhanced-autopilot", .method = HTTP_POST, .handler = enhancedAutopilotHandler, .user_ctx = NULL};
    httpd_uri_t uriNagKiller = {
        .uri = "/api/nag-killer", .method = HTTP_POST, .handler = nagKillerHandler, .user_ctx = NULL};
    httpd_uri_t uriEnablePrint = {
        .uri = "/api/enable-print", .method = HTTP_POST, .handler = enablePrintHandler, .user_ctx = NULL};
    httpd_uri_t uriOta = {
        .uri = "/api/ota", .method = HTTP_POST, .handler = otaHandler, .user_ctx = NULL};
    httpd_uri_t uriChinaMode = {
        .uri = "/api/china-mode", .method = HTTP_POST, .handler = chinaModeHandler, .user_ctx = NULL};
    httpd_uri_t uriProfileModeAuto = {
        .uri = "/api/profile-mode-auto", .method = HTTP_POST, .handler = profileModeAutoHandler, .user_ctx = NULL};
    httpd_uri_t uriPreheat = {
        .uri = "/api/preheat", .method = HTTP_POST, .handler = preheatHandler, .user_ctx = NULL};
    httpd_uri_t uriPreheatStatus = {
        .uri = "/api/preheat/status", .method = HTTP_GET, .handler = preheatStatusHandler, .user_ctx = NULL};
    httpd_uri_t uriPreheatConfig = {
        .uri = "/api/preheat/config", .method = HTTP_POST, .handler = preheatConfigHandler, .user_ctx = NULL};
    httpd_uri_t uriHwMode = {
        .uri = "/api/hw-mode", .method = HTTP_POST, .handler = hwModeHandler, .user_ctx = NULL};
    httpd_uri_t uriSpeedProfile = {
        .uri = "/api/speed-profile", .method = HTTP_POST, .handler = speedProfileHandler, .user_ctx = NULL};
    httpd_uri_t uriSpeedOffset = {
        .uri = "/api/speed-offset", .method = HTTP_POST, .handler = speedOffsetHandler, .user_ctx = NULL};
    httpd_uri_t uriSmartOffset = {
        .uri = "/api/smart-offset", .method = HTTP_POST, .handler = smartOffsetHandler, .user_ctx = NULL};
    httpd_uri_t uriSmartOffsetGet = {
        .uri = "/api/smart-offset", .method = HTTP_GET, .handler = smartOffsetGetHandler, .user_ctx = NULL};
    httpd_uri_t uriWifi = {
        .uri = "/api/wifi", .method = HTTP_POST, .handler = wifiConfigHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLive = {
        .uri = "/api/can-live", .method = HTTP_GET, .handler = canLiveHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLogStatus = {
        .uri = "/api/can-log/status", .method = HTTP_GET, .handler = canLogStatusHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLogDump = {
        .uri = "/api/can-log/dump", .method = HTTP_GET, .handler = canLogDumpHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLogStart = {
        .uri = "/api/can-log/start", .method = HTTP_POST, .handler = canLogStartHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLogStop = {
        .uri = "/api/can-log/stop", .method = HTTP_POST, .handler = canLogStopHandler, .user_ctx = NULL};
    httpd_uri_t uriCanLogClear = {
        .uri = "/api/can-log/clear", .method = HTTP_POST, .handler = canLogClearHandler, .user_ctx = NULL};
    httpd_uri_t uriGenerate204 = {
        .uri = "/generate_204", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};
    httpd_uri_t uriHotspot = {
        .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};
    httpd_uri_t uriDriveDataToggle = {
        .uri = "/api/drive-data/toggle", .method = HTTP_POST, .handler = driveDataToggleHandler, .user_ctx = NULL};
    httpd_uri_t uriDriveDataClear = {
        .uri = "/api/drive-data/clear", .method = HTTP_POST, .handler = driveDataClearHandler, .user_ctx = NULL};
    httpd_uri_t uriDriveDataStatus = {
        .uri = "/api/drive-data/status", .method = HTTP_GET, .handler = driveDataStatusHandler, .user_ctx = NULL};
    httpd_uri_t uriDriveDataCsv = {
        .uri = "/api/drive-data/csv", .method = HTTP_GET, .handler = driveDataCsvHandler, .user_ctx = NULL};

    httpd_register_uri_handler(webServer, &uriRoot);
    httpd_register_uri_handler(webServer, &uriStatus);
    httpd_register_uri_handler(webServer, &uriBypassTlsscRequirement);
    httpd_register_uri_handler(webServer, &uriIsaSpeedChime);
    httpd_register_uri_handler(webServer, &uriEmergencyVehicleDetection);
    httpd_register_uri_handler(webServer, &uriEnhancedAutopilot);
    httpd_register_uri_handler(webServer, &uriNagKiller);
    httpd_register_uri_handler(webServer, &uriEnablePrint);
    httpd_register_uri_handler(webServer, &uriOta);
    httpd_register_uri_handler(webServer, &uriChinaMode);
    httpd_register_uri_handler(webServer, &uriProfileModeAuto);
    httpd_register_uri_handler(webServer, &uriPreheat);
    httpd_register_uri_handler(webServer, &uriPreheatStatus);
    httpd_register_uri_handler(webServer, &uriPreheatConfig);
    httpd_register_uri_handler(webServer, &uriHwMode);
    httpd_register_uri_handler(webServer, &uriSpeedProfile);
    httpd_register_uri_handler(webServer, &uriSpeedOffset);
    httpd_register_uri_handler(webServer, &uriSmartOffset);
    httpd_register_uri_handler(webServer, &uriSmartOffsetGet);
    httpd_register_uri_handler(webServer, &uriWifi);
    httpd_register_uri_handler(webServer, &uriCanLive);
    httpd_register_uri_handler(webServer, &uriCanLogStatus);
    httpd_register_uri_handler(webServer, &uriCanLogDump);
    httpd_register_uri_handler(webServer, &uriCanLogStart);
    httpd_register_uri_handler(webServer, &uriCanLogStop);
    httpd_register_uri_handler(webServer, &uriCanLogClear);
    httpd_register_uri_handler(webServer, &uriGenerate204);
    httpd_register_uri_handler(webServer, &uriHotspot);
    httpd_register_uri_handler(webServer, &uriDriveDataToggle);
    httpd_register_uri_handler(webServer, &uriDriveDataClear);
    httpd_register_uri_handler(webServer, &uriDriveDataStatus);
    httpd_register_uri_handler(webServer, &uriDriveDataCsv);

    Serial.println("Web dashboard ready at http://192.168.4.1/");
    logRing.push("[BOOT] Web dashboard ready, waiting for CAN frames...", millis());
}

#endif // DRIVER_TWAI && !NATIVE_BUILD
