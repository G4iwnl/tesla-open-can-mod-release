#!/bin/sh
# Tesla ISO v1.0 - 首装脚本
# 在路由器上 (BusyBox + dnsmasq + iptables) 运行
#
# 用法:
#   cd /tmp/tesla-iso-1.0.0
#   sh install.sh

set -u

THIS_VER=$(cat "$(dirname "$0")/files/etc/tesla_iso/version" 2>/dev/null || echo "1.0.0")
SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -z "$SCRIPT_DIR" ] && SCRIPT_DIR="$(pwd)"
FILES_DIR="$SCRIPT_DIR/files"

echo "=================================================="
echo "  Tesla ISO Installer v$THIS_VER"
echo "=================================================="
echo ""

# ---------- 0. 前置检查 ----------
echo ">>> [0/9] 检查前置依赖..."
MISSING=""
for cmd in dnsmasq iptables ip6tables sed grep awk uhttpd tar; do
  command -v "$cmd" >/dev/null 2>&1 || MISSING="$MISSING $cmd"
done
if [ -n "$MISSING" ]; then
  echo "  错误: 缺少命令:$MISSING"
  echo "  本路由器不兼容 (需要 OpenWrt/LEDE 含 uhttpd)"
  echo "  提示: 如果只缺 uhttpd, 可以尝试 opkg install uhttpd"
  exit 1
fi
for f in files/etc/dnsmasq.conf files/etc/dnsmasq.d/tesla_iso.conf files/etc/firewall.user \
         files/usr/sbin/tesla-iso-httpd-init.sh files/usr/sbin/tesla-iso-collector.sh \
         files/www-tesla/index.html files/www-tesla/cgi-bin/api.sh; do
  if [ ! -f "$SCRIPT_DIR/$f" ]; then
    echo "  错误: 找不到 $f"
    exit 1
  fi
done
echo "  OK"

# ---------- 1. 检测路由器 LAN IP ----------
echo ""
echo ">>> [1/9] 检测路由器 LAN IP..."
ROUTER_IP=$(ip -4 addr show br-lan 2>/dev/null | awk '/inet / {print $2}' | cut -d/ -f1 | head -1)
[ -z "$ROUTER_IP" ] && ROUTER_IP=$(ifconfig br-lan 2>/dev/null | awk '/inet addr/ {sub(/addr:/,"",$2); print $2}' | head -1)
[ -z "$ROUTER_IP" ] && ROUTER_IP="192.168.2.254"
echo "  路由器 IP: $ROUTER_IP"

# ---------- 2. 检测旧版本方案 (tesla-wifi-isolation) ----------
echo ""
echo ">>> [2/9] 检测旧版方案..."
OLD_DETECTED=0
if [ -f /etc/tesla_whitelist.sh ]; then
  echo "  检测到旧版 tesla-wifi-isolation 方案"
  OLD_DETECTED=1
fi
if iptables -L DNSONLY -n >/dev/null 2>&1; then
  echo "  检测到旧版 DNSONLY iptables 链"
  OLD_DETECTED=1
fi

# ---------- 3. 备份当前配置 ----------
echo ""
echo ">>> [3/9] 备份当前配置..."
TS=$(date +%Y%m%d_%H%M%S 2>/dev/null || echo manual)
BACKUP_DIR="/etc/tesla_iso_backup/preinstall_$TS"
mkdir -p "$BACKUP_DIR"
[ -f /etc/dnsmasq.conf ] && cp /etc/dnsmasq.conf "$BACKUP_DIR/dnsmasq.conf.bak"
[ -f /etc/firewall.user ] && cp /etc/firewall.user "$BACKUP_DIR/firewall.user.bak"
[ -f /etc/hosts ] && cp /etc/hosts "$BACKUP_DIR/hosts.bak"
[ -d /etc/dnsmasq.d ] && cp -r /etc/dnsmasq.d "$BACKUP_DIR/dnsmasq.d.bak" 2>/dev/null
[ -f /etc/tesla_whitelist.sh ] && cp /etc/tesla_whitelist.sh "$BACKUP_DIR/tesla_whitelist.sh.bak"
[ -f /etc/whitelist_ips.txt ] && cp /etc/whitelist_ips.txt "$BACKUP_DIR/whitelist_ips.txt.bak"
echo "0.0.0" > "$BACKUP_DIR/version.bak"
echo "  备份位置: $BACKUP_DIR"

