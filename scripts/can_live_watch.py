#!/usr/bin/env python3
"""
Tesla CAN Live Watch — 实时监控所有 CAN 信号变化
================================================
每秒查询 ESP32 的 /api/can-live，检测并高亮显示变化的信号。
用于找出滚轮调速时哪些 CAN 值发生了变化。

用法:
    python3 scripts/can_live_watch.py              # 每秒刷新，只显示变化
    python3 scripts/can_live_watch.py --all         # 每秒刷新，显示所有值
    python3 scripts/can_live_watch.py --interval 2  # 每2秒刷新
    python3 scripts/can_live_watch.py --dump        # 单次输出所有当前值
"""

import os
import sys
import time
import json
import argparse
from datetime import datetime

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
NO_PROXY = {"http": None, "https": None}


def fetch():
    r = requests.get(f"{BASE_URL}/api/can-live", timeout=10, proxies=NO_PROXY)
    r.raise_for_status()
    return r.json()


def dump_once():
    """单次输出所有值"""
    data = fetch()

    print(f"\n{'='*70}")
    print(f"  Tesla CAN Live — {data['ids_seen']} 个 CAN ID 在线")
    print(f"{'='*70}")

    print(f"\n--- 解码信号 ---")
    for k, v in sorted(data["signals"].items()):
        print(f"  {k:<35} {v}")

    print(f"\n--- 原始帧 ({len(data['frames'])} 个 CAN ID) ---")
    print(f"  {'ID':>8}  {'DLC':>3}  {'数据':>18}  {'帧率':>7}  {'帧数':>8}")
    print(f"  {'-'*55}")
    for f in data["frames"]:
        print(f"  {f['id']:>8}  {f['dlc']:>3}  {f['data']:>18}  {f['hz']:>6.1f}/s  {f['count']:>8}")


def watch_changes(interval, show_all):
    """持续监控，检测变化"""
    print(f"实时监控模式，每 {interval} 秒刷新")
    print(f"模式: {'显示全部' if show_all else '仅显示变化'}")
    print(f"按 Ctrl+C 停止\n")

    prev_signals = {}
    prev_frames = {}
    first = True

    while True:
        try:
            data = fetch()
            now = datetime.now().strftime("%H:%M:%S")
            signals = data["signals"]
            frames = {f["id"]: f["data"] for f in data["frames"]}

            if first:
                # 第一次：记录基线
                prev_signals = dict(signals)
                prev_frames = dict(frames)
                first = False
                print(f"[{now}] 基线已记录: {data['ids_seen']} 个 CAN ID")
                if show_all:
                    print(f"\n  --- 信号 ---")
                    for k, v in sorted(signals.items()):
                        print(f"  {k:<35} {v}")
                    print(f"\n  --- 帧 ---")
                    for f in data["frames"]:
                        print(f"  {f['id']:>8} {f['data']}")
                print()
                time.sleep(interval)
                continue

            # 检测信号变化
            sig_changes = {}
            for k, v in signals.items():
                if k in prev_signals and prev_signals[k] != v:
                    sig_changes[k] = (prev_signals[k], v)

            # 检测帧数据变化
            frame_changes = {}
            for fid, fdata in frames.items():
                if fid in prev_frames and prev_frames[fid] != fdata:
                    frame_changes[fid] = (prev_frames[fid], fdata)
            # 新出现的帧
            new_frames = {fid: fdata for fid, fdata in frames.items() if fid not in prev_frames}

            has_changes = sig_changes or frame_changes or new_frames

            if has_changes:
                print(f"[{now}] *** 检测到变化 ***")

                if sig_changes:
                    print(f"  信号变化:")
                    for k, (old, new) in sorted(sig_changes.items()):
                        print(f"    {k:<35} {old} → {new}")

                if frame_changes:
                    print(f"  帧数据变化:")
                    for fid, (old, new) in sorted(frame_changes.items()):
                        # 高亮变化的字节
                        diff = ""
                        for j in range(0, min(len(old), len(new)), 2):
                            ob = old[j:j+2]
                            nb = new[j:j+2]
                            if ob != nb:
                                diff += f"[byte{j//2}:{ob}→{nb}] "
                        print(f"    {fid:>8}  {old} → {new}  {diff}")

                if new_frames:
                    print(f"  新发现帧:")
                    for fid, fdata in sorted(new_frames.items()):
                        print(f"    {fid:>8}  {fdata}")

                print()

            elif show_all:
                print(f"[{now}] 无变化 ({data['ids_seen']} IDs)")

            prev_signals = dict(signals)
            prev_frames = dict(frames)

        except requests.ConnectionError:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 连接失败，重试...")
        except requests.Timeout:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 超时，重试...")
        except Exception as e:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 错误: {e}")

        time.sleep(interval)


def main():
    parser = argparse.ArgumentParser(description="Tesla CAN Live Watch")
    parser.add_argument("--host", default="2.2.2.2", help="ESP32 IP")
    parser.add_argument("--interval", type=float, default=1.0, help="刷新间隔（秒）")
    parser.add_argument("--all", action="store_true", help="显示所有值（默认只显示变化）")
    parser.add_argument("--dump", action="store_true", help="单次输出所有当前值")
    args = parser.parse_args()

    global BASE_URL
    BASE_URL = f"http://{args.host}"

    try:
        if args.dump:
            dump_once()
        else:
            watch_changes(args.interval, args.all)
    except KeyboardInterrupt:
        print("\n停止")
    except requests.ConnectionError:
        print(f"连接失败: {BASE_URL}")
        sys.exit(1)


if __name__ == "__main__":
    main()
