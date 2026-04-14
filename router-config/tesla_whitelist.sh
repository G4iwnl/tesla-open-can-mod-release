#!/bin/sh
# Tesla WiFi Whitelist
# - PID lock to prevent concurrent runs
# - Suffix matching for non-Tesla, exact matching for Tesla
# - Hardcoded 116.129.226.175 fallback (connman always allowed)
# - Persist IPs to /etc/whitelist_ips.txt (only on change, flash-friendly)

# --- PID lock ---
LOCKFILE="/tmp/tesla_whitelist.lock"
if [ -f "$LOCKFILE" ]; then
  OLD_PID=$(cat "$LOCKFILE" 2>/dev/null)
  if [ -n "$OLD_PID" ] && [ -d "/proc/$OLD_PID" ]; then
    exit 0
  fi
fi
echo $$ > "$LOCKFILE"
trap 'rm -f "$LOCKFILE"' EXIT INT TERM

DNS="127.0.0.1"
IPFILE="/tmp/whitelist_ips.txt"
PERSIST_IPFILE="/etc/whitelist_ips.txt"
LOGFILE="/tmp/whitelist.log"
DNSLOG="/tmp/dns.log"

TESLA_EXACT="connman.vn.cloud.tesla.cn nav-prd-maps.tesla.cn"
CONNMAN_FIXED_IP="116.129.226.175"

# Tesla MAC OUI prefixes (IEEE registered for Tesla Motors/Tesla, Inc.)
# Add more here if your Tesla uses a different OUI
TESLA_MAC_OUIS="4c:fc:aa dc:44:27 18:cf:5e 68:ab:1e ec:3a:4e 98:ed:5c 98:ed:7e 0c:29:8f 54:26:96 e8:eb:1b cc:88:26 bc:d0:74 b0:41:1d 8c:f5:a3 74:e1:b6"

# Auto-detect Tesla IP: DHCP leases by MAC OUI, then hostname, then ARP table
detect_tesla_ip() {
    _ip=""
    # 1. /tmp/dhcp.leases by MAC OUI
    if [ -f /tmp/dhcp.leases ]; then
        for _oui in $TESLA_MAC_OUIS; do
            _ip=$(awk -v p="^$_oui" '{if (tolower($2) ~ p) {print $3; exit}}' /tmp/dhcp.leases 2>/dev/null)
            [ -n "$_ip" ] && { echo "$_ip"; return 0; }
        done
        # 2. /tmp/dhcp.leases by hostname
        _ip=$(awk '{h=tolower($4)} h ~ /^(tesla|model[-_]?[3sxy])/ {print $3; exit}' /tmp/dhcp.leases 2>/dev/null)
        [ -n "$_ip" ] && { echo "$_ip"; return 0; }
    fi
    # 3. /proc/net/arp by MAC OUI
    if [ -f /proc/net/arp ]; then
        for _oui in $TESLA_MAC_OUIS; do
            _ip=$(awk -v p="^$_oui" 'NR>1 {if (tolower($4) ~ p) {print $1; exit}}' /proc/net/arp 2>/dev/null)
            [ -n "$_ip" ] && { echo "$_ip"; return 0; }
        done
    fi
    return 1
}

TESLA_IP=$(detect_tesla_ip)
[ -z "$TESLA_IP" ] && TESLA_IP="__NOT_FOUND__"

# Auto-detect router LAN IP (for iptables self-allow rule)
detect_router_ip() {
    _ip=""
    if command -v ip >/dev/null 2>&1; then
        _ip=$(ip -4 addr show br-lan 2>/dev/null | awk '/inet / {print $2}' | cut -d/ -f1 | head -1)
    fi
    if [ -z "$_ip" ]; then
        _ip=$(ifconfig br-lan 2>/dev/null | awk '/inet addr/ {sub(/addr:/,"",$2); print $2}' | head -1)
    fi
    [ -n "$_ip" ] && echo "$_ip"
}
ROUTER_IP=$(detect_router_ip)
[ -z "$ROUTER_IP" ] && ROUTER_IP="192.168.2.254"

SUFFIX_WHITELIST="baidu.com baidu.cn bdimg.com bdstatic.com bdycdn.cn bcebos.com qq.com tencent.com tencentmusic.com gtimg.cn tencent-cloud.net weixin.com 163.com 126.net apple.com aliyun.com ntsc.ac.cn"

echo "$(date) - === Whitelist update start (pid=$$, tesla=$TESLA_IP) ===" > $LOGFILE

# --- Step 1: extract Tesla queries from dns.log ---
QUERIED=""
if [ -f "$DNSLOG" ] && [ "$TESLA_IP" != "__NOT_FOUND__" ]; then
  QUERIED=$(grep "query\[A\]" $DNSLOG 2>/dev/null | grep " $TESLA_IP\$" | awk '{print $6}' | sort -u)
fi

is_tesla_domain() {
  case "$1" in
    *.tesla.cn|tesla.cn|*.tesla.com|tesla.com|*.teslamotors.com|teslamotors.com|*.tesla.services|tesla.services) return 0 ;;
    *) return 1 ;;
  esac
}