# ---------- 4. 清理旧版方案 ----------
if [ "$OLD_DETECTED" = "1" ]; then
  echo ""
  echo ">>> [4/9] 清理旧版方案..."
  # 移除旧 iptables DNSONLY 链
  while iptables -D FORWARD -j DNSONLY 2>/dev/null; do :; done
  iptables -F DNSONLY 2>/dev/null
  iptables -X DNSONLY 2>/dev/null
  # 移除旧 cron 任务
  for cf in /etc/crontabs/*; do
    [ -f "$cf" ] || continue
    if grep -q "tesla_whitelist.sh" "$cf" 2>/dev/null; then
      grep -v "tesla_whitelist.sh" "$cf" > "$cf.tmp" && mv "$cf.tmp" "$cf"
    fi
  done
  # 移除旧文件
  rm -f /etc/tesla_whitelist.sh /etc/whitelist_ips.txt
  rm -f /tmp/tesla_whitelist.lock /tmp/whitelist_ips.txt /tmp/whitelist.log
  # 移除旧的 connman 硬绑 (新方案不需要硬绑了)
  if [ -f /etc/hosts ]; then
    grep -v "connman.vn.cloud.tesla.cn" /etc/hosts > /tmp/hosts.new 2>/dev/null && mv /tmp/hosts.new /etc/hosts
  fi
  echo "  旧版清理完成"
else
  echo ""
  echo ">>> [4/9] 跳过 (无旧版)"
fi

# ---------- 5. 部署新版文件 ----------
echo ""
echo ">>> [5/9] 部署 v$THIS_VER 文件..."

# 创建必要目录
mkdir -p /etc/dnsmasq.d /etc/tesla_iso /usr/sbin /www-tesla/cgi-bin

# 部署文件 (先 copy 临时再 mv, 减少中间状态)
cp "$FILES_DIR/etc/dnsmasq.conf" /etc/dnsmasq.conf.new && mv /etc/dnsmasq.conf.new /etc/dnsmasq.conf
chmod 644 /etc/dnsmasq.conf
echo "  /etc/dnsmasq.conf"

# tesla_iso.conf: 保留用户已有的例外, 不覆盖
if [ ! -f /etc/dnsmasq.d/tesla_iso.conf ]; then
  cp "$FILES_DIR/etc/dnsmasq.d/tesla_iso.conf" /etc/dnsmasq.d/tesla_iso.conf
  chmod 644 /etc/dnsmasq.d/tesla_iso.conf
  echo "  /etc/dnsmasq.d/tesla_iso.conf (新建)"
else
  echo "  /etc/dnsmasq.d/tesla_iso.conf (已存在, 保留用户例外)"
fi

cp "$FILES_DIR/etc/firewall.user" /etc/firewall.user.new && mv /etc/firewall.user.new /etc/firewall.user
chmod +x /etc/firewall.user
echo "  /etc/firewall.user"

cp "$FILES_DIR/usr/sbin/tesla-iso-httpd-init.sh" /usr/sbin/tesla-iso-httpd-init.sh.new \
  && mv /usr/sbin/tesla-iso-httpd-init.sh.new /usr/sbin/tesla-iso-httpd-init.sh
chmod +x /usr/sbin/tesla-iso-httpd-init.sh
echo "  /usr/sbin/tesla-iso-httpd-init.sh"

cp "$FILES_DIR/usr/sbin/tesla-iso-collector.sh" /usr/sbin/tesla-iso-collector.sh.new \
  && mv /usr/sbin/tesla-iso-collector.sh.new /usr/sbin/tesla-iso-collector.sh
chmod +x /usr/sbin/tesla-iso-collector.sh
echo "  /usr/sbin/tesla-iso-collector.sh"

cp "$FILES_DIR/www-tesla/index.html" /www-tesla/index.html.new && mv /www-tesla/index.html.new /www-tesla/index.html
chmod 644 /www-tesla/index.html
cp "$FILES_DIR/www-tesla/cgi-bin/api.sh" /www-tesla/cgi-bin/api.sh.new && mv /www-tesla/cgi-bin/api.sh.new /www-tesla/cgi-bin/api.sh
chmod +x /www-tesla/cgi-bin/api.sh
echo "  /www-tesla/index.html + cgi-bin/api.sh"

echo "$THIS_VER" > /etc/tesla_iso/version
echo "  /etc/tesla_iso/version = $THIS_VER"

# --- 注册 cron 任务 (collector 每 5 分钟跑一次) ---
echo ""
echo ">>> 注册 cron 任务..."
mkdir -p /etc/crontabs
CURRENT_USER=$(whoami 2>/dev/null || id -un 2>/dev/null || echo root)
CRON_FILE="/etc/crontabs/$CURRENT_USER"
touch "$CRON_FILE"
# 先清除自己的旧条目防重复
if grep -q "tesla-iso-collector.sh" "$CRON_FILE" 2>/dev/null; then
  grep -v "tesla-iso-collector.sh" "$CRON_FILE" > "$CRON_FILE.tmp" && mv "$CRON_FILE.tmp" "$CRON_FILE"
fi
echo "*/5 * * * * /usr/sbin/tesla-iso-collector.sh" >> "$CRON_FILE"
chmod 600 "$CRON_FILE"
echo "  $CRON_FILE: */5 * * * * /usr/sbin/tesla-iso-collector.sh"

# 创建 runtime 目录
mkdir -p /tmp/tesla_iso 2>/dev/null

# 立即跑一次 collector 以建立初始数据
/usr/sbin/tesla-iso-collector.sh 2>/dev/null
echo "  collector 首次运行完成"

