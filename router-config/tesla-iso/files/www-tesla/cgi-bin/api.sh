#!/bin/sh
# Tesla ISO - 单 CGI 入口 v1.0
# 处理所有 web 请求, 通过 query string 的 action 参数分发
#
# 输入:
#   QUERY_STRING        action=xxx&...
#   REQUEST_METHOD      GET / POST
#   CONTENT_LENGTH      POST body 长度
#   stdin               POST body
#
# 输出: JSON (Content-Type: application/json)

set -u

ALLOWFILE=/etc/dnsmasq.d/tesla_iso.conf
DNSLOG=/tmp/dns.log
VERSION_FILE=/etc/tesla_iso/version
BACKUP_DIR=/etc/tesla_iso_backup
UPLOAD_PATH=/tmp/tesla_iso_upgrade.tar.gz
UPGRADE_LOG=/tmp/tesla_iso_upgrade.log
DHCP_LEASES=/tmp/dhcp.leases
SEEN_TSV=/tmp/tesla_iso/dns_seen.tsv
SEEN_PERSIST=/etc/tesla_iso/dns_seen.tsv
COLLECTOR=/usr/sbin/tesla-iso-collector.sh
MODE_FILE=/etc/tesla_iso/mode

# ============================================================================
# 工具函数
# ============================================================================

# 输出 HTTP 头
header_json() {
  printf 'Content-Type: application/json; charset=utf-8\r\n'
  printf 'Cache-Control: no-store\r\n'
  printf '\r\n'
}

# JSON 字符串转义
# 用法: json_str "some string"
json_str() {
  # \ -> \\, " -> \", 控制字符简单转空格
  printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/	/ /g' | tr -d '\r\n'
}

# 输出 ok 响应
ok_response() {
  header_json
  printf '{"ok":true%s}\n' "$1"
}

# 输出 error 响应
err_response() {
  header_json
  printf '{"ok":false,"error":"%s"}\n' "$(json_str "$1")"
}

# URL decode (busybox 兼容)
# 用法: echo "hello%20world" | url_decode
url_decode() {
  # %xx -> 字符; + -> 空格
  # busybox printf 支持 \xHH
  _s=$(cat)
  _s=$(printf '%s' "$_s" | sed 's/+/ /g')
  # 用 printf 配合 sed 把 %HH 转成 \xHH 形式再让 printf 解释
  printf '%b' "$(printf '%s' "$_s" | sed 's/\\/\\\\/g; s/%\([0-9A-Fa-f][0-9A-Fa-f]\)/\\x\1/g')"
}

# 从 form-urlencoded body 提取字段
# 用法: form_get "key" "raw_body"
form_get() {
  _key="$1"
  _body="$2"
  # 取出 key=value 的 value 部分
  _val=$(printf '%s' "$_body" | tr '&' '\n' | grep "^$_key=" | head -1 | sed "s/^$_key=//")
  printf '%s' "$_val" | url_decode
}

# 从 query string 提取参数
# 用法: query_get "key"
query_get() {
  _key="$1"
  printf '%s' "${QUERY_STRING:-}" | tr '&' '\n' | grep "^$_key=" | head -1 | sed "s/^$_key=//" | url_decode
}

# 读取 POST body 到字符串
read_post_body() {
  if [ -n "${CONTENT_LENGTH:-}" ] && [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null
  fi
}

# 读取 POST body 到文件
read_post_body_to_file() {
  _f="$1"
  if [ -n "${CONTENT_LENGTH:-}" ] && [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    dd bs=4096 count=$(( (CONTENT_LENGTH + 4095) / 4096 )) 2>/dev/null > "$_f"
  fi
}

# 校验域名格式: 必须是 tesla.cn / tesla.com / teslamotors.com / tesla.services 的子域
validate_tesla_domain() {
  _d="$1"
  case "$_d" in
    *.tesla.cn|*.tesla.com|*.teslamotors.com|*.teslamotors.cn|*.tesla.services) return 0 ;;
    tesla.cn|tesla.com|teslamotors.com|teslamotors.cn|tesla.services) return 0 ;;
    *) return 1 ;;
  esac
}

