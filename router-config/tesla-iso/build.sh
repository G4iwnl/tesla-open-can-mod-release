#!/bin/sh
# Tesla ISO - 打包脚本 (在 mac/linux 上跑)
# 把 tesla-iso/ 目录打成 tesla-iso-<version>.tar.gz
#
# 用法:
#   cd router-config/tesla-iso
#   sh build.sh

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" 2>/dev/null && pwd)"
cd "$SCRIPT_DIR"

VERSION=$(cat files/etc/tesla_iso/version 2>/dev/null | head -1 | tr -d ' \r\n')
if [ -z "$VERSION" ]; then
  echo "ERROR: cannot read version from files/etc/tesla_iso/version"
  exit 1
fi

PKGNAME="tesla-iso-$VERSION"
OUTDIR="$SCRIPT_DIR/dist"
mkdir -p "$OUTDIR"

# 准备临时打包目录
TMPDIR="$(mktemp -d)"
trap "rm -rf $TMPDIR" EXIT INT TERM

PKGROOT="$TMPDIR/$PKGNAME"
mkdir -p "$PKGROOT"

# 拷贝需要的内容
cp -r files "$PKGROOT/"
cp -r migrations "$PKGROOT/" 2>/dev/null || mkdir -p "$PKGROOT/migrations"
cp manifest.txt "$PKGROOT/"
cp install.sh upgrade.sh uninstall.sh "$PKGROOT/"
[ -f README.md ] && cp README.md "$PKGROOT/"

# 设置可执行位
chmod +x "$PKGROOT/install.sh" "$PKGROOT/upgrade.sh" "$PKGROOT/uninstall.sh"
chmod +x "$PKGROOT/files/etc/firewall.user"
chmod +x "$PKGROOT/files/usr/sbin/tesla-iso-httpd-init.sh"
chmod +x "$PKGROOT/files/www-tesla/cgi-bin/api.sh"

# 打包
OUTFILE="$OUTDIR/$PKGNAME.tar.gz"
( cd "$TMPDIR" && tar -czf "$OUTFILE" "$PKGNAME" )

# sha256
if command -v sha256sum >/dev/null 2>&1; then
  SHA=$(sha256sum "$OUTFILE" | awk '{print $1}')
elif command -v shasum >/dev/null 2>&1; then
  SHA=$(shasum -a 256 "$OUTFILE" | awk '{print $1}')
else
  SHA="(no sha256 tool)"
fi

SIZE=$(wc -c < "$OUTFILE" | tr -d ' ')

echo ""
echo "=================================================="
echo "  ✅ Build success"
echo "=================================================="
echo "  Package : $OUTFILE"
echo "  Version : $VERSION"
echo "  Size    : $SIZE bytes"
echo "  SHA256  : $SHA"
echo ""
echo "  下一步:"
echo "  1. 把 $PKGNAME.tar.gz 发给用户"
echo "  2. 用户在 web 升级页拖入安装"
echo "     首次安装: 用户跑 install.sh"
echo ""
