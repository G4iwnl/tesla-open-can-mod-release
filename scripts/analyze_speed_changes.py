#!/usr/bin/env python3
"""
分析 CAN 日志中的限速相关变化：
- 用户速度偏移调整（滚轮操作）
- 道路限速识别变化
- DAS 目标速度变化
- ACC 速度上限变化
- 整车限速变化
- 地图限速变化
"""

import csv
import struct
import sys
from pathlib import Path
from datetime import timedelta


def ms_to_time(ms):
    """毫秒转为可读时间 (mm:ss.ms)"""
    s = ms / 1000.0
    m = int(s // 60)
    s = s % 60
    return f"{m:02d}:{s:06.3f}"


def parse_0x3D9(data_hex):
    """UI_gpsVehicleSpeed — 用户速度偏移"""
    d = bytes.fromhex(data_hex)
    if len(d) < 7:
        return None
    raw = d[5] & 0x3F
    offset = raw - 30
    units_bit = (d[5] >> 7) & 0x01
    units = "KPH" if units_bit else "MPH"
    mpp = (d[6] & 0x1F) * 5
    return {"userSpeedOffset": offset, "units": units, "mppSpeedLimit": mpp}


def parse_0x399(data_hex):
    """DAS_status — 融合限速"""
    d = bytes.fromhex(data_hex)
    if len(d) < 3:
        return None
    fused = (d[1] & 0x1F) * 5
    suppress = (d[1] >> 5) & 0x01
    vision = (d[2] & 0x1F) * 5
    return {"fusedSpeedLimit": fused, "suppress": suppress, "visionOnly": vision}


def parse_0x2B9(data_hex):
    """DAS_control — 目标速度"""
    d = bytes.fromhex(data_hex)
    if len(d) < 2:
        return None
    raw = ((d[1] & 0x0F) << 8) | d[0]
    speed = raw * 0.1
    return {"setSpeed": round(speed, 1)}


def parse_0x389(data_hex):
    """DAS_status2 — ACC 速度上限"""
    d = bytes.fromhex(data_hex)
    if len(d) < 2:
        return None
    raw = ((d[1] & 0x03) << 8) | d[0]
    limit = raw * 0.2
    return {"accSpeedLimit": round(limit, 1)}


def parse_0x334(data_hex):
    """UI_powertrainControl — 整车限速"""
    d = bytes.fromhex(data_hex)
    if len(d) < 3:
        return None
    raw = d[2]
    if raw == 255:
        return {"speedLimit": "SNA"}
    return {"speedLimit": raw + 50}


def parse_0x238(data_hex):
    """UI_driverAssistMapData — 地图限速"""
    d = bytes.fromhex(data_hex)
    if len(d) < 2:
        return None
    raw = d[1] & 0x1F
    limit_map = {
        0: "?", 1: 5, 2: 7, 3: 10, 4: 15, 5: 20, 6: 25,
        7: 30, 8: 35, 9: 40, 10: 45, 11: 50, 12: 55,
        13: 60, 14: 65, 15: 70, 16: 75, 17: 80, 18: 85,
        19: 90, 20: 95, 21: 100, 22: 105, 23: 110, 24: 115,
        25: 120, 26: 130, 27: 140, 28: 150, 29: 160,
        30: "UNLIMITED", 31: "SNA",
    }
    return {"mapSpeedLimit": limit_map.get(raw, raw)}


def parse_0x3FD(data_hex):
    """UI_autopilotControl — FSD 控制帧"""
    d = bytes.fromhex(data_hex)
    if len(d) < 8:
        return None
    mux = d[0] & 0x07
    if mux == 0:
        fsd_ui = (d[4] >> 6) & 0x01
        raw_offset = (d[3] >> 1) & 0x3F
        profile_hw3 = (d[6] >> 1) & 0x03
        profile_hw4 = (d[7] >> 4) & 0x07
        bit46 = (d[5] >> 6) & 0x01
        return {
            "mux": 0, "fsd_ui": fsd_ui,
            "smartSetSpeedOffset_raw": raw_offset,
            "profile_hw3": profile_hw3, "profile_hw4": profile_hw4,
            "fsd_enabled": bit46,
        }
    elif mux == 2:
        so = ((d[0] >> 6) & 0x03) | ((d[1] & 0x3F) << 2)
        profile_hw4 = (d[7] >> 4) & 0x07
        return {"mux": 2, "speed_offset_injected": so, "profile_hw4": profile_hw4}
    return {"mux": mux}


PARSERS = {
    "0x3D9": ("用户速度偏移 (滚轮)", parse_0x3D9),
    "0x399": ("道路限速识别", parse_0x399),
    "0x2B9": ("DAS 目标速度", parse_0x2B9),
    "0x389": ("ACC 速度上限", parse_0x389),
    "0x334": ("整车限速", parse_0x334),
    "0x238": ("地图限速", parse_0x238),
    "0x3FD": ("FSD 控制帧", parse_0x3FD),
}


def analyze_file(csv_path):
    print(f"\n{'=' * 80}")
    print(f"分析文件: {csv_path.name}")
    print(f"{'=' * 80}")

    # Track previous values to detect changes
    prev = {}
    changes = []
    first_ts = None
    last_ts = None
    frame_count = 0

    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            frame_count += 1
            ts = int(row["timestamp_ms"])
            cid = row["can_id_hex"]
            data_hex = row["data_hex"]

            if first_ts is None:
                first_ts = ts
            last_ts = ts

            if cid not in PARSERS:
                continue

            label, parser = PARSERS[cid]
            try:
                parsed = parser(data_hex)
            except Exception:
                continue
            if parsed is None:
                continue

            # For 0x3FD, use mux as sub-key
            if cid == "0x3FD":
                mux = parsed.get("mux")
                key = f"{cid}_mux{mux}"
            else:
                key = cid

            # Detect changes
            if key in prev:
                old = prev[key]
                diffs = {}
                for k, v in parsed.items():
                    if k in old and old[k] != v:
                        diffs[k] = (old[k], v)
                if diffs:
                    changes.append((ts, cid, label, diffs))
            prev[key] = parsed

    if first_ts is None:
        print("  文件为空")
        return

    duration = (last_ts - first_ts) / 1000.0
    print(f"  总帧数: {frame_count}")
    print(f"  时间跨度: {duration:.1f} 秒 ({ms_to_time(first_ts - first_ts)} ~ {ms_to_time(last_ts - first_ts)})")
    print(f"  检测到 {len(changes)} 次变化\n")

    if not changes:
        print("  没有检测到任何限速/偏移变化")
        return

    # Group changes by signal type
    print(f"{'时间':>12} {'CAN ID':>8}  {'信号':20}  {'变化详情'}")
    print("-" * 90)

    for ts, cid, label, diffs in changes:
        rel_time = ms_to_time(ts - first_ts)
        for field, (old_val, new_val) in diffs.items():
            # Skip counter/noise fields
            if field in ("mux",):
                continue
            print(f"  {rel_time:>10} {cid:>8}  {field:<20}  {old_val} → {new_val}")

    # Summary of key signals
    print(f"\n{'─' * 80}")
    print("重点变化汇总:")
    print(f"{'─' * 80}")

    key_fields = {
        "userSpeedOffset": "用户速度偏移（滚轮调整）",
        "fusedSpeedLimit": "融合限速（视觉+地图）",
        "mapSpeedLimit": "地图限速",
        "setSpeed": "DAS 目标速度",
        "accSpeedLimit": "ACC 速度上限",
        "speedLimit": "整车限速",
        "fsd_enabled": "FSD 启用",
        "fsd_ui": "FSD UI 选中",
        "profile_hw4": "速度档位 (HW4)",
        "suppress": "限速警告抑制",
        "smartSetSpeedOffset_raw": "智能速度偏移 (raw)",
    }

    for field, desc in key_fields.items():
        field_changes = [
            (ts, diffs[field])
            for ts, cid, label, diffs in changes
            if field in diffs
        ]
        if field_changes:
            print(f"\n  [{desc}] 共 {len(field_changes)} 次变化:")
            for ts, (old_val, new_val) in field_changes:
                rel_time = ms_to_time(ts - first_ts)
                print(f"    {rel_time}  {old_val} → {new_val}")


def main():
    import glob

    search_dir = Path(__file__).resolve().parent.parent
    csv_files = sorted(search_dir.glob("can_log_*.csv"))

    if not csv_files:
        print("没有找到 can_log_*.csv 文件")
        sys.exit(1)

    print(f"找到 {len(csv_files)} 个日志文件\n")

    for f in csv_files:
        size = f.stat().st_size
        if size < 500:
            print(f"跳过 {f.name} (太小: {size}B)")
            continue
        analyze_file(f)


if __name__ == "__main__":
    main()
