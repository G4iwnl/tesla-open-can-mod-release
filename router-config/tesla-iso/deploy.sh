#!/bin/bash
# Tesla ISO 自动部署脚本 (Mac/Linux)
#
# 一键完成:
#   1. LuCI 登录拿 sysauth + stok
#   2. 配置 WiFi (SSID + 密码)
#   3. 启用 DHCP server
#   4. 启动本地 HTTP server 让路由器拉包
#   5. telnet 进路由器 wget + 解压 + 跑 install.sh
#   6. 关闭本地 HTTP server
#   7. 验证 web 控制台 + DNS 解析
#
# 用法:
#   # 全部默认 (需通过环境变量或参数指定 WiFi 密码)
#   sh deploy.sh
#
#   # 自定义参数
#   sh deploy.sh --router-ip 192.168.2.254 \
#                --user admin --pass YOUR_ROUTER_PASSWORD \
#                --wifi-ssid YOUR_WIFI_SSID --wifi-pass YOUR_WIFI_PASSWORD \
#                --pkg dist/tesla-iso-1.0.3.tar.gz
#
#   # 跳过 wifi/dhcp 配置 (只部署 tesla-iso)
#   sh deploy.sh --skip-luci
#
# ⚠ 警告:
#   - 如果你的电脑是 WiFi 连接这台路由器, 改 WiFi 后会断网, 部署失败
#   - 建议用网线连路由器, 或者用另一台机器跑这个脚本
#   - 路由器原 WiFi SSID 会被覆盖, 所有连着原 WiFi 的设备会断

set -e

# ============ 默认参数 ============
ROUTER_IP="${ROUTER_IP:-192.168.2.254}"
ROUTER_USER="${ROUTER_USER:-admin}"
ROUTER_PASS="${ROUTER_PASS:-admin}"
WIFI_SSID="${WIFI_SSID:-tesla}"
WIFI_PASS="${WIFI_PASS:?请通过 WIFI_PASS 环境变量或 --wifi-pass 参数指定 WiFi 密码}"
PKG_PATH="${PKG_PATH:-}"
MAC_IP="${MAC_IP:-}"
SKIP_LUCI=0
SKIP_DEPLOY=0
HTTP_PORT=8889

# ============ 解析参数 ============
while [ $# -gt 0 ]; do
  case "$1" in
    --router-ip)  ROUTER_IP="$2"; shift 2 ;;
    --user)       ROUTER_USER="$2"; shift 2 ;;
    --pass)       ROUTER_PASS="$2"; shift 2 ;;
    --wifi-ssid)  WIFI_SSID="$2"; shift 2 ;;
    --wifi-pass)  WIFI_PASS="$2"; shift 2 ;;
    --pkg)        PKG_PATH="$2"; shift 2 ;;
    --mac-ip)     MAC_IP="$2"; shift 2 ;;
    --skip-luci)  SKIP_LUCI=1; shift ;;
    --skip-deploy) SKIP_DEPLOY=1; shift ;;
    -h|--help)
      grep '^#' "$0" | head -30 | sed 's/^# \?//'
      exit 0
      ;;
    *) echo "未知参数: $1"; exit 1 ;;
  esac
done

# ============ 自动检测包路径 ============
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -z "$SCRIPT_DIR" ] && SCRIPT_DIR="$(pwd)"
cd "$SCRIPT_DIR"

if [ -z "$PKG_PATH" ]; then
  PKG_PATH=$(ls -t dist/tesla-iso-*.tar.gz 2>/dev/null | head -1)
  if [ -z "$PKG_PATH" ]; then
    echo "❌ ERROR: 未找到 dist/tesla-iso-*.tar.gz, 请先 sh build.sh"
    exit 1
  fi
fi

if [ ! -f "$PKG_PATH" ]; then
  echo "❌ ERROR: 找不到包文件: $PKG_PATH"
  exit 1
fi

PKG_NAME=$(basename "$PKG_PATH")
PKG_DIR=$(cd "$(dirname "$PKG_PATH")" && pwd)

# ============ 自动检测 Mac LAN IP (跟路由器同网段的接口) ============
if [ -z "$MAC_IP" ]; then
  # 尝试用 route get 拿出口接口
  IFACE=$(env DYLD_INSERT_LIBRARIES= route -n get "$ROUTER_IP" 2>/dev/null | awk '/interface:/{print $2}' | head -1)
  if [ -n "$IFACE" ]; then
    MAC_IP=$(env DYLD_INSERT_LIBRARIES= ifconfig "$IFACE" 2>/dev/null | awk '/inet /{print $2; exit}')
  fi
  # 失败则尝试同网段匹配
  if [ -z "$MAC_IP" ]; then
    PREFIX=$(echo "$ROUTER_IP" | cut -d. -f1-3)
    MAC_IP=$(env DYLD_INSERT_LIBRARIES= ifconfig 2>/dev/null | grep -oE "inet $PREFIX\.[0-9]+" | head -1 | awk '{print $2}')
  fi
