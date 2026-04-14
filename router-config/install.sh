#!/bin/sh
# Tesla WiFi Isolation - Auto Installer
# Run this ON THE ROUTER (OpenWrt/BusyBox), not on your Mac.
#
# Usage:
#   cd /tmp/tesla-wifi-isolation
#   sh install.sh
#
# Requires: iptables, ip6tables, dnsmasq, crond, nslookup, sed, grep, awk, sort, uniq, cmp
# All scripts auto-detect router IP and Tesla IP — no manual IP config needed.

SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -z "$SCRIPT_DIR" ] && SCRIPT_DIR="$(pwd)"

echo "=================================================="
echo "  Tesla WiFi Isolation Installer"
echo "=================================================="
echo ""

# ---------- Step 0: Prerequisite check ----------
echo ">>> [0/9] Checking prerequisites..."
MISSING=""
for cmd in iptables ip6tables dnsmasq nslookup sed grep awk sort uniq cmp sync; do
    command -v "$cmd" >/dev/null 2>&1 || MISSING="$MISSING $cmd"
done
if [ -n "$MISSING" ]; then
    echo "  ERROR: missing commands:$MISSING"
    echo "  This router is not compatible (need OpenWrt/LEDE/BusyBox with iptables)."
    exit 1
fi
if ! command -v crond >/dev/null 2>&1 && [ ! -x /usr/sbin/crond ] && [ ! -x /sbin/crond ]; then
    echo "  ERROR: crond not found"
    exit 1
fi

for f in dnsmasq.conf firewall.user tesla_whitelist.sh; do
    if [ ! -f "$SCRIPT_DIR/$f" ]; then
        echo "  ERROR: $SCRIPT_DIR/$f not found"
        echo "  Run this script from the directory containing the config files."
        exit 1
    fi
done
echo "  OK"

# ---------- Step 1: Detect router LAN IP (for preview) ----------
echo ""
echo ">>> [1/9] Detecting router LAN IP..."
ROUTER_IP=$(ip -4 addr show br-lan 2>/dev/null | awk '/inet / {print $2}' | cut -d/ -f1 | head -1)
[ -z "$ROUTER_IP" ] && ROUTER_IP=$(ifconfig br-lan 2>/dev/null | awk '/inet addr/ {sub(/addr:/,"",$2); print $2}' | head -1)
if [ -n "$ROUTER_IP" ]; then
    echo "  Detected: $ROUTER_IP"
else
    echo "  WARNING: auto-detect failed, scripts will use fallback 192.168.2.254"
fi

# ---------- Step 2: Preview Tesla detection ----------
echo ""
echo ">>> [2/9] Previewing Tesla auto-detection..."
TESLA_FOUND=""
if [ -f /tmp/dhcp.leases ]; then
    # Try MAC OUI prefixes
    for oui in 4c:fc:aa dc:44:27 18:cf:5e 68:ab:1e ec:3a:4e 98:ed:5c 98:ed:7e 0c:29:8f 54:26:96 e8:eb:1b cc:88:26 bc:d0:74 b0:41:1d 8c:f5:a3 74:e1:b6; do
        TESLA_FOUND=$(awk -v p="^$oui" '{if (tolower($2) ~ p) {print $3"|"$2"|"$4; exit}}' /tmp/dhcp.leases 2>/dev/null)
        [ -n "$TESLA_FOUND" ] && break
    done
    if [ -z "$TESLA_FOUND" ]; then
        TESLA_FOUND=$(awk '{h=tolower($4)} h ~ /^(tesla|model[-_]?[3sxy])/ {print $3"|"$2"|"$4; exit}' /tmp/dhcp.leases 2>/dev/null)
    fi
fi
if [ -n "$TESLA_FOUND" ]; then
    echo "  Tesla detected: $TESLA_FOUND (ip|mac|hostname)"
else
    echo "  Tesla not currently in DHCP leases."
    echo "  OK - script will auto-detect at runtime whenever Tesla connects."
fi

# ---------- Step 3: Backup existing config ----------
echo ""
echo ">>> [3/9] Backing up existing config..."
BACKUP_DIR="/etc/tesla-wifi-backup-$(date +%s 2>/dev/null || echo manual)"
mkdir -p "$BACKUP_DIR"
for f in /etc/dnsmasq.conf /etc/firewall.user /etc/hosts /etc/tesla_whitelist.sh /etc/whitelist_ips.txt; do
    [ -f "$f" ] && cp "$f" "$BACKUP_DIR/$(basename $f).bak" 2>/dev/null