# 触发 dnsmasq 重载配置
reload_dnsmasq() {
  if pidof dnsmasq >/dev/null 2>&1; then
    kill -HUP $(pidof dnsmasq) 2>/dev/null
    return 0
  fi
  return 1
}

# ============================================================================
# 当前模式检测
# 返回值: safe / aggressive / custom
# ============================================================================
detect_mode() {
  if [ -f "$MODE_FILE" ]; then
    _saved=$(cat "$MODE_FILE" 2>/dev/null | tr -d ' \r\n')
    case "$_saved" in
      safe|aggressive) printf '%s' "$_saved"; return ;;
    esac
  fi
  printf 'custom'
}

# ============================================================================
# action: status
# ============================================================================
action_status() {
  _ver="-"
  [ -f "$VERSION_FILE" ] && _ver=$(cat "$VERSION_FILE" 2>/dev/null | head -1)

  _dnsmasq="stopped"
  pidof dnsmasq >/dev/null 2>&1 && _dnsmasq="running"

  _crond="stopped"
  pidof crond >/dev/null 2>&1 && _crond="running"

  _devices=0
  if [ -f "$DHCP_LEASES" ]; then
    _devices=$(wc -l < "$DHCP_LEASES" 2>/dev/null | tr -d ' ')
  fi

  _allow_count=0
  if [ -f "$ALLOWFILE" ]; then
    _allow_count=$(grep -c '^server=/' "$ALLOWFILE" 2>/dev/null || echo 0)
  fi

  _mode=$(detect_mode)

  header_json
  printf '{"version":"%s","dnsmasq":"%s","crond":"%s","devices":%s,"allow_count":%s,"mode":"%s"}\n' \
    "$(json_str "$_ver")" "$_dnsmasq" "$_crond" "$_devices" "$_allow_count" "$_mode"
}

# ============================================================================
# action: get_mode
# ============================================================================
action_get_mode() {
  header_json
  printf '{"mode":"%s"}\n' "$(detect_mode)"
}

# ============================================================================
# action: switch_mode
# POST body: mode=safe 或 mode=aggressive
# 重写 /etc/dnsmasq.d/tesla_iso.conf 为对应模板, 重启 dnsmasq
# ============================================================================
action_switch_mode() {
  _body=$(read_post_body)
  _mode=$(form_get "mode" "$_body")

  case "$_mode" in
    safe|aggressive) ;;
    *) err_response "invalid mode (must be safe or aggressive)"; return ;;
  esac

  # 重写 tesla_iso.conf
  if [ "$_mode" = "safe" ]; then
    cat > "$ALLOWFILE" <<'SAFE_EOF'
# Tesla ISO - 例外放行列表 (mode=safe 保守版)
# 此文件由 web 切换模式时自动重写, 手动编辑会被覆盖
#
# 保守版只放行 3 条 Tesla 系统例外, 最大化保护 FSD
#
# 可用功能 (保守版):
#   - WiFi 连接
#   - Tesla 中国导航 (含路口放大图)
#   - 在线音乐 (QQ 音乐/网易云/百度等, 走非 tesla 域名 默认放行)
#   - 微信通知
#   - 第三方地图 (百度/高德)
#
# 不可用功能 (保守版):
#   - 手机 App 控车 (Tesla 官方 App)
#   - Tesla 自带媒体服务 (剧院模式视频)
#   - 语音助手 "小 T"
#   - OTA 升级
#
# FSD 安全等级: 最高

# === 系统例外 (3 条, 必须) ===
server=/connman.vn.cloud.tesla.cn/114.114.114.114
server=/nav-prd-maps.tesla.cn/114.114.114.114
server=/maps-cn-prd.go.tesla.services/114.114.114.114
SAFE_EOF
  else
    cat > "$ALLOWFILE" <<'AGG_EOF'