fi

if [ -z "$MAC_IP" ]; then
  echo "❌ ERROR: 无法自动检测 Mac 同网段 IP, 请用 --mac-ip 指定"
  exit 1
fi

# ============ 工具函数 ============
need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "❌ ERROR: 缺少命令 $1, 请先安装"
    exit 1
  fi
}

need_cmd curl
need_cmd python3
need_cmd expect
need_cmd telnet

# proxychains 兼容: 所有命令前加 env DYLD_INSERT_LIBRARIES= 绕过代理 (mac)
CURL="env DYLD_INSERT_LIBRARIES= curl"
EXPECT="env DYLD_INSERT_LIBRARIES= expect"
PYTHON="env DYLD_INSERT_LIBRARIES= python3"

cat <<INFO
==================================================
  Tesla ISO 自动部署
==================================================
  Router IP : $ROUTER_IP
  Router 用户 : $ROUTER_USER
  WiFi SSID : $WIFI_SSID
  WiFi 密码 : $WIFI_PASS
  升级包    : $PKG_PATH
  Mac IP    : $MAC_IP
  Skip LuCI : $SKIP_LUCI
  Skip 部署 : $SKIP_DEPLOY
==================================================

INFO

# ============ Step 1: LuCI 登录 ============
if [ "$SKIP_LUCI" = "0" ]; then
  echo ">>> [1/7] LuCI 登录 ($ROUTER_IP)..."

  COOKIE=$(mktemp)
  HEADERS=$(mktemp)
  trap "rm -f $COOKIE $HEADERS" EXIT

  $CURL -sS -i -c "$COOKIE" --max-time 10 \
    -d "username=$ROUTER_USER&password=$ROUTER_PASS" \
    "http://$ROUTER_IP/cgi-bin/luci/" > "$HEADERS" 2>&1

  STOK=$(grep -i 'set-cookie' "$HEADERS" 2>/dev/null | grep -oE 'stok=[a-f0-9]+' | head -1 | cut -d= -f2)
  SYSAUTH=$(grep sysauth "$COOKIE" 2>/dev/null | awk '{print $7}' | head -1)

  if [ -z "$STOK" ] || [ -z "$SYSAUTH" ]; then
    echo "  ❌ 登录失败"
    echo "  --- response head ---"
    head -15 "$HEADERS"
    echo "  --- cookie ---"
    cat "$COOKIE"
    exit 1
  fi
  echo "  ✓ 登录成功 (stok=${STOK:0:8}...)"

  # ============ Step 2: 配置 WiFi ============
  echo ""
  echo ">>> [2/7] 配置 WiFi (SSID=$WIFI_SSID)..."
  $CURL -sS -b "sysauth=$SYSAUTH" --max-time 15 \
    -F "cbi.submit=1" \
    -F "tab.wireless.ra0=general" \
    -F "cbid.wireless.ra0.enabled=1" \
    -F "cbid.wireless.ra0.ssid=$WIFI_SSID" \
    -F "cbid.wireless.ra0.channel=auto" \
    -F "cbid.wireless.ra0.hwmode=11bgn" \
    -F "cbid.wireless.ra0.encryption=wpa2psk-aes" \
    -F "cbid.wireless.ra0.key=$WIFI_PASS" \
    -F "cbi.cbe.wireless.ra0.hide_ssid=1" \
    -F "cbid.wireless.ra0.htmode=HT40" \
    -F "cbid.wireless.ra0.beacon=100" \
    -F "cbid.wireless.ra0.dtim=1" \
    -F "cbid.wireless.ra0.frag=2346" \
    -F "cbid.wireless.ra0.rts=2347" \
    -F "cbid.wireless.ra0.max_client=64" \
    -F "cbid.wireless.ra0.tx_power=100" \
    -F 'cbi.apply=保存&应用' \
    "http://$ROUTER_IP/cgi-bin/luci/;stok=$STOK/admin/setup/wireless" \
    -o /dev/null -w "  HTTP %{http_code}\n"

  # ============ Step 3: 启用 DHCP server ============
  echo ""
  echo ">>> [3/7] 启用 DHCP server..."
  $CURL -sS -b "sysauth=$SYSAUTH" --max-time 15 \
    -F "cbi.submit=1" \
    -F "tab.network.lan=general" \
    -F "cbid.network.lan.proto=static" \
    -F "cbid.network.lan.ipaddr=$ROUTER_IP" \
    -F "cbid.network.lan.netmask=255.255.255.0" \
    -F "tab.dhcp.lan=general" \
    -F "cbi.cbe.dhcp.lan.ignore=1" \
    -F "cbid.dhcp.lan.start=100" \
    -F "cbid.dhcp.lan.limit=150" \
    -F "cbid.dhcp.lan.leasetime=12h" \
    -F 'cbi.apply=保存&应用' \
    "http://$ROUTER_IP/cgi-bin/luci/;stok=$STOK/admin/setup/lan" \
    -o /dev/null -w "  HTTP %{http_code}\n"

  echo ""
  echo ">>> [4/7] 等待服务重启 (10s)..."
  sleep 10
