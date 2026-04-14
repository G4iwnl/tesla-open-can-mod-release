#!/bin/bash
# Tesla CAN Mod - ESP32-S3 自动烧录脚本 (Mac/Linux)
#
# 一键完成:
#   1. 自动检测 ESP32-S3 USB CDC 串口
#   2. 激活 PlatformIO venv
#   3. pio run -e waveshare_esp32s3_rs485_can -t upload (编译 + 烧录)
#   4. 烧录后等 WiFi STA 上线
#   5. 通过新 IP 验证 web + CAN driver 状态
#
# 用法:
#   sh flash_esp32.sh                              # 全自动
#   sh flash_esp32.sh --port /dev/cu.usbmodem11201 # 指定串口
#   sh flash_esp32.sh --no-verify                  # 跳过烧录后验证
#   sh flash_esp32.sh --ip 2.2.2.2                 # 指定验证 IP (默认从 platformio.ini 读)
#
# 前置:
#   - PlatformIO 已安装 (venv 路径通过 --venv 参数或 VENV_PATH 环境变量指定)
#   - 板子用 USB-C 数据线 (非充电线) 接 Mac
#   - 路由器侧已经部署好 (sh router-config/tesla-iso/deploy.sh)
#     这样烧录后板子能立即连上 tesla wifi 拿到 2.2.2.2 静态 IP

set -e

# ============ 默认参数 ============
ENV_NAME="waveshare_esp32s3_rs485_can"
PORT=""
VERIFY=1
EXPECT_IP="2.2.2.2"
VENV_PATH="${VENV_PATH:-$HOME/.platformio/penv}"

# ============ 解析参数 ============
while [ $# -gt 0 ]; do
  case "$1" in
    --port)        PORT="$2"; shift 2 ;;
    --env)         ENV_NAME="$2"; shift 2 ;;
    --ip)          EXPECT_IP="$2"; shift 2 ;;
    --no-verify)   VERIFY=0; shift ;;
    --venv)        VENV_PATH="$2"; shift 2 ;;
    -h|--help)
      grep '^#' "$0" | head -25 | sed 's/^# \?//'
      exit 0
      ;;
    *) echo "未知参数: $1"; exit 1 ;;
  esac
done

# ============ proxychains 兼容 ============
CURL="env DYLD_INSERT_LIBRARIES= curl"
PIO="env DYLD_INSERT_LIBRARIES= PYTHONUNBUFFERED=1 pio"
LS="env DYLD_INSERT_LIBRARIES= ls"

# ============ 自动定位项目根 ============
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -z "$SCRIPT_DIR" ] && SCRIPT_DIR="$(pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "platformio.ini" ]; then
  echo "❌ ERROR: 未在项目根目录, 找不到 platformio.ini"
  exit 1
fi

# ============ 激活 PlatformIO venv ============
if [ ! -f "$VENV_PATH/bin/activate" ]; then
  echo "❌ ERROR: PlatformIO venv 不存在: $VENV_PATH"
  echo "  用 --venv /path/to/venv 指定, 或先创建 venv 装 platformio"
  exit 1
fi
# shellcheck disable=SC1090
. "$VENV_PATH/bin/activate"

if ! command -v pio >/dev/null 2>&1; then
  echo "❌ ERROR: 激活 venv 后仍找不到 pio 命令"
  exit 1
fi

# ============ 自动检测串口 ============
if [ -z "$PORT" ]; then
  # ESP32-S3 走内置 USB CDC, Mac 上叫 /dev/cu.usbmodem*
  PORT=$($LS /dev/cu.usbmodem* 2>/dev/null | head -1)
  if [ -z "$PORT" ]; then
    # Linux 通常叫 /dev/ttyACM*
    PORT=$($LS /dev/ttyACM* 2>/dev/null | head -1)
  fi
fi

if [ -z "$PORT" ]; then
  echo "❌ ERROR: 没检测到 USB CDC 串口"
  echo "  - 检查 USB-C 线是否插好 (必须是数据线, 不是充电线)"
  echo "  - 检查 Mac/Linux 是否识别到设备:"
  echo "      ls /dev/cu.usbmodem* /dev/ttyACM*"
  echo "  - 或用 --port /dev/cu.usbmodemXXXX 手动指定"
  exit 1
fi

# ============ 读取 platformio.ini 里的 STA IP 默认值 ============
# 形如: -DWIFI_STA_STATIC_IP=2,2,2,2 -> 2.2.2.2
if [ -z "$EXPECT_IP" ] || [ "$EXPECT_IP" = "auto" ]; then
  EXPECT_IP=$(grep -E '^\s*-DWIFI_STA_STATIC_IP=' platformio.ini 2>/dev/null \
    | head -1 | sed -E 's/.*=([0-9]+),([0-9]+),([0-9]+),([0-9]+).*/\1.\2.\3.\4/')
fi

