#!/bin/sh
# Tesla ISO - 升级脚本
# 由 web 上传升级包后自动调用, 也可以 telnet 进路由器手动跑
#
# 流程:
#   1. 检测当前版本和新版本
#   2. 备份当前所有受管文件到 /etc/tesla_iso_backup/<旧版本>_<时间戳>/
#   3. 跑 migrations/<from>-to-<to>.sh (如果存在)
#   4. 部署新 files/
#   5. 重启服务
#   6. 失败 → 自动回滚
#
# 用法:
#   cd /tmp/tesla_iso_upgrade_work/tesla-iso-x.x.x/
#   sh upgrade.sh

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -z "$SCRIPT_DIR" ] && SCRIPT_DIR="$(pwd)"
FILES_DIR="$SCRIPT_DIR/files"
NEW_VER=$(cat "$FILES_DIR/etc/tesla_iso/version" 2>/dev/null || echo "unknown")
OLD_VER=$(cat /etc/tesla_iso/version 2>/dev/null || echo "0.0.0")
TS=$(date +%Y%m%d_%H%M%S 2>/dev/null || echo manual)
BACKUP_DIR="/etc/tesla_iso_backup/${OLD_VER}_${TS}"

echo "=================================================="
echo "  Tesla ISO Upgrade: $OLD_VER → $NEW_VER"
echo "  $(date 2>/dev/null)"
echo "=================================================="

# ---------- 0. 完整性检查 ----------
echo ">>> 检查升级包完整性..."
for f in files/etc/dnsmasq.conf files/etc/firewall.user files/usr/sbin/tesla-iso-httpd-init.sh \
         files/www-tesla/index.html files/www-tesla/cgi-bin/api.sh; do
  if [ ! -f "$SCRIPT_DIR/$f" ]; then
    echo "  错误: 升级包损坏, 找不到 $f"
    exit 1
  fi
done
echo "  OK"

# ---------- 1. 备份当前 ----------
echo ""
echo ">>> 备份当前 v$OLD_VER 到 $BACKUP_DIR ..."
mkdir -p "$BACKUP_DIR" || { echo "  错误: 无法创建备份目录"; exit 1; }

[ -f /etc/dnsmasq.conf ]                       && cp /etc/dnsmasq.conf "$BACKUP_DIR/dnsmasq.conf.bak"
[ -f /etc/firewall.user ]                      && cp /etc/firewall.user "$BACKUP_DIR/firewall.user.bak"
[ -f /etc/dnsmasq.d/tesla_iso.conf ]           && cp /etc/dnsmasq.d/tesla_iso.conf "$BACKUP_DIR/tesla_iso.conf.bak"
[ -f /usr/sbin/tesla-iso-httpd-init.sh ]       && cp /usr/sbin/tesla-iso-httpd-init.sh "$BACKUP_DIR/httpd-init.sh.bak"
[ -f /www-tesla/index.html ]                   && cp /www-tesla/index.html "$BACKUP_DIR/index.html.bak"
[ -f /www-tesla/cgi-bin/api.sh ]               && cp /www-tesla/cgi-bin/api.sh "$BACKUP_DIR/api.sh.bak"
[ -f /etc/tesla_iso/version ]                  && cp /etc/tesla_iso/version "$BACKUP_DIR/version.bak"
echo "$OLD_VER" > "$BACKUP_DIR/version.bak"

# 生成 rollback 脚本到备份目录
cat > "$BACKUP_DIR/rollback.sh" <<'EOF'
#!/bin/sh
# Auto-generated rollback script
BAK="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
[ -f "$BAK/dnsmasq.conf.bak" ] && cp "$BAK/dnsmasq.conf.bak" /etc/dnsmasq.conf
[ -f "$BAK/firewall.user.bak" ] && cp "$BAK/firewall.user.bak" /etc/firewall.user
[ -f "$BAK/tesla_iso.conf.bak" ] && cp "$BAK/tesla_iso.conf.bak" /etc/dnsmasq.d/tesla_iso.conf
[ -f "$BAK/httpd-init.sh.bak" ] && cp "$BAK/httpd-init.sh.bak" /usr/sbin/tesla-iso-httpd-init.sh && chmod +x /usr/sbin/tesla-iso-httpd-init.sh
[ -f "$BAK/index.html.bak" ] && cp "$BAK/index.html.bak" /www-tesla/index.html
[ -f "$BAK/api.sh.bak" ] && cp "$BAK/api.sh.bak" /www-tesla/cgi-bin/api.sh && chmod +x /www-tesla/cgi-bin/api.sh
[ -f "$BAK/version.bak" ] && cp "$BAK/version.bak" /etc/tesla_iso/version
sh /etc/firewall.user 2>/dev/null
if [ -x /etc/init.d/dnsmasq ]; then
  /etc/init.d/dnsmasq restart >/dev/null 2>&1
else
  killall dnsmasq 2>/dev/null
  sleep 1
  /usr/sbin/dnsmasq -C /etc/dnsmasq.conf 2>/dev/null
fi
/usr/sbin/tesla-iso-httpd-init.sh restart 2>/dev/null
echo "Rollback complete"
EOF
chmod +x "$BACKUP_DIR/rollback.sh"
echo "  备份完成"

# ---------- 2. 跑 migrations ----------
echo ""
echo ">>> 检查迁移脚本..."
MIG_SCRIPT="$SCRIPT_DIR/migrations/${OLD_VER}-to-${NEW_VER}.sh"
if [ -f "$MIG_SCRIPT" ]; then
  echo "  跑 migrations/${OLD_VER}-to-${NEW_VER}.sh"
  if ! sh "$MIG_SCRIPT"; then
    echo "  错误: migration 失败, 准备回滚..."
    sh "$BACKUP_DIR/rollback.sh"
    exit 1
  fi
