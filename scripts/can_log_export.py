#!/usr/bin/env python3
"""
Tesla CAN Log Export Tool
=========================
Connects to the ESP32 CAN Monitor API, downloads binary CAN log data,
decodes known FSD signals, and exports to CSV.

Usage:
    python3 can_log_export.py                    # One-shot dump
    python3 can_log_export.py --poll 30          # Poll every 30s
    python3 can_log_export.py --start            # Start monitoring
    python3 can_log_export.py --stop             # Stop monitoring
    python3 can_log_export.py --status           # Show buffer status
    python3 can_log_export.py --clear            # Clear buffer

Output: can_log_YYYYMMDD_HHMMSS.csv
"""

import argparse
import struct
import sys
import time
from datetime import datetime
from pathlib import Path

import os
# 绕过 proxychains / 系统代理，确保直连局域网设备
os.environ.pop("http_proxy", None)
os.environ.pop("https_proxy", None)
os.environ.pop("HTTP_PROXY", None)
os.environ.pop("HTTPS_PROXY", None)
os.environ.pop("all_proxy", None)
os.environ.pop("ALL_PROXY", None)
os.environ["no_proxy"] = "2.2.2.0/24,2.2.2.2,localhost,127.0.0.1"
os.environ["NO_PROXY"] = os.environ["no_proxy"]

try:
    import requests
except ImportError:
    print("需要 requests 库: pip install requests")
    sys.exit(1)

BASE_URL = "http://2.2.2.2"
ENTRY_SIZE = 15  # 4B ts + 2B id + 1B dlc + 8B data
ENTRY_FORMAT = "<IHB8s"  # little-endian: uint32, uint16, uint8, 8 bytes

# ── Signal decoders ──────────────────────────────────────────────────────────

def decode_0x3FD(data, dlc):
    """UI_autopilotControl — FSD main frame (mux 0/1/2)"""
    mux = data[0] & 0x07
    parts = [f"mux={mux}"]
    if mux == 0:
        fsd_ui = (data[4] >> 6) & 0x01
        parts.append(f"fsd_ui_selected={fsd_ui}")
        # speed offset (HW3): byte3 bit1-6
        raw_offset = (data[3] >> 1) & 0x3F
        offset = max(min((raw_offset - 30) * 5, 100), 0)
        parts.append(f"raw_speed_offset={raw_offset} calc={offset}")
        # speed profile (HW3): byte6 bit1-2
        profile_hw3 = (data[6] >> 1) & 0x03
        parts.append(f"profile_hw3={profile_hw3}")
        # bit 46 (FSD enable)
        bit46 = (data[5] >> 6) & 0x01
        parts.append(f"bit46_fsd={bit46}")
        # bit 60 (HW4 FSD)
        bit60 = (data[7] >> 4) & 0x01
        parts.append(f"bit60_hw4={bit60}")
    elif mux == 1:
        bit19 = (data[2] >> 3) & 0x01
        bit47 = (data[5] >> 7) & 0x01
        parts.append(f"bit19_eceR79={bit19} bit47_hw4nag={bit47}")
    elif mux == 2:
        # HW3 speed offset: byte0 bit6-7 + byte1 bit0-5
        so = ((data[0] >> 6) & 0x03) | ((data[1] & 0x3F) << 2)
        parts.append(f"speed_offset_injected={so}")
        # HW4 speed profile: byte7 bit4-6
        profile_hw4 = (data[7] >> 4) & 0x07
        parts.append(f"profile_hw4={profile_hw4}")
    return " ".join(parts)


def decode_0x399(data, dlc):
    """DAS_status — ISA + speed limits"""
    parts = []
    fused = (data[1]) & 0x1F
    parts.append(f"fusedSpeedLimit={fused * 5}kph")
    suppress = (data[1] >> 5) & 0x01
    parts.append(f"isa_suppress={suppress}")
    vision = (data[2]) & 0x1F
    parts.append(f"visionOnlyLimit={vision * 5}kph")
    return " ".join(parts)


def decode_0x3D9(data, dlc):
    """UI_gpsVehicleSpeed — user speed offset"""
    parts = []
    raw = data[5] & 0x3F
    offset = raw - 30
    parts.append(f"userSpeedOffset={offset:+d}kph/mph")
    units = (data[5] >> 7) & 0x01
    parts.append(f"units={'KPH' if units else 'MPH'}")
    mpp = (data[6]) & 0x1F
    parts.append(f"mppSpeedLimit={mpp * 5}kph")
    return " ".join(parts)


