#!/bin/sh
# Tesla WiFi Isolation - Uninstaller
# Run this ON THE ROUTER to remove all changes and restore the latest backup.

echo "=================================================="
echo "  Tesla WiFi Isolation Uninstaller"
echo "=================================================="
echo ""

# ---------- Step 1: Find latest backup ----------
BACKUP_DIR=$(ls -dt /etc/tesla-wifi-backup-* 2>/dev/null | head -1)
if [ -z "$BACKUP_DIR" ]; then
    echo "  WARNING: no backup found, will only remove current config"
    echo "  (You can Ctrl-C now to abort)"
    sleep 3
else
    echo "  Found backup: $BACKUP_DIR"
    printf "  Restore from this backup? [y/N] "
    read _answer
    if [ "$_answer" != "y" ] && [ "$_answer" != "Y" ]; then
        echo "  Aborted."
        exit 0
    fi
fi

# ---------- Step 2: Remove iptables rules ----------
echo ""
echo ">>> Removing iptables rules..."
while iptables -D FORWARD -j DNSONLY 2>/dev/null; do :; done
iptables -F DNSONLY 2>/dev/null
iptables -X DNSONLY 2>/dev/null
while ip6tables -D FORWARD -j DROP 2>/dev/null; do :; done
echo "  OK"

# ---------- Step 3: Remove cron entry ----------
echo ""
echo ">>> Removing cron entries..."
for cf in /etc/crontabs/*; do
    [ -f "$cf" ] || continue
    if grep -q "tesla_whitelist.sh" "$cf" 2>/dev/null; then
        grep -v "tesla_whitelist.sh" "$cf" > "$cf.tmp" && mv "$cf.tmp" "$cf"
        echo "  Cleaned $cf"
    fi
done

# ---------- Step 4: Remove installed files ----------
echo ""
echo ">>> Removing installed files..."
rm -f /etc/tesla_whitelist.sh
rm -f /etc/whitelist_ips.txt
rm -f /tmp/tesla_whitelist.lock
rm -f /tmp/whitelist_ips.txt
rm -f /tmp/whitelist.log
echo "  OK"

# ---------- Step 5: Restore backup ----------
if [ -n "$BACKUP_DIR" ] && [ -d "$BACKUP_DIR" ]; then
    echo ""
    echo ">>> Restoring from backup..."
    [ -f "$BACKUP_DIR/dnsmasq.conf.bak" ] && cp "$BACKUP_DIR/dnsmasq.conf.bak" /etc/dnsmasq.conf && echo "  dnsmasq.conf restored"
    [ -f "$BACKUP_DIR/firewall.user.bak" ] && cp "$BACKUP_DIR/firewall.user.bak" /etc/firewall.user && echo "  firewall.user restored"
    [ -f "$BACKUP_DIR/hosts.bak" ] && cp "$BACKUP_DIR/hosts.bak" /etc/hosts && echo "  hosts restored"
    if [ -d "$BACKUP_DIR/crontabs.bak" ]; then
        rm -rf /etc/crontabs
        cp -r "$BACKUP_DIR/crontabs.bak" /etc/crontabs
        echo "  crontabs restored"
    fi
else
    echo ""
    echo ">>> No backup to restore. Manually verify:"
    echo "     /etc/dnsmasq.conf (restore from your own backup or router reset)"
    echo "     /etc/firewall.user"
    echo "     /etc/hosts"
fi

# ---------- Step 6: Restart services ----------
echo ""
echo ">>> Restarting services..."
if [ -x /etc/init.d/dnsmasq ]; then
    /etc/init.d/dnsmasq restart >/dev/null 2>&1
fi
# Only apply firewall.user if we successfully restored from backup
# (otherwise the current /etc/firewall.user may still be our installer version)
if [ -n "$BACKUP_DIR" ] && [ -f "$BACKUP_DIR/firewall.user.bak" ] && [ -f /etc/firewall.user ]; then
    sh /etc/firewall.user 2>/dev/null
fi
# Kill the crond we started (only if user had no cron entries anymore)
if [ -d /etc/crontabs ] && [ -z "$(ls -A /etc/crontabs 2>/dev/null)" ]; then
    killall crond 2>/dev/null
fi
echo "  OK"

echo ""
echo "=================================================="
echo "  Uninstall complete"
echo "=================================================="
echo ""
echo "  Tesla WiFi isolation has been removed."
echo "  Your router's original DNS/firewall behavior is restored (if a backup was available)."
echo ""