else
  echo "  无需迁移 (无 ${OLD_VER}-to-${NEW_VER}.sh)"
fi

# ---------- 3. 部署新 files ----------
echo ""
echo ">>> 部署 v$NEW_VER 文件..."
ROLLBACK=0

deploy() {
  _src="$1"
  _dst="$2"
  _mode="$3"
  if [ ! -f "$_src" ]; then
    return 0
  fi
  cp "$_src" "$_dst.new" || { echo "  失败: $_dst"; ROLLBACK=1; return 1; }
  mv "$_dst.new" "$_dst" || { echo "  失败 mv: $_dst"; ROLLBACK=1; return 1; }
  chmod "$_mode" "$_dst" 2>/dev/null
  echo "  $_dst"
}

mkdir -p /etc/dnsmasq.d /etc/tesla_iso /usr/sbin /www-tesla/cgi-bin 2>/dev/null

deploy "$FILES_DIR/etc/dnsmasq.conf"                /etc/dnsmasq.conf                644
deploy "$FILES_DIR/etc/firewall.user"               /etc/firewall.user               755
deploy "$FILES_DIR/usr/sbin/tesla-iso-httpd-init.sh" /usr/sbin/tesla-iso-httpd-init.sh 755
deploy "$FILES_DIR/www-tesla/index.html"            /www-tesla/index.html            644
deploy "$FILES_DIR/www-tesla/cgi-bin/api.sh"        /www-tesla/cgi-bin/api.sh        755

# tesla_iso.conf 特殊处理: 保留用户已经加的例外
# 策略: 取新版本的 SYSTEM 段 + 旧版本的 USER 段
if [ -f /etc/dnsmasq.d/tesla_iso.conf ] && [ -f "$FILES_DIR/etc/dnsmasq.d/tesla_iso.conf" ]; then
  echo "  合并 tesla_iso.conf (保留用户例外)..."
  _newconf="$FILES_DIR/etc/dnsmasq.d/tesla_iso.conf"
  _oldconf=/etc/dnsmasq.d/tesla_iso.conf
  _merged=/tmp/tesla_iso_merged.conf
  # 新版本的 SYSTEM 段 (含 SYSTEM EXCEPTIONS 行到 USER EXCEPTIONS BELOW 行)
  awk '
    /USER EXCEPTIONS BELOW/ { print; exit }
    { print }
  ' "$_newconf" > "$_merged"
  # 加入旧版本的 USER 段 (USER EXCEPTIONS BELOW 行之后的 server= 行)
  awk '
    /USER EXCEPTIONS BELOW/ { in_user=1; next }
    in_user && /^server=/ { print }
  ' "$_oldconf" >> "$_merged"
  cp "$_merged" "$_oldconf"
  rm -f "$_merged"
  echo "  /etc/dnsmasq.d/tesla_iso.conf (merged)"
elif [ -f "$FILES_DIR/etc/dnsmasq.d/tesla_iso.conf" ]; then
  cp "$FILES_DIR/etc/dnsmasq.d/tesla_iso.conf" /etc/dnsmasq.d/tesla_iso.conf
  echo "  /etc/dnsmasq.d/tesla_iso.conf (新建)"
fi

# 写入新版本号
echo "$NEW_VER" > /etc/tesla_iso/version

if [ "$ROLLBACK" = "1" ]; then
  echo ""
  echo ">>> 部署失败, 自动回滚..."
  sh "$BACKUP_DIR/rollback.sh"
  exit 1
fi

# ---------- 4. 重启服务 ----------
echo ""
echo ">>> 重启服务..."
if [ -x /etc/init.d/dnsmasq ]; then
  /etc/init.d/dnsmasq restart >/dev/null 2>&1
else
  killall dnsmasq 2>/dev/null
  sleep 1
  /usr/sbin/dnsmasq -C /etc/dnsmasq.conf 2>/dev/null
fi
sleep 1

sh /etc/firewall.user 2>/dev/null
sleep 1

/usr/sbin/tesla-iso-httpd-init.sh restart 2>/dev/null

# ---------- 5. 自检 ----------
echo ""
echo ">>> 升级后自检..."
if pgrep -x dnsmasq >/dev/null 2>&1; then
  echo "  [OK] dnsmasq"
else
  echo "  [FAIL] dnsmasq 未启动, 回滚..."
  sh "$BACKUP_DIR/rollback.sh"
  exit 1
fi

if /usr/sbin/tesla-iso-httpd-init.sh status >/dev/null 2>&1; then
  echo "  [OK] httpd"
else
  echo "  [WARN] httpd 未启动 (可能需要手动跑 firewall.user)"
fi

# DNS 解析自检
_test=$(nslookup hermes-prd.vn.cloud.tesla.cn 127.0.0.1 2>/dev/null | awk '/^Address/{for(i=1;i<=NF;i++) if($i ~ /^127\.0\.0\.1$/) {print "OK"; exit}}')
if [ "$_test" = "OK" ]; then
  echo "  [OK] hermes-prd 仍被 sinkhole"
else
  echo "  [WARN] hermes-prd sinkhole 校验失败"
fi

echo ""
echo "=================================================="
echo "  ✅ 升级成功: v$OLD_VER → v$NEW_VER"
echo "=================================================="
echo "  备份: $BACKUP_DIR"
echo "  回滚命令: sh $BACKUP_DIR/rollback.sh"
echo "  Web 自动会重启, 请手动刷新页面"
echo ""
exit 0
