#!/bin/sh
# Tesla ISO - 卸载脚本
# 完全清除 v1.0+ 方案, 从最早的 preinstall 备份恢复
#
# 用法:
#   sh uninstall.sh             交互式确认
#   sh uninstall.sh -y          非交互直接卸载

set -u

ASSUME_YES=0
[ "${1:-}" = "-y" ] && ASSUME_YES=1

echo "=================================================="
echo "  Tesla ISO Uninstaller"
echo "=================================================="
echo ""

# ---------- 找最早的 preinstall 备份 ----------
PREBACKUP=$(ls -d /etc/tesla_iso_backup/preinstall_* 2>/dev/null | sort | head -1)
if [ -z "$PREBACKUP" ]; then
  echo "  警告: 找不到首装备份 /etc/tesla_iso_backup/preinstall_*"
  echo "  将只清除 tesla-iso 文件, 不恢复原配置"
  if [ "$ASSUME_YES" = "0" ]; then
    printf "  继续? [y/N] "
    read _ans
    [ "$_ans" != "y" ] && [ "$_ans" != "Y" ] && exit 0
  fi
else
  echo "  找到首装备份: $PREBACKUP"
  if [ "$ASSUME_YES" = "0" ]; then
    printf "  从此备份恢复并卸载 tesla-iso? [y/N] "
    read _ans
    [ "$_ans" != "y" ] && [ "$_ans" != "Y" ] && { echo "  已取消"; exit 0; }
  fi
fi

# ---------- 停 web ----------
echo ""
echo ">>> 停止 web 控制台..."
[ -x /usr/sbin/tesla-iso-httpd-init.sh ] && /usr/sbin/tesla-iso-httpd-init.sh stop 2>/dev/null
echo "  OK"

# ---------- 移除 iptables INPUT 规则 ----------
echo ""
echo ">>> 移除 iptables 规则..."
while iptables -D INPUT -p tcp --dport 8888 -i br-lan -j ACCEPT 2>/dev/null; do :; done
while iptables -D INPUT -p tcp --dport 8888 -j DROP 2>/dev/null; do :; done
# IPv6 是否要保留 DROP? 卸载时恢复成不 DROP 才是干净
while ip6tables -D FORWARD -j DROP 2>/dev/null; do :; done
echo "  OK"

# ---------- 移除 cron 任务 ----------
echo ""
echo ">>> 移除 cron 任务..."
for cf in /etc/crontabs/*; do
  [ -f "$cf" ] || continue
  if grep -q "tesla-iso-collector.sh" "$cf" 2>/dev/null; then
    grep -v "tesla-iso-collector.sh" "$cf" > "$cf.tmp" && mv "$cf.tmp" "$cf"
    echo "  cleaned $cf"
  fi
done

# ---------- 移除文件 ----------
echo ""
echo ">>> 移除 tesla-iso 文件..."
rm -f /usr/sbin/tesla-iso-httpd-init.sh
rm -f /usr/sbin/tesla-iso-collector.sh
rm -rf /www-tesla
rm -rf /etc/tesla_iso
rm -rf /tmp/tesla_iso
rm -f /etc/dnsmasq.d/tesla_iso.conf
rm -f /tmp/tesla-iso-httpd.pid /tmp/tesla_iso_upgrade.tar.gz /tmp/tesla_iso_upgrade.log
rm -f /tmp/tesla_iso_collector.lock
rm -rf /tmp/tesla_iso_upgrade_work
echo "  OK"

# ---------- 恢复备份 ----------
if [ -n "$PREBACKUP" ] && [ -d "$PREBACKUP" ]; then
  echo ""
  echo ">>> 从 $PREBACKUP 恢复..."
  [ -f "$PREBACKUP/dnsmasq.conf.bak" ] && cp "$PREBACKUP/dnsmasq.conf.bak" /etc/dnsmasq.conf && echo "  /etc/dnsmasq.conf"
  [ -f "$PREBACKUP/firewall.user.bak" ] && cp "$PREBACKUP/firewall.user.bak" /etc/firewall.user && echo "  /etc/firewall.user"
  [ -f "$PREBACKUP/hosts.bak" ] && cp "$PREBACKUP/hosts.bak" /etc/hosts && echo "  /etc/hosts"
  if [ -d "$PREBACKUP/dnsmasq.d.bak" ]; then
    rm -rf /etc/dnsmasq.d
    cp -r "$PREBACKUP/dnsmasq.d.bak" /etc/dnsmasq.d
    echo "  /etc/dnsmasq.d"
  fi
fi

# ---------- 重启 dnsmasq ----------
echo ""
echo ">>> 重启 dnsmasq..."
if [ -x /etc/init.d/dnsmasq ]; then
  /etc/init.d/dnsmasq restart >/dev/null 2>&1
fi
sleep 1
if pgrep -x dnsmasq >/dev/null 2>&1; then
  echo "  OK"
else
  echo "  警告: dnsmasq 未启动"
fi

# ---------- 应用恢复的 firewall.user ----------
if [ -n "$PREBACKUP" ] && [ -f "$PREBACKUP/firewall.user.bak" ]; then
  echo ""
  echo ">>> 应用恢复的 firewall.user..."
  sh /etc/firewall.user 2>/dev/null
  echo "  OK"
fi

echo ""
echo "=================================================="
echo "  ✅ Tesla ISO 已卸载"
echo "=================================================="
echo ""
echo "  备份目录 /etc/tesla_iso_backup/ 仍然保留, 如需彻底清除:"
echo "    rm -rf /etc/tesla_iso_backup"
echo ""