is_tesla_exact() {
  for t in $TESLA_EXACT; do
    [ "$1" = "$t" ] && return 0
  done
  return 1
}

is_suffix_allowed() {
  for s in $SUFFIX_WHITELIST; do
    if [ "$1" = "$s" ]; then return 0; fi
    case "$1" in
      *.$s) return 0 ;;
    esac
  done
  return 1
}

# --- Step 2: classify ---
ALLOWED_DOMAINS=""
BLOCKED_DOMAINS=""

for d in $QUERIED; do
  if is_tesla_domain "$d"; then
    if is_tesla_exact "$d"; then
      ALLOWED_DOMAINS="$ALLOWED_DOMAINS $d"
    else
      BLOCKED_DOMAINS="$BLOCKED_DOMAINS $d"
    fi
  elif is_suffix_allowed "$d"; then
    ALLOWED_DOMAINS="$ALLOWED_DOMAINS $d"
  else
    BLOCKED_DOMAINS="$BLOCKED_DOMAINS $d"
  fi
done

# Always include tesla exact domains
for t in $TESLA_EXACT; do
  case " $ALLOWED_DOMAINS " in
    *" $t "*) ;;
    *) ALLOWED_DOMAINS="$ALLOWED_DOMAINS $t" ;;
  esac
done

echo "--- Allowed domains ---" >> $LOGFILE
for d in $ALLOWED_DOMAINS; do echo "  $d" >> $LOGFILE; done
echo "--- Blocked domains (review if you need any of these) ---" >> $LOGFILE
for d in $BLOCKED_DOMAINS; do echo "  $d" >> $LOGFILE; done

# --- Step 3: build IP list ---
TMPFILE="/tmp/wl_tmp.$$"
: > "$TMPFILE"

# Start with hardcoded connman
echo "$CONNMAN_FIXED_IP" >> "$TMPFILE"

# Merge existing persistent IPs (defensive: only valid IPv4)
if [ -f "$PERSIST_IPFILE" ]; then
  grep -E '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' "$PERSIST_IPFILE" >> "$TMPFILE" 2>/dev/null
fi

# Resolve all allowed domains
for d in $ALLOWED_DOMAINS; do
  # Parse nslookup output: works for both "Address 1: IP" and "Address: IP" formats
  ips=$(nslookup "$d" "$DNS" 2>/dev/null | awk '/^Address/ {for(i=1;i<=NF;i++) if($i ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) print $i}' | grep -v '^127\.')
  for ip in $ips; do
    echo "$ip" >> "$TMPFILE"
    echo "  $d -> $ip" >> $LOGFILE
  done
done

# Dedupe using sort | uniq (avoid busybox sort -u -o combo bug)
sort "$TMPFILE" | uniq > "$IPFILE"
rm -f "$TMPFILE"

# Ensure IPFILE exists even if empty
[ -f "$IPFILE" ] || : > "$IPFILE"

COUNT=$(wc -l < "$IPFILE" 2>/dev/null)
[ -z "$COUNT" ] && COUNT=0
echo "$(date) - Total unique IPs: $COUNT" >> $LOGFILE

# --- Step 4: rebuild DNSONLY atomically ---
while iptables -D FORWARD -j DNSONLY 2>/dev/null; do :; done
iptables -F DNSONLY 2>/dev/null
iptables -X DNSONLY 2>/dev/null
iptables -N DNSONLY

iptables -A DNSONLY -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A DNSONLY -p udp --dport 53 -j ACCEPT
iptables -A DNSONLY -p udp --sport 53 -j ACCEPT
iptables -A DNSONLY -p tcp --dport 53 -j ACCEPT
iptables -A DNSONLY -p tcp --sport 53 -j ACCEPT
iptables -A DNSONLY -p udp --dport 67:68 -j ACCEPT
iptables -A DNSONLY -p udp --dport 123 -j ACCEPT
iptables -A DNSONLY -d "$ROUTER_IP" -j ACCEPT

# HARDCODED connman (fallback even if IPFILE is empty)
iptables -A DNSONLY -d 116.129.226.175 -j ACCEPT

# Additional IPs (skip connman dedup)
if [ -s "$IPFILE" ]; then
  while read ip; do
    [ -z "$ip" ] && continue
    [ "$ip" = "116.129.226.175" ] && continue
    iptables -A DNSONLY -d "$ip" -j ACCEPT
  done < "$IPFILE"
fi

iptables -A DNSONLY -j DROP
iptables -I FORWARD 1 -j DNSONLY

echo "$(date) - Applied $COUNT IPs to DNSONLY" >> $LOGFILE

# --- Step 5: persist only if changed ---
if ! cmp -s "$IPFILE" "$PERSIST_IPFILE" 2>/dev/null; then
  cp "$IPFILE" "$PERSIST_IPFILE" 2>/dev/null
  sync
  echo "$(date) - Persisted to $PERSIST_IPFILE ($COUNT IPs)" >> $LOGFILE
fi

tail -500 $LOGFILE > /tmp/wl_log_tmp 2>/dev/null && mv /tmp/wl_log_tmp $LOGFILE