# ---------- 6. 重启 dnsmasq ----------
echo ""
echo ">>> [6/9] 重启 dnsmasq..."
if [ -x /etc/init.d/dnsmasq ]; then
  /etc/init.d/dnsmasq restart >/dev/null 2>&1
else
  killall dnsmasq 2>/dev/null
  sleep 1
  /usr/sbin/dnsmasq -C /etc/dnsmasq.conf 2>/dev/null
fi
# OpenWrt procd 重启 dnsmasq 有几秒延迟, retry up to 10s
_dnsmasq_ok=0
for _i in 1 2 3 4 5 6 7 8 9 10; do
  if pidof dnsmasq >/dev/null 2>&1; then
    _dnsmasq_ok=1
    break
  fi
  sleep 1
done
if [ "$_dnsmasq_ok" = "1" ]; then
  echo "  dnsmasq 运行中"
else
  echo "  警告: dnsmasq 未启动, 检查 /etc/dnsmasq.conf"
fi

# ---------- 7. 应用 firewall.user ----------
echo ""
echo ">>> [7/9] 应用 firewall.user..."
sh /etc/firewall.user 2>/dev/null
sleep 1
echo "  完成"

# ---------- 8. 启动 web httpd ----------
echo ""
echo ">>> [8/9] 启动 web 控制台..."
/usr/sbin/tesla-iso-httpd-init.sh restart 2>/dev/null
sleep 1
if /usr/sbin/tesla-iso-httpd-init.sh status >/dev/null 2>&1; then
  echo "  web 控制台已启动: http://$ROUTER_IP:8888"
else
  echo "  警告: web 控制台启动失败"
fi

# ---------- 9. 自检 ----------
echo ""
echo ">>> [9/9] 自检..."
TESTS_OK=0
TESTS_FAIL=0

check() {
  if eval "$2" >/dev/null 2>&1; then
    echo "  [OK]   $1"
    TESTS_OK=$((TESTS_OK + 1))
  else
    echo "  [FAIL] $1"
    TESTS_FAIL=$((TESTS_FAIL + 1))
  fi
}

# DNS 解析检查
resolve_test() {
  _expected="$1"
  _domain="$2"
  _got=$(nslookup "$_domain" 127.0.0.1 2>/dev/null | awk '/^Name:/{seen=1;next} seen && /^Address/ {for(i=1;i<=NF;i++) if($i ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) {print $i; exit}}')
  case "$_expected" in
    sinkhole) [ "$_got" = "127.0.0.1" ] ;;
    real)     [ -n "$_got" ] && [ "$_got" != "127.0.0.1" ] ;;
  esac
}

check "tesla.cn 被 sinkhole"           "resolve_test sinkhole www.tesla.cn"
check "hermes-prd 被 sinkhole"         "resolve_test sinkhole hermes-prd.vn.cloud.tesla.cn"
check "api-prd 被 sinkhole"            "resolve_test sinkhole api-prd.vn.cloud.tesla.cn"
check "telemetry-prd 被 sinkhole"      "resolve_test sinkhole telemetry-prd.vn.cloud.tesla.cn"
check "nav-prd-maps 例外放行"          "resolve_test real nav-prd-maps.tesla.cn"
check "connman 例外放行"               "resolve_test real connman.vn.cloud.tesla.cn"
check "百度可解析 (DNS 全开)"          "resolve_test real www.baidu.com"
check "腾讯可解析"                     "resolve_test real www.qq.com"
check "dnsmasq 进程"                   "pidof dnsmasq"
check "web httpd 进程"                 "/usr/sbin/tesla-iso-httpd-init.sh status"
check "IPv6 FORWARD 已封"              "ip6tables -L FORWARD -n | head -3 | grep -q DROP"
check "8888 LAN-only INPUT 规则"       "iptables -L INPUT -n | grep -q '8888'"
check "version 文件"                   "[ -f /etc/tesla_iso/version ]"

echo ""
echo "=================================================="
if [ "$TESTS_FAIL" = "0" ]; then
  echo "  ✅ 安装成功: $TESTS_OK 项检查通过"
  echo "=================================================="
  echo ""
  echo "  📍 Web 控制台: http://$ROUTER_IP:8888"
  echo ""
  echo "  ⚠️  下一步必做:"
  echo "  1. Tesla 车机连接本路由器的 WiFi (不要连其他)"
  echo "  2. 禁用车机 4G 数据"
  echo "  3. !! 重启车机一次 !! (按住方向盘左右滚轮 10 秒)"
  echo "     这是为了清除车机内的旧 DNS 缓存和 hermes 长连接"
  echo ""
  echo "  💾 备份: $BACKUP_DIR"
  echo ""
  exit 0
else
  echo "  ❌ 失败: $TESTS_FAIL 项检查未通过 ($TESTS_OK 通过)"
  echo "=================================================="
  echo ""
  echo "  备份: $BACKUP_DIR"
  echo "  卸载: sh $SCRIPT_DIR/uninstall.sh"
  echo ""
  exit 1
fi