done
[ -d /etc/crontabs ] && cp -r /etc/crontabs "$BACKUP_DIR/crontabs.bak" 2>/dev/null
echo "  Backup saved to: $BACKUP_DIR"

# ---------- Step 4: Install config files ----------
echo ""
echo ">>> [4/9] Installing config files..."
cp "$SCRIPT_DIR/dnsmasq.conf" /etc/dnsmasq.conf
chmod 644 /etc/dnsmasq.conf
echo "  /etc/dnsmasq.conf installed"

cp "$SCRIPT_DIR/firewall.user" /etc/firewall.user
chmod +x /etc/firewall.user
echo "  /etc/firewall.user installed"

cp "$SCRIPT_DIR/tesla_whitelist.sh" /etc/tesla_whitelist.sh
chmod +x /etc/tesla_whitelist.sh
echo "  /etc/tesla_whitelist.sh installed"

# ---------- Step 5: Add connman to /etc/hosts ----------
echo ""
echo ">>> [5/9] Adding connman to /etc/hosts..."
CONNMAN_LINE="116.129.226.175 connman.vn.cloud.tesla.cn"
touch /etc/hosts
if grep -q "connman.vn.cloud.tesla.cn" /etc/hosts 2>/dev/null; then
    grep -v "connman.vn.cloud.tesla.cn" /etc/hosts > /tmp/hosts.new
    echo "$CONNMAN_LINE" >> /tmp/hosts.new
    mv /tmp/hosts.new /etc/hosts
    echo "  Updated existing entry"
else
    echo "$CONNMAN_LINE" >> /etc/hosts
    echo "  Added new entry"
fi

# ---------- Step 6: Install crontab (every minute) ----------
echo ""
echo ">>> [6/9] Installing crontab entry..."
CURRENT_USER=$(whoami 2>/dev/null || id -un 2>/dev/null || echo root)
mkdir -p /etc/crontabs