def decode_0x2B9(data, dlc):
    """DAS_control — DAS set speed"""
    raw = ((data[1] & 0x0F) << 8) | data[0]
    speed = raw * 0.1
    return f"setSpeed={speed:.1f}kph"


def decode_0x389(data, dlc):
    """DAS_status2 — ACC speed limit"""
    raw = ((data[1] & 0x03) << 8) | data[0]
    limit = raw * 0.2
    return f"accSpeedLimit={limit:.1f}mph"


def decode_0x3F8(data, dlc):
    """UI_driverAssistControl — follow distance"""
    fd = (data[5] & 0xE0) >> 5
    return f"followDistance={fd}"


def decode_0x238(data, dlc):
    """UI_driverAssistMapData — map speed limit"""
    raw = (data[1]) & 0x1F
    limit_names = {
        0: "UNKNOWN", 30: "UNLIMITED", 31: "SNA",
    }
    # For values 1-29, approximate: 5 * (raw + some offset)
    # Actually it's an enum: 1=5, 2=7, 3=10, 4=15, 5=20 ... 28=150, 29=160
    limit_map = {
        0: "?", 1: "5", 2: "7", 3: "10", 4: "15", 5: "20", 6: "25",
        7: "30", 8: "35", 9: "40", 10: "45", 11: "50", 12: "55",
        13: "60", 14: "65", 15: "70", 16: "75", 17: "80", 18: "85",
        19: "90", 20: "95", 21: "100", 22: "105", 23: "110", 24: "115",
        25: "120", 26: "130", 27: "140", 28: "150", 29: "160",
        30: "UNLIMITED", 31: "SNA",
    }
    return f"mapSpeedLimit={limit_map.get(raw, raw)}kph"


def decode_0x334(data, dlc):
    """UI_powertrainControl — vehicle speed limit"""
    raw = data[2]
    limit = raw + 50
    return f"speedLimit={limit}kph" if raw != 255 else "speedLimit=SNA"


def decode_0x370(data, dlc):
    """EPAS3P_sysStatus — nag status"""
    hands = (data[4] >> 6) & 0x03
    counter = data[6] & 0x0F
    return f"handsOnLevel={hands} counter={counter}"


def decode_0x132(data, dlc):
    """BMS_hvBusStatus"""
    v = ((data[1] << 8) | data[0]) * 0.01
    i = struct.unpack_from("<h", bytes(data), 2)[0] * 0.1
    return f"voltage={v:.1f}V current={i:.1f}A power={v * i:.0f}W"


def decode_0x292(data, dlc):
    """BMS_socStatus"""
    raw = ((data[1] & 0x03) << 8) | data[0]
    soc = raw * 0.1
    return f"soc={soc:.1f}%"


def decode_0x312(data, dlc):
    """BMS_thermalStatus"""
    if dlc >= 6:
        tmin = data[4] - 40
        tmax = data[5] - 40
        return f"battTempMin={tmin}C battTempMax={tmax}C"
    return ""


def decode_0x398(data, dlc):
    """GTW_carConfig — HW version"""
    das_hw = (data[0] >> 6) & 0x03
    hw_names = {0: "Unknown", 1: "Legacy?", 2: "HW3", 3: "HW4"}
    return f"hwVersion={hw_names.get(das_hw, das_hw)}"


def decode_0x318(data, dlc):
    """GTW_carState — OTA status"""
    ota = data[6] & 0x03
    return f"otaInProgress={ota}"


def decode_0x286(data, dlc):
    """DI_locStatus — cruise set speed"""
    # bit 15, 9 bits
    raw = ((data[2] & 0x01) << 8) | data[1]
    speed = raw * 0.5
    return f"cruiseSetSpeed={speed:.1f}"


def decode_0x3EE(data, dlc):
    """DAS_autopilot — Legacy"""
    mux = data[0] & 0x07
    parts = [f"mux={mux}"]
    if mux == 0:
        fsd_ui = (data[4] >> 6) & 0x01
        bit46 = (data[5] >> 6) & 0x01
        parts.append(f"fsd_ui={fsd_ui} bit46={bit46}")
    return " ".join(parts)