# Tesla ISO - 例外放行列表 (mode=aggressive 激进版)
# 此文件由 web 切换模式时自动重写, 手动编辑会被覆盖
#
# 激进版在保守版基础上增加 4 条 (实测 work):
#   - 手机 App 控车 (hermes-prd)
#   - Hermes 信令握手 (signaling)
#   - 媒体服务 (media-server-me)
#   - Tesla 官网链接 (www.tesla.cn)
#
# 中等 FSD 风险:
#   - hermes-prd 是 Tesla 命令通道, 理论上 revoke 命令可能走它
#   - 但 revoke 流程需要协同其他通道 (apigateway-x2-trigger / api-prd / telemetry-prd / ownership)
#   - 这些协同通道仍然 sinkhole, 单条 hermes-prd 不足以完成 revoke

# === 系统例外 (3 条, 必须) ===
server=/connman.vn.cloud.tesla.cn/114.114.114.114
server=/nav-prd-maps.tesla.cn/114.114.114.114
server=/maps-cn-prd.go.tesla.services/114.114.114.114

# === 激进版例外 (4 条) ===
server=/hermes-prd.vn.cloud.tesla.cn/114.114.114.114
server=/signaling.vn.cloud.tesla.cn/114.114.114.114
server=/media-server-me.tesla.cn/114.114.114.114
server=/www.tesla.cn/114.114.114.114
AGG_EOF
  fi

  # 写入 mode 标记
  mkdir -p /etc/tesla_iso 2>/dev/null
  echo "$_mode" > "$MODE_FILE"

  # 重启 dnsmasq 让规则生效
  if [ -x /etc/init.d/dnsmasq ]; then
    /etc/init.d/dnsmasq restart >/dev/null 2>&1
  else
    killall dnsmasq 2>/dev/null
    sleep 1
    /usr/sbin/dnsmasq -C /etc/dnsmasq.conf 2>/dev/null
  fi
  sleep 2

  if pidof dnsmasq >/dev/null 2>&1; then
    header_json
    printf '{"ok":true,"mode":"%s"}\n' "$_mode"
  else
    err_response "dnsmasq did not restart after mode switch"
  fi
}