# Remove any existing tesla_whitelist.sh entries from ALL crontab files (avoid duplicate triggers)
for cf in /etc/crontabs/*; do
    [ -f "$cf" ] || continue
    if grep -q "tesla_whitelist.sh" "$cf" 2>/dev/null; then
        grep -v "tesla_whitelist.sh" "$cf" > "$cf.tmp" && mv "$cf.tmp" "$cf"
    fi
done

# Add entry to current user's crontab only
CRON_FILE="/etc/crontabs/$CURRENT_USER"
touch "$CRON_FILE"
echo "* * * * * /etc/tesla_whitelist.sh" >> "$CRON_FILE"
chmod 600 "$CRON_FILE"
echo "  Written to $CRON_FILE (user: $CURRENT_USER)"

# ---------- Step 7: Restart dnsmasq to load new DNS config ----------
echo ""
echo ">>> [7/9] Restarting dnsmasq..."
if [ -x /etc/init.d/dnsmasq ]; then
    /etc/init.d/dnsmasq restart >/dev/null 2>&1
else
    killall dnsmasq 2>/dev/null
    sleep 1
    /usr/sbin/dnsmasq -C /etc/dnsmasq.conf -k >/dev/null 2>&1 &
fi
sleep 2
if pgrep dnsmasq >/dev/null 2>&1; then
    echo "  dnsmasq running"
else
    echo "  WARNING: dnsmasq may not have restarted, check manually"
fi

# ---------- Step 8: Apply firewall.user (build DNSONLY + start crond) ----------
echo ""
echo ">>> [8/9] Applying firewall rules..."
sh /etc/firewall.user
sleep 1
# Defensive: ensure crond running
if ! pgrep -x crond >/dev/null 2>&1; then
    crond -c /etc/crontabs -b -L /tmp/cron.log 2>/dev/null
    sleep 1
fi
# Run whitelist script once to seed
/etc/tesla_whitelist.sh 2>/dev/null
echo "  Done"

# ---------- Step 9: Verify ----------
echo ""
echo ">>> [9/9] Verification..."
echo ""

TESTS_OK=0
TESTS_FAIL=0

check_eq() {
    _label="$1"; _actual="$2"; _expected="$3"
    if [ "$_actual" = "$_expected" ]; then
        echo "  [OK]   $_label"
        TESTS_OK=$((TESTS_OK+1))
    else
        echo "  [FAIL] $_label (got: $_actual, want: $_expected)"
        TESTS_FAIL=$((TESTS_FAIL+1))
    fi
}

check_real_ip() {
    _label="$1"; _actual="$2"
    case "$_actual" in
        127.*|"")
            echo "  [FAIL] $_label (got: $_actual, want: real IP)"
            TESTS_FAIL=$((TESTS_FAIL+1))
            ;;
        *)
            echo "  [OK]   $_label -> $_actual"
            TESTS_OK=$((TESTS_OK+1))
            ;;
    esac
}

resolve() {
    # Skip the "Address X: 127.0.0.1 localhost" DNS-server-info line;
    # only read Address lines AFTER the "Name:" line (actual answer).
    nslookup "$1" 127.0.0.1 2>/dev/null | awk '
        /^Name:/ {seen_name=1; next}
        seen_name && /^Address/ {
            for(i=1;i<=NF;i++) if($i ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) {print $i; exit}
        }'
}

echo "DNS resolution tests:"
check_eq "connman.vn.cloud.tesla.cn is fixed IP" "$(resolve connman.vn.cloud.tesla.cn)" "116.129.226.175"
check_real_ip "nav-prd-maps.tesla.cn (real)" "$(resolve nav-prd-maps.tesla.cn)"
check_eq "www.tesla.cn sinkholed" "$(resolve www.tesla.cn)" "127.0.0.1"
check_eq "api-prd.vn.cloud.tesla.cn sinkholed" "$(resolve api-prd.vn.cloud.tesla.cn)" "127.0.0.1"
check_eq "hermes-prd.vn.cloud.tesla.cn sinkholed" "$(resolve hermes-prd.vn.cloud.tesla.cn)" "127.0.0.1"
check_eq "telemetry-prd.vn.cloud.tesla.cn sinkholed" "$(resolve telemetry-prd.vn.cloud.tesla.cn)" "127.0.0.1"
check_real_ip "www.qq.com" "$(resolve www.qq.com)"
check_real_ip "baidu.com" "$(resolve baidu.com)"

echo ""
echo "iptables checks:"
if iptables -L DNSONLY -n >/dev/null 2>&1; then
    echo "  [OK]   DNSONLY chain exists"
    TESTS_OK=$((TESTS_OK+1))
else
    echo "  [FAIL] DNSONLY chain missing"
    TESTS_FAIL=$((TESTS_FAIL+1))
fi

if iptables -C FORWARD -j DNSONLY 2>/dev/null; then
    echo "  [OK]   DNSONLY in FORWARD chain"
    TESTS_OK=$((TESTS_OK+1))
else
    echo "  [FAIL] DNSONLY not in FORWARD chain"
    TESTS_FAIL=$((TESTS_FAIL+1))
fi

# ip6tables FORWARD DROP at position 1
IP6_FIRST_RULE=$(ip6tables -L FORWARD -n 2>/dev/null | awk 'NR==3 {print $1}')
check_eq "IPv6 FORWARD first rule is DROP" "$IP6_FIRST_RULE" "DROP"

if pgrep -x crond >/dev/null 2>&1; then
    echo "  [OK]   crond running"
    TESTS_OK=$((TESTS_OK+1))
else
    echo "  [FAIL] crond not running"
    TESTS_FAIL=$((TESTS_FAIL+1))
fi

if grep -l tesla_whitelist.sh /etc/crontabs/* >/dev/null 2>&1; then
    echo "  [OK]   cron entry installed"
    TESTS_OK=$((TESTS_OK+1))
else
    echo "  [FAIL] cron entry missing"
    TESTS_FAIL=$((TESTS_FAIL+1))
fi

echo ""
echo "=================================================="
if [ "$TESTS_FAIL" = "0" ]; then
    echo "  SUCCESS: $TESTS_OK/$((TESTS_OK+TESTS_FAIL)) tests passed"
    echo "=================================================="
    echo ""
    echo "  Installation complete."
    echo "  Backup: $BACKUP_DIR"
    echo ""
    echo "  Next steps:"
    echo "  1. Connect Tesla to this router's WiFi"
    echo "  2. Script auto-detects Tesla by MAC OUI and starts whitelisting"
    echo "  3. Watch:  tail -f /tmp/whitelist.log"
    echo "  4. Watch:  tail -f /tmp/dns.log"
    echo ""
    exit 0
else
    echo "  FAILED: $TESTS_FAIL/$((TESTS_OK+TESTS_FAIL)) tests failed"
    echo "=================================================="
    echo ""
    echo "  Review output above. Backup at $BACKUP_DIR for rollback."
    echo "  To uninstall: sh $SCRIPT_DIR/uninstall.sh"
    echo ""
    exit 1
fi