DECODERS = {
    0x3FD: decode_0x3FD,
    0x399: decode_0x399,
    0x3D9: decode_0x3D9,
    0x2B9: decode_0x2B9,
    0x389: decode_0x389,
    0x3F8: decode_0x3F8,
    0x238: decode_0x238,
    0x334: decode_0x334,
    0x370: decode_0x370,
    0x132: decode_0x132,
    0x292: decode_0x292,
    0x312: decode_0x312,
    0x398: decode_0x398,
    0x318: decode_0x318,
    0x286: decode_0x286,
    0x3EE: decode_0x3EE,
}

CAN_NAMES = {
    0x045: "STW_ACTN_RQ",
    0x082: "UI_tripPlanning",
    0x0D5: "UI_cruiseControl",
    0x118: "DI_systemStatus",
    0x132: "BMS_hvBusStatus",
    0x238: "UI_driverAssistMapData",
    0x286: "DI_locStatus",
    0x2B9: "DAS_control",
    0x292: "BMS_socStatus",
    0x312: "BMS_thermalStatus",
    0x313: "UI_trackModeSettings",
    0x318: "GTW_carState",
    0x334: "UI_powertrainControl",
    0x370: "EPAS3P_sysStatus",
    0x389: "DAS_status2",
    0x398: "GTW_carConfig",
    0x399: "DAS_status",
    0x3D9: "UI_gpsVehicleSpeed",
    0x3EE: "DAS_autopilot",
    0x3F8: "UI_driverAssistControl",
    0x3FD: "UI_autopilotControl",
}


# ── API helpers ──────────────────────────────────────────────────────────────

NO_PROXY = {"http": None, "https": None}

def api_get(path, retries=3):
    for attempt in range(retries):
        try:
            r = requests.get(f"{BASE_URL}{path}", timeout=15, proxies=NO_PROXY)
            r.raise_for_status()
            return r
        except (requests.ConnectionError, requests.Timeout):
            if attempt < retries - 1:
                time.sleep(2)
            else:
                raise


def api_post(path, retries=3):
    for attempt in range(retries):
        try:
            r = requests.post(f"{BASE_URL}{path}", timeout=15, proxies=NO_PROXY)
            r.raise_for_status()
            return r
        except (requests.ConnectionError, requests.Timeout):
            if attempt < retries - 1:
                time.sleep(2)
            else:
                raise


def show_status():
    r = api_get("/api/can-log/status")
    info = r.json()
    print(f"Monitor 初始化: {'OK' if info['inited'] else 'FAILED'}")
    print(f"Monitor 状态:   {'运行中' if info['enabled'] else '已停止'}")
    print(f"已记录条数:     {info['entries']}")
    print(f"缓冲容量:       {info['capacity']}")
    print(f"填充率:         {info['fill_pct']:.1f}%")
    print(f"监控 CAN ID 数: {info['watch_ids']}")


def start_monitor():
    api_post("/api/can-log/start")
    print("Monitor 已启动")


def stop_monitor():
    api_post("/api/can-log/stop")
    print("Monitor 已停止")


def clear_buffer():
    api_post("/api/can-log/clear")
    print("缓冲区已清空")


def dump_and_save(output_path: Path):
    """Download binary dump and convert to CSV."""
    # Check status first
    status = api_get("/api/can-log/status").json()
    count = status["entries"]
    if count == 0:
        print("缓冲区为空，无数据可导出")
        return 0

    print(f"下载 {count} 条记录...")
    r = api_get("/api/can-log/dump")

    if r.headers.get("content-type", "").startswith("application/json"):
        print("缓冲区为空")
        return 0

    raw = r.content
    num_entries = len(raw) // ENTRY_SIZE
    if num_entries == 0:
        print("收到空数据")
        return 0

    print(f"收到 {len(raw)} 字节 = {num_entries} 条记录")

    # Also save raw binary
    bin_path = output_path.with_suffix(".bin")
    bin_path.write_bytes(raw)
    print(f"二进制原始数据: {bin_path}")

    # Parse and write CSV
    with open(output_path, "w") as f:
        f.write("timestamp_ms,can_id_hex,can_name,dlc,data_hex,decoded\n")

        for i in range(num_entries):
            offset = i * ENTRY_SIZE
            ts, can_id, dlc = struct.unpack_from("<IHB", raw, offset)
            data = raw[offset + 7 : offset + 15]

            data_hex = data[:dlc].hex().upper()
            name = CAN_NAMES.get(can_id, f"0x{can_id:03X}")

            # Decode known signals
            decoder = DECODERS.get(can_id)
            decoded = ""
            if decoder:
                try:
                    decoded = decoder(list(data), dlc)
                except Exception:
                    decoded = "DECODE_ERROR"

            f.write(f"{ts},0x{can_id:03X},{name},{dlc},{data_hex},{decoded}\n")

    print(f"CSV 导出完成: {output_path} ({num_entries} 条)")
    return num_entries