cat <<INFO
==================================================
  Tesla CAN Mod - ESP32 烧录
==================================================
  Project   : $SCRIPT_DIR
  Env       : $ENV_NAME
  Port      : $PORT
  STA IP    : $EXPECT_IP (烧后验证)
  Verify    : $VERIFY
==================================================

INFO

# ============ Step 1: 编译 + 烧录 ============
echo ">>> [1/3] 编译 + 烧录 (pio run -e $ENV_NAME -t upload)..."
$PIO run -e "$ENV_NAME" -t upload --upload-port "$PORT" 2>&1 | tail -25
RC=${PIPESTATUS[0]:-$?}
if [ "$RC" != "0" ]; then
  echo ""
  echo "❌ 烧录失败 (exit=$RC)"
  echo "  - 串口被占用? 关闭 pio device monitor / 串口工具"
  echo "  - 板子在 download 模式? 按 RST 一次"
  echo "  - 看完整日志: pio run -e $ENV_NAME -t upload --upload-port $PORT"
  exit $RC
fi
echo ""

if [ "$VERIFY" = "0" ]; then
  echo "✅ 烧录完成 (跳过验证)"
  exit 0
fi

# ============ Step 2: 等 WiFi STA 上线 ============
echo ">>> [2/3] 等待板子重启 + WiFi STA 上线 ($EXPECT_IP)..."
ATTEMPTS=0
MAX_ATTEMPTS=20  # 20 秒
while [ $ATTEMPTS -lt $MAX_ATTEMPTS ]; do
  CODE=$($CURL -sS --max-time 2 -o /dev/null -w "%{http_code}" "http://$EXPECT_IP/api/status" 2>/dev/null || echo "000")
  if [ "$CODE" = "200" ]; then
    echo "  ✓ 板子上线 (http://$EXPECT_IP/api/status → 200, 用时 ${ATTEMPTS}s)"
    break
  fi
  sleep 1
  ATTEMPTS=$((ATTEMPTS + 1))
done

if [ $ATTEMPTS -ge $MAX_ATTEMPTS ]; then
  echo "  ⚠ 20 秒内 $EXPECT_IP 未响应"
  echo "    可能原因:"
  echo "    1. 路由器侧 secondary IP 2.2.2.1/24 没生效"
  echo "       → telnet 192.168.2.254 后跑 'ip addr show br-lan' 检查"
  echo "    2. 板子 WiFi 没连上 (SSID/密码不对?)"
  echo "       → pio device monitor --port $PORT -b 115200 看串口日志"
  echo "    3. Mac 没有 2.2.2.0/24 路由"
  echo "       → 默认应该走 192.168.2.254 default gateway"
  exit 1
fi
echo ""

# ============ Step 3: 验证关键状态 ============
echo ">>> [3/3] 验证 status / CAN 健康..."
JSON=$($CURL -sS --max-time 5 "http://$EXPECT_IP/api/status" 2>/dev/null)
if [ -z "$JSON" ]; then
  echo "  ⚠ status JSON 拿不到"
  exit 1
fi

# Python 解析关键字段
echo "$JSON" | python3 -c '
import sys, json
d = json.load(sys.stdin)
c = d.get("can", {})
print(f"  hw_mode      = {d.get(\"hw_mode\")} ({[\"LEGACY\",\"HW3\",\"HW4\"][d.get(\"hw_mode\",2)]})")
print(f"  can.state    = {c.get(\"state\")}")
print(f"  sta_ip       = {d.get(\"sta_ip\")}")
print(f"  sta_ssid     = {d.get(\"sta_ssid\")}")
print(f"  version      = {d.get(\"version\")}")
print(f"  uptime_s     = {d.get(\"uptime_s\")}")
print(f"  preheat      = {d.get(\"preheat\")}")
print(f"  tx_errors    = {c.get(\"tx_errors\")}")
print(f"  bus_errors   = {c.get(\"bus_errors\")}")

# 关键健康检查
ok = True
if c.get("state") != "RUNNING":
    print(f"  ✗ CAN state 不是 RUNNING")
    ok = False
if d.get("sta_ip") != "'$EXPECT_IP'":
    print(f"  ✗ sta_ip 跟期望不符 (期望 '$EXPECT_IP')")
    ok = False
print()
if ok:
    print("  ✅ 全部 OK")
else:
    print("  ⚠ 有异常")
    sys.exit(2)
'
RC=$?

cat <<DONE

==================================================
  ✅ ESP32 烧录 + 验证完成
==================================================
  访问 (Mac/手机): http://192.168.2.253/   ⚠ 老 IP 已废弃
                  http://$EXPECT_IP/        ⭐ 新 IP
  访问 (车机):    http://2.2.2.1:8888/     (Tesla ISO)
                  http://2.2.2.2/          (CAN Mod 模块, = $EXPECT_IP)
DONE

exit $RC