else
  echo ">>> [1-4/7] 跳过 LuCI 配置 (--skip-luci)"
fi

# ============ Step 5: 部署 tesla-iso ============
if [ "$SKIP_DEPLOY" = "0" ]; then
  echo ""
  echo ">>> [5/7] 启动本地 HTTP server (http://$MAC_IP:$HTTP_PORT/)..."
  ( cd "$PKG_DIR" && $PYTHON -m http.server $HTTP_PORT --bind "$MAC_IP" >/dev/null 2>&1 ) &
  HTTP_PID=$!
  sleep 1
  trap "kill $HTTP_PID 2>/dev/null; rm -f $COOKIE $HEADERS 2>/dev/null" EXIT

  # 等待 telnet 端口可用 (LuCI 改完 wifi/dhcp 后路由器可能短暂忙)
  echo "    等待 telnet 23 可用..."
  for i in 1 2 3 4 5 6 7 8 9 10; do
    if $CURL -sS --max-time 2 "telnet://$ROUTER_IP:23" -o /dev/null 2>/dev/null; then break; fi
    if nc -z "$ROUTER_IP" 23 2>/dev/null; then break; fi
    sleep 1
  done

  echo ""
  echo ">>> [6/7] telnet 部署 $PKG_NAME..."

  # 在 shell 端预先算好解压后的目录名 (避免 expect/Tcl 解析 ${VAR%pattern})
  INSTALL_DIR=$(basename "$PKG_NAME" .tar.gz)

  $EXPECT <<EXPECT_EOF
set timeout 240
log_user 1
spawn telnet $ROUTER_IP
expect "login:"
send "$ROUTER_USER\r"
expect "assword:"
send "$ROUTER_PASS\r"
expect "admin@router:"
send "cd /tmp && rm -rf tesla-iso-*.tar.gz tesla-iso-1.* 2>/dev/null; wget -q -O $PKG_NAME http://$MAC_IP:$HTTP_PORT/$PKG_NAME && tar -xzf $PKG_NAME && cd $INSTALL_DIR && sh install.sh; echo MARK_INSTALL_DONE_\$?\r"
expect {
  "MARK_INSTALL_DONE_0" { send_user "\n--- INSTALL OK ---\n" }
  "MARK_INSTALL_DONE_1" { send_user "\n--- INSTALL FAILED ---\n" }
  timeout { send_user "\n--- INSTALL TIMEOUT ---\n" }
}
expect "admin@router:"
send "exit\r"
expect eof
EXPECT_EOF

  kill $HTTP_PID 2>/dev/null || true
  sleep 1
else
  echo ""
  echo ">>> [5-6/7] 跳过 tesla-iso 部署 (--skip-deploy)"
fi

# ============ Step 7: 验证 ============
echo ""
echo ">>> [7/7] 验证..."
sleep 2

WEB_CODE=$($CURL -sS --max-time 5 -o /dev/null -w "%{http_code}" "http://$ROUTER_IP:8888/" 2>/dev/null || echo "000")
if [ "$WEB_CODE" = "200" ]; then
  echo "  ✓ Web 控制台 200 OK"
else
  echo "  ⚠ Web 控制台 HTTP $WEB_CODE"
fi