# ============================================================================
# action: list_allow
# ============================================================================
action_list_allow() {
  header_json
  printf '{"items":['
  _first=1
  _section="system"
  if [ -f "$ALLOWFILE" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
      case "$line" in
        *"USER EXCEPTIONS BELOW"*) _section="user" ;;
      esac
      case "$line" in
        server=/*)
          # 解析 server=/domain/upstream
          _domain=$(printf '%s' "$line" | sed 's|^server=/||; s|/.*||')
          # 注释从行尾的 # 之后取 (但 server= 行通常没有行内注释)
          _note=""
          [ "$_first" -eq 0 ] && printf ','
          printf '{"domain":"%s","note":"%s","type":"%s"}' \
            "$(json_str "$_domain")" "$(json_str "$_note")" "$_section"
          _first=0
          ;;
      esac
    done < "$ALLOWFILE"
  fi
  printf ']}\n'
}

# ============================================================================
# action: list_seen
# 从 dns_seen.tsv 读 collector 维护的车机查询档案
# 字段: domain / first_seen / last_seen / count / note (ALLOWED|WARN|"")
# ============================================================================
action_list_seen() {
  header_json
  # 选档案文件: 优先 runtime, 否则 persist
  _tsv=""
  if [ -f "$SEEN_TSV" ]; then
    _tsv="$SEEN_TSV"
  elif [ -f "$SEEN_PERSIST" ]; then
    _tsv="$SEEN_PERSIST"
  fi
  if [ -z "$_tsv" ]; then
    # 如果没有, 触发一次 collector 来生成
    if [ -x "$COLLECTOR" ]; then
      "$COLLECTOR" >/dev/null 2>&1
    fi
    [ -f "$SEEN_TSV" ] && _tsv="$SEEN_TSV"
  fi

  printf '{"items":['
  _first=1
  if [ -n "$_tsv" ] && [ -f "$_tsv" ]; then
    while IFS=$'\t' read -r d first last cnt note rest; do
      [ -z "$d" ] && continue
      case "$d" in '#'*) continue ;; esac
      [ "$_first" -eq 0 ] && printf ','
      printf '{"domain":"%s","first":"%s","last":"%s","count":%s,"note":"%s"}' \
        "$(json_str "$d")" "$(json_str "$first")" "$(json_str "$last")" "${cnt:-0}" "$(json_str "$note")"
      _first=0
    done < "$_tsv"
  fi
  printf ']}\n'
}

# ============================================================================
# action: list_pending  (保留以向后兼容旧前端, 实际转发到 list_seen)
# ============================================================================
action_list_pending() {
  action_list_seen
}

# ============================================================================
# action: export_seen
# 直接返回 TSV 文件供下载, 给作者/网友打包发回来分析
# ============================================================================
action_export_seen() {
  printf 'Content-Type: text/tab-separated-values; charset=utf-8\r\n'
  printf 'Content-Disposition: attachment; filename="dns_seen_%s.tsv"\r\n' "$(date +%Y%m%d_%H%M 2>/dev/null || echo export)"
  printf 'Cache-Control: no-store\r\n'
  printf '\r\n'
  if [ -f "$SEEN_TSV" ]; then
    cat "$SEEN_TSV"
  elif [ -f "$SEEN_PERSIST" ]; then
    cat "$SEEN_PERSIST"
  else
    echo "# no data yet"
  fi
}

# ============================================================================
# action: run_collector  (手动触发一次扫描)
# ============================================================================
action_run_collector() {
  if [ -x "$COLLECTOR" ]; then
    "$COLLECTOR" >/dev/null 2>&1
    ok_response ""
  else
    err_response "collector not installed"
  fi
}

# ============================================================================
# action: log_tail
# ============================================================================
action_log_tail() {
  header_json
  if [ ! -f "$DNSLOG" ]; then
    printf '{"lines":[]}\n'
    return
  fi
  printf '{"lines":['
  _first=1
  tail -30 "$DNSLOG" 2>/dev/null | while IFS= read -r line; do
    [ "$_first" -eq 0 ] && printf ','
    printf '"%s"' "$(json_str "$line")"
    _first=0
  done
  printf ']}\n'
}

# ============================================================================
# action: add_allow
# POST body: domain=xxx&note=yyy
# ============================================================================
action_add_allow() {
  _body=$(read_post_body)
  _domain=$(form_get "domain" "$_body")
  _note=$(form_get "note" "$_body")

  if [ -z "$_domain" ]; then
    err_response "domain required"
    return
  fi
  # 域名格式校验 (只允许小写字母数字点连字符)
  if ! printf '%s' "$_domain" | grep -qE '^[a-z0-9.-]+$'; then
    err_response "invalid domain format"
    return
  fi
  if ! validate_tesla_domain "$_domain"; then
    err_response "domain must be subdomain of tesla.cn/tesla.com/teslamotors.com/tesla.services"
    return
  fi

  # 检查是否已存在
  if grep -qE "^server=/$_domain/" "$ALLOWFILE" 2>/dev/null; then
    err_response "already in allowlist"
    return
  fi

  # 直接 append 到文件末尾 (避免 awk 误匹配注释里的 marker)
  _line="server=/$_domain/114.114.114.114"
  if [ -n "$_note" ]; then
    _line="$_line  # $_note"
  fi
  printf '%s\n' "$_line" >> "$ALLOWFILE"

  reload_dnsmasq
  ok_response ""
}

# ============================================================================
# action: del_allow
# POST body: domain=xxx
# ============================================================================
action_del_allow() {
  _body=$(read_post_body)
  _domain=$(form_get "domain" "$_body")

  if [ -z "$_domain" ]; then
    err_response "domain required"
    return
  fi
  if ! printf '%s' "$_domain" | grep -qE '^[a-z0-9.-]+$'; then
    err_response "invalid domain format"
    return
  fi

  # 系统例外保护: 不允许删除任何系统例外
  case "$_domain" in
    connman.vn.cloud.tesla.cn|nav-prd-maps.tesla.cn|maps-cn-prd.go.tesla.services)
      err_response "system exception cannot be removed"
      return
      ;;
  esac

  if [ ! -f "$ALLOWFILE" ]; then
    err_response "allowlist not found"
    return
  fi

  if ! grep -qE "^server=/$_domain/" "$ALLOWFILE"; then
    err_response "domain not in allowlist"
    return
  fi

  # 直接 grep -v 删除匹配的 server= 行 (整个文件范围)
  grep -v "^server=/$_domain/" "$ALLOWFILE" > "$ALLOWFILE.tmp" && mv "$ALLOWFILE.tmp" "$ALLOWFILE"

  reload_dnsmasq
  ok_response ""
}

# ============================================================================
# action: restart_dnsmasq
# ============================================================================
action_restart_dnsmasq() {
  if [ -x /etc/init.d/dnsmasq ]; then
    /etc/init.d/dnsmasq restart >/dev/null 2>&1
  else
    killall dnsmasq 2>/dev/null
    sleep 1
    /usr/sbin/dnsmasq -C /etc/dnsmasq.conf 2>/dev/null
  fi
  sleep 1
  if pidof dnsmasq >/dev/null 2>&1; then
    ok_response ""
  else
    err_response "dnsmasq did not start"
  fi
}

# ============================================================================
# action: upload_pkg
# POST body: 文件原始字节 (Content-Type: application/octet-stream)
# Query: name=xxx.tar.gz
# ============================================================================
action_upload_pkg() {
  _name=$(query_get "name")
  if [ -z "$_name" ]; then
    err_response "filename required"
    return
  fi
  read_post_body_to_file "$UPLOAD_PATH"
  if [ ! -s "$UPLOAD_PATH" ]; then
    err_response "upload failed (empty file)"
    return
  fi
  _size=$(wc -c < "$UPLOAD_PATH" 2>/dev/null | tr -d ' ')
  # 校验是合法 tar.gz
  if ! tar -tzf "$UPLOAD_PATH" >/dev/null 2>&1; then
    rm -f "$UPLOAD_PATH"
    err_response "not a valid tar.gz"
    return
  fi
  header_json
  printf '{"ok":true,"size":%s,"name":"%s"}\n' "$_size" "$(json_str "$_name")"
}

# ============================================================================
# action: install_upgrade
# 触发 upgrade.sh, 把日志写到 UPGRADE_LOG
# ============================================================================
action_install_upgrade() {
  if [ ! -f "$UPLOAD_PATH" ]; then
    err_response "no upload package found, upload first"
    return
  fi
  : > "$UPGRADE_LOG"
  # 解压到临时目录
  _workdir=/tmp/tesla_iso_upgrade_work
  rm -rf "$_workdir"
  mkdir -p "$_workdir"
  if ! tar -xzf "$UPLOAD_PATH" -C "$_workdir" 2>>"$UPGRADE_LOG"; then
    err_response "tar extract failed"
    return
  fi
  # 找到包根目录
  _pkgroot=$(find "$_workdir" -maxdepth 2 -name upgrade.sh -type f 2>/dev/null | head -1 | xargs dirname 2>/dev/null)
  if [ -z "$_pkgroot" ] || [ ! -f "$_pkgroot/upgrade.sh" ]; then
    err_response "upgrade.sh not found in package"
    return
  fi
  chmod +x "$_pkgroot/upgrade.sh"
  # 后台跑 upgrade.sh, 因为它会重启 web 自身
  (
    cd "$_pkgroot" && sh upgrade.sh >> "$UPGRADE_LOG" 2>&1
    rm -f "$UPLOAD_PATH"
  ) &
  sleep 1
  _log=$(tail -20 "$UPGRADE_LOG" 2>/dev/null)
  header_json
  printf '{"ok":true,"log":"%s"}\n' "$(json_str "$_log")"
}

# ============================================================================
# action: list_versions
# ============================================================================
action_list_versions() {
  header_json
  printf '{"items":['
  _first=1
  if [ -d "$BACKUP_DIR" ]; then
    for d in "$BACKUP_DIR"/*; do
      [ -d "$d" ] || continue
      _name=$(basename "$d")
      _ver=$(cat "$d/version.bak" 2>/dev/null || echo "?")
      _time=$(printf '%s' "$_name" | sed 's/.*_//')
      [ "$_first" -eq 0 ] && printf ','
      printf '{"version":"%s","time":"%s","dir":"%s"}' \
        "$(json_str "$_ver")" "$(json_str "$_time")" "$(json_str "$_name")"
      _first=0
    done
  fi
  printf ']}\n'
}

# ============================================================================
# action: rollback
# POST: dir=xxx (相对于 BACKUP_DIR)
# ============================================================================
action_rollback() {
  _body=$(read_post_body)
  _dir=$(form_get "dir" "$_body")
  if [ -z "$_dir" ]; then
    err_response "dir required"
    return
  fi
  # 防路径穿越
  case "$_dir" in
    */*|..*|/*) err_response "invalid dir"; return ;;
  esac
  _full="$BACKUP_DIR/$_dir"
  if [ ! -d "$_full" ]; then
    err_response "backup not found"
    return
  fi
  # 后台跑 rollback 脚本 (web 会重启)
  : > "$UPGRADE_LOG"
  if [ -x "$_full/rollback.sh" ]; then
    ( sh "$_full/rollback.sh" >> "$UPGRADE_LOG" 2>&1 ) &
  else
    # 没有 rollback.sh, fallback: 直接复制 files/
    (
      [ -f "$_full/dnsmasq.conf.bak" ] && cp "$_full/dnsmasq.conf.bak" /etc/dnsmasq.conf
      [ -f "$_full/firewall.user.bak" ] && cp "$_full/firewall.user.bak" /etc/firewall.user
      [ -f "$_full/tesla_iso.conf.bak" ] && cp "$_full/tesla_iso.conf.bak" /etc/dnsmasq.d/tesla_iso.conf
      [ -f "$_full/version.bak" ] && cp "$_full/version.bak" /etc/tesla_iso/version
      sh /etc/firewall.user 2>/dev/null
      /etc/init.d/dnsmasq restart 2>/dev/null || (killall dnsmasq 2>/dev/null; sleep 1; /usr/sbin/dnsmasq -C /etc/dnsmasq.conf)
    ) >> "$UPGRADE_LOG" 2>&1 &
  fi
  ok_response ""
}

# ============================================================================
# 路由分发
# ============================================================================
ACTION=$(query_get "action")
case "$ACTION" in
  status)           action_status ;;
  get_mode)         action_get_mode ;;
  switch_mode)      action_switch_mode ;;
  list_allow)       action_list_allow ;;
  list_seen)        action_list_seen ;;
  list_pending)     action_list_pending ;;
  export_seen)      action_export_seen ;;
  run_collector)    action_run_collector ;;
  log_tail)         action_log_tail ;;
  add_allow)        action_add_allow ;;
  del_allow)        action_del_allow ;;
  restart_dnsmasq)  action_restart_dnsmasq ;;
  upload_pkg)       action_upload_pkg ;;
  install_upgrade)  action_install_upgrade ;;
  list_versions)    action_list_versions ;;
  rollback)         action_rollback ;;
  *)
    header_json
    printf '{"ok":false,"error":"unknown action: %s"}\n' "$(json_str "${ACTION:-}")"
    ;;
esac