def poll_loop(interval_s, output_dir: Path):
    """Continuously poll and append to CSV files."""
    print(f"持续导出模式，每 {interval_s} 秒轮询一次")
    print(f"输出目录: {output_dir}")
    print("按 Ctrl+C 停止\n")

    # Start monitor if not running
    status = api_get("/api/can-log/status").json()
    if not status["enabled"]:
        start_monitor()

    session_ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = output_dir / f"can_log_{session_ts}.csv"
    total = 0

    # Write header
    with open(csv_path, "w") as f:
        f.write("timestamp_ms,can_id_hex,can_name,dlc,data_hex,decoded\n")

    fail_count = 0
    try:
        while True:
            try:
                status = api_get("/api/can-log/status").json()
                count = status["entries"]

                if count > 0:
                    r = api_get("/api/can-log/dump")
                    if not r.headers.get("content-type", "").startswith("application/json"):
                        raw = r.content
                        num = len(raw) // ENTRY_SIZE

                        with open(csv_path, "a") as f:
                            for i in range(num):
                                off = i * ENTRY_SIZE
                                ts, can_id, dlc = struct.unpack_from("<IHB", raw, off)
                                data = raw[off + 7 : off + 15]
                                data_hex = data[:dlc].hex().upper()
                                name = CAN_NAMES.get(can_id, f"0x{can_id:03X}")
                                decoder = DECODERS.get(can_id)
                                decoded = ""
                                if decoder:
                                    try:
                                        decoded = decoder(list(data), dlc)
                                    except Exception:
                                        decoded = ""
                                f.write(f"{ts},0x{can_id:03X},{name},{dlc},{data_hex},{decoded}\n")

                        total += num
                        print(f"[{datetime.now().strftime('%H:%M:%S')}] +{num} 条 (总计 {total}), "
                              f"缓冲 {status['fill_pct']:.1f}%")

                        # Clear buffer after successful download
                        api_post("/api/can-log/clear")

                fail_count = 0

            except (requests.ConnectionError, requests.Timeout) as e:
                fail_count += 1
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 连接失败 ({fail_count}次), 等待重试...")
                if fail_count > 10:
                    print("连续失败超过 10 次，退出")
                    break

            time.sleep(interval_s)

    except KeyboardInterrupt:
        print(f"\n停止。总计导出 {total} 条到 {csv_path}")


def main():
    parser = argparse.ArgumentParser(description="Tesla CAN Log Export Tool")
    parser.add_argument("--host", default="2.2.2.2", help="ESP32 IP (default: 2.2.2.2)")
    parser.add_argument("--status", action="store_true", help="显示缓冲状态")
    parser.add_argument("--start", action="store_true", help="启动监控")
    parser.add_argument("--stop", action="store_true", help="停止监控")
    parser.add_argument("--clear", action="store_true", help="清空缓冲")
    parser.add_argument("--poll", type=int, metavar="SEC", help="持续轮询间隔（秒）")
    parser.add_argument("-o", "--output", default=".", help="输出目录")
    args = parser.parse_args()

    global BASE_URL
    BASE_URL = f"http://{args.host}"

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    try:
        if args.status:
            show_status()
        elif args.start:
            start_monitor()
        elif args.stop:
            stop_monitor()
        elif args.clear:
            clear_buffer()
        elif args.poll:
            poll_loop(args.poll, output_dir)
        else:
            # One-shot dump
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            csv_path = output_dir / f"can_log_{ts}.csv"
            dump_and_save(csv_path)
    except requests.ConnectionError:
        print(f"连接失败: {BASE_URL}")
        print("请确认：1) ESP32 已开机  2) WiFi 已连接  3) IP 地址正确")
        sys.exit(1)
    except requests.Timeout:
        print(f"请求超时: {BASE_URL}")
        sys.exit(1)


if __name__ == "__main__":
    main()