VER=$($CURL -sS --max-time 5 "http://$ROUTER_IP:8888/cgi-bin/api.sh?action=status" 2>/dev/null \
  | grep -oE '"version":"[^"]+"' | cut -d\" -f4)
if [ -n "$VER" ]; then
  echo "  ✓ Tesla ISO 版本: $VER"
else
  echo "  ⚠ 拿不到版本号"
fi

# ============ Step 7.5: 双网段并存验证 (2.2.2.1 secondary IP) ============
# 车机浏览器禁止访问 RFC1918 私有 IP, 所以路由器需要 2.2.2.1/24 secondary IP
# 这步验证 firewall.user 已正确加 secondary IP, 并且 web 服务对该 IP 响应
echo ""
echo ">>> [7.5/9] 双网段验证 (2.2.2.1/24)..."
DUAL_8888=$($CURL -sS --max-time 5 -o /dev/null -w "%{http_code}" "http://2.2.2.1:8888/" 2>/dev/null || echo "000")
DUAL_80=$($CURL -sS --max-time 5 -o /dev/null -w "%{http_code}" "http://2.2.2.1/" 2>/dev/null || echo "000")
if [ "$DUAL_8888" = "200" ]; then
  echo "  ✓ http://2.2.2.1:8888/ → HTTP 200 (Tesla ISO 控制台,车机可访问)"
else
  echo "  ✗ http://2.2.2.1:8888/ → HTTP $DUAL_8888"
  echo "    检查: telnet $ROUTER_IP 后跑 'ip addr show br-lan | grep 2.2.2.1'"
  echo "    应该看到: inet 2.2.2.1/24 ... br-lan"
fi
if [ "$DUAL_80" = "200" ] || [ "$DUAL_80" = "302" ] || [ "$DUAL_80" = "401" ]; then
  echo "  ✓ http://2.2.2.1/      → HTTP $DUAL_80 (LuCI 路由器后台)"
else
  echo "  ⚠ http://2.2.2.1/      → HTTP $DUAL_80"
fi

# 双网段失败, 给出诊断
if [ "$DUAL_8888" != "200" ]; then
  echo ""
  echo "  ⚠ 双网段验证失败. 可能原因:"
  echo "    1. firewall.user 没生效 (应自动 ip addr add 2.2.2.1/24 dev br-lan)"
  echo "    2. uhttpd 没监听 2.2.2.1 (应该监听 0.0.0.0:8888,会自动包含)"
  echo "    3. iptables 拒绝 2.2.2.1 入站 (firewall.user 应放行 RFC1918 + br-lan)"
fi

# ============ Step 8: 切换到保守版 (默认安全) ============
echo ""
echo ">>> [8/9] 切换到保守版 (mode=safe)..."
$CURL -sS -X POST -d "mode=safe" --max-time 15 \
  "http://$ROUTER_IP:8888/cgi-bin/api.sh?action=switch_mode" 2>/dev/null \
  | grep -oE '"ok":(true|false)' || echo "  ⚠ 切换响应异常"
sleep 4  # 等 dnsmasq 重启

# ============ Step 9: 跑域名检测 ============
echo ""
echo ">>> [9/9] 跑域名检测..."
echo ""
if [ -x "$SCRIPT_DIR/verify.sh" ] || [ -f "$SCRIPT_DIR/verify.sh" ]; then
  bash "$SCRIPT_DIR/verify.sh" --router-ip "$ROUTER_IP" || VERIFY_FAIL=1
else
  echo "  ⚠ 找不到 verify.sh, 跳过检测"
fi

echo ""
cat <<DONE
==================================================
  ✅ 部署完成
==================================================
  ── 电脑/手机访问 (192.168.2.x DHCP) ──
  Tesla ISO 控制台 : http://$ROUTER_IP:8888
  路由器后台 LuCI  : http://$ROUTER_IP

  ── 车机访问 (车机禁 RFC1918 段, 走 2.2.2.x) ──
  Tesla ISO 控制台 : http://2.2.2.1:8888
  路由器后台 LuCI  : http://2.2.2.1
  CAN Mod 模块     : http://2.2.2.2  (ESP32 静态 IP)

  ── WiFi & 登录 ──
  WiFi SSID  : $WIFI_SSID
  WiFi 密码  : $WIFI_PASS
  路由器登录 : telnet $ROUTER_IP / $ROUTER_USER:$ROUTER_PASS
  当前模式   : 🛡 保守版 (safe)

  下一步:
  1. Tesla 车机连接 WiFi: $WIFI_SSID
  2. 重启车机一次 (按住方向盘左右滚轮 10 秒) 清 DNS 缓存
  3. 车机浏览器打开 http://2.2.2.1:8888
     - 如需手机 App 控车, 在顶部切换到 "⚡ 激进版"
DONE
