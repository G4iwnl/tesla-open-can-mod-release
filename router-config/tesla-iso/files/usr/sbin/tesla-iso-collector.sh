#!/bin/sh
# Tesla ISO - DNS 查询采集器 v1.0.1
#
# 功能:
#   1. 增量扫 /tmp/dns.log 提取所有 *.tesla.* / *.teslamotors.* 的查询
#   2. 维护 /tmp/tesla_iso/dns_seen.tsv (高频更新, 每 5 分钟一次)
#   3. 每小时 sync 一次到 /etc/tesla_iso/dns_seen.tsv (持久化, 仅在变化时写)
#   4. 自动截断 /tmp/dns.log 到尾 2000 行 (防 tmpfs 爆)
#   5. 自动给疑似危险域名打 ⚠ 标 (hermes/api-prd/telemetry/trigger/x2/config/...)
#
# 调用方式:
#   cron 每 5 分钟: */5 * * * * /usr/sbin/tesla-iso-collector.sh
#   或手动: sh /usr/sbin/tesla-iso-collector.sh
#
# 数据格式 (TSV):
#   domain<TAB>first_seen<TAB>last_seen<TAB>count<TAB>note

set -u

DNS_LOG=/tmp/dns.log
RUNTIME_DIR=/tmp/tesla_iso
RUNTIME_TSV="$RUNTIME_DIR/dns_seen.tsv"
PERSIST_DIR=/etc/tesla_iso
PERSIST_TSV="$PERSIST_DIR/dns_seen.tsv"
SYNC_COUNTER="$RUNTIME_DIR/sync_counter"
LOCKFILE=/tmp/tesla_iso_collector.lock
ALLOWFILE=/etc/dnsmasq.d/tesla_iso.conf
LOG_TRUNC_LINES=2000
SYNC_EVERY=12   # 每 12 次跑 (12 × 5min = 60min) sync 一次到 flash

# --- PID 锁防并发 ---
if [ -f "$LOCKFILE" ]; then
  _old=$(cat "$LOCKFILE" 2>/dev/null)
  if [ -n "$_old" ] && [ -d "/proc/$_old" ]; then
    exit 0
  fi
fi
echo $$ > "$LOCKFILE"
trap 'rm -f "$LOCKFILE"' EXIT INT TERM

# --- 初始化目录 ---
mkdir -p "$RUNTIME_DIR" "$PERSIST_DIR"

# --- 启动时如果 runtime tsv 不存在, 从 persist 加载 ---
if [ ! -f "$RUNTIME_TSV" ] && [ -f "$PERSIST_TSV" ]; then
  cp "$PERSIST_TSV" "$RUNTIME_TSV"
fi
[ -f "$RUNTIME_TSV" ] || : > "$RUNTIME_TSV"

# --- 当前时间戳 (BusyBox date 简单格式) ---
NOW=$(date '+%Y-%m-%d %H:%M' 2>/dev/null)
[ -z "$NOW" ] && NOW="-"

# --- 提取 dns.log 中的 tesla 查询 ---
# dnsmasq query 行格式:
#   Apr  8 09:00:01 dnsmasq[1234]: query[A] hermes-prd.vn.cloud.tesla.cn from 192.168.2.193
# 我们只取 query[A] 行的第 6 列 (域名), 过滤 tesla 相关
TMP_QUERIES=/tmp/tesla_iso_collector_q.$$
if [ -f "$DNS_LOG" ]; then
  grep -E 'query\[(A|AAAA)\]' "$DNS_LOG" 2>/dev/null \
    | awk '{print $6}' \
    | grep -iE '\.(tesla\.(cn|com|services)|teslamotors\.(cn|com))$|^(tesla\.(cn|com|services)|teslamotors\.(cn|com))$' \
    | tr 'A-Z' 'a-z' \
    | sort | uniq -c \
    | awk '{count=$1; $1=""; sub(/^ /,""); print $0"\t"count}' \
    > "$TMP_QUERIES"
fi
# 现在 TMP_QUERIES 每行: domain<TAB>count_in_this_window

# --- 危险关键词模式 (用于自动 note 标注) ---
# 命中任一关键词 -> note 字段加 "WARN"
# 用户后续可以用 web 上的"仅看 ⚠"过滤
is_dangerous() {
  case "$1" in
    *hermes*|*telemetry*|*trigger*|*mothership*) return 0 ;;
    *api-prd*|*api.prd*|*-x2-*|*x2-*) return 0 ;;
    *config*|*entitlement*|*license*|*vault*|*feature-flag*|*revoke*) return 0 ;;
    *str0*|*str1*|*streamer*) return 0 ;;
  esac
  return 1
}

# --- 加载已放行域名集 (用于 note 标注 ALLOWED) ---
TMP_ALLOW=/tmp/tesla_iso_collector_allow.$$
if [ -f "$ALLOWFILE" ]; then
  grep '^server=/' "$ALLOWFILE" 2>/dev/null | sed 's|^server=/||; s|/.*||' | sort -u > "$TMP_ALLOW"
else
  : > "$TMP_ALLOW"
fi

# --- 合并: 跟现有 RUNTIME_TSV 比对, 增量更新 ---
TMP_MERGED=/tmp/tesla_iso_collector_merged.$$
: > "$TMP_MERGED"

# 已有的域名集
TMP_EXIST=/tmp/tesla_iso_collector_exist.$$
if [ -f "$RUNTIME_TSV" ]; then
  awk -F'\t' '!/^#/ && NF>=4 {print $1}' "$RUNTIME_TSV" | sort -u > "$TMP_EXIST"
else
  : > "$TMP_EXIST"
fi

# 第一遍: 已有的行, 更新 last_seen 和 count
if [ -s "$RUNTIME_TSV" ]; then
  while IFS=$'\t' read -r d first last cnt note rest; do
    [ -z "$d" ] && continue
    case "$d" in '#'*) printf '%s\n' "$d" >> "$TMP_MERGED"; continue ;; esac
    # 在新查询中是否出现?
    _new_count=$(awk -F'\t' -v dom="$d" '$1==dom {print $2; exit}' "$TMP_QUERIES" 2>/dev/null)
    if [ -n "$_new_count" ] && [ "$_new_count" -gt 0 ] 2>/dev/null; then
      # 更新 count + last_seen
      cnt=$((cnt + _new_count))
      last="$NOW"
    fi
    # 重新计算 note
    _note=""
    if grep -qx "$d" "$TMP_ALLOW" 2>/dev/null; then
      _note="ALLOWED"
    elif is_dangerous "$d"; then
      _note="WARN"
    fi
    printf '%s\t%s\t%s\t%s\t%s\n' "$d" "$first" "$last" "$cnt" "$_note" >> "$TMP_MERGED"
  done < "$RUNTIME_TSV"
fi

# 第二遍: 新出现的域名, 加新行
if [ -s "$TMP_QUERIES" ]; then
  while IFS=$'\t' read -r d cnt; do
    [ -z "$d" ] && continue
    if grep -qx "$d" "$TMP_EXIST" 2>/dev/null; then
      continue
    fi
    _note=""
    if grep -qx "$d" "$TMP_ALLOW" 2>/dev/null; then
      _note="ALLOWED"
    elif is_dangerous "$d"; then
      _note="WARN"
    fi
    printf '%s\t%s\t%s\t%s\t%s\n' "$d" "$NOW" "$NOW" "$cnt" "$_note" >> "$TMP_MERGED"
  done < "$TMP_QUERIES"
fi

# 写头部注释
TMP_FINAL=/tmp/tesla_iso_collector_final.$$
{
  echo "# Tesla ISO - DNS Seen Database"
  echo "# domain	first_seen	last_seen	count	note"
  echo "# note 含义: ALLOWED=已放行 / WARN=疑似危险 / 空=普通"
  grep -v '^#' "$TMP_MERGED" 2>/dev/null | sort -t$'\t' -k4 -rn
} > "$TMP_FINAL"

# 原子替换 RUNTIME_TSV
mv "$TMP_FINAL" "$RUNTIME_TSV"
rm -f "$TMP_QUERIES" "$TMP_ALLOW" "$TMP_EXIST" "$TMP_MERGED"

# --- 截断 dns.log 防 tmpfs 爆 ---
if [ -f "$DNS_LOG" ]; then
  _lines=$(wc -l < "$DNS_LOG" 2>/dev/null | tr -d ' ')
  if [ -n "$_lines" ] && [ "$_lines" -gt "$LOG_TRUNC_LINES" ] 2>/dev/null; then
    tail -$LOG_TRUNC_LINES "$DNS_LOG" > "$DNS_LOG.tmp" 2>/dev/null && mv "$DNS_LOG.tmp" "$DNS_LOG"
  fi
fi

# --- 每 SYNC_EVERY 次 sync 到 flash (cmp 防写) ---
_count=0
[ -f "$SYNC_COUNTER" ] && _count=$(cat "$SYNC_COUNTER" 2>/dev/null)
[ -z "$_count" ] && _count=0
_count=$((_count + 1))
if [ "$_count" -ge "$SYNC_EVERY" ]; then
  if ! cmp -s "$RUNTIME_TSV" "$PERSIST_TSV" 2>/dev/null; then
    cp "$RUNTIME_TSV" "$PERSIST_TSV" 2>/dev/null
    sync 2>/dev/null
  fi
  _count=0
fi
echo "$_count" > "$SYNC_COUNTER"

exit 0
