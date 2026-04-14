#!/bin/bash
# Tesla ISO 域名检测脚本 (Mac/Linux)
#
# 用 dig @<router_ip> 检测每个 Tesla 域名的解析结果是否符合预期:
#   - 危险通道  → 必须 sinkhole 到 127.0.0.1
#   - 系统例外  → 必须解析到真实 IP
#   - 激进版例外 → mode=safe 时 sinkhole, mode=aggressive 时 real
#   - 非 Tesla 域名 → 必须正常解析 (验证 DNS 全开)
#
# 用法:
#   sh verify.sh                              # 默认 192.168.2.254
#   sh verify.sh --router-ip 192.168.x.x
#   sh verify.sh --mode safe                  # 强制按 safe 模式判定预期
#   sh verify.sh --mode aggressive

set -e

ROUTER_IP="${ROUTER_IP:-192.168.2.254}"
MODE_OVERRIDE=""

while [ $# -gt 0 ]; do
  case "$1" in
    --router-ip) ROUTER_IP="$2"; shift 2 ;;
    --mode)      MODE_OVERRIDE="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,17p' "$0" | sed 's/^# \?//'
      exit 0
      ;;
    *) echo "未知参数: $1"; exit 1 ;;
  esac
done

# proxychains 兼容
CURL="env DYLD_INSERT_LIBRARIES= curl"
DIG="env DYLD_INSERT_LIBRARIES= dig"

# 检查依赖
if ! command -v dig >/dev/null 2>&1; then
  echo "❌ ERROR: 缺少 dig 命令 (mac 自带, brew install bind 也可)"
  exit 1
fi

# 拿当前 mode
if [ -n "$MODE_OVERRIDE" ]; then
  MODE="$MODE_OVERRIDE"
else
  MODE=$($CURL -sS --max-time 5 "http://$ROUTER_IP:8888/cgi-bin/api.sh?action=get_mode" 2>/dev/null \
    | grep -oE '"mode":"[^"]+"' | cut -d'"' -f4)
  [ -z "$MODE" ] && MODE="unknown"
fi

cat <<INFO
==================================================
  Tesla ISO 域名检测
==================================================
  Router IP : $ROUTER_IP
  当前模式  : $MODE
==================================================

INFO

PASS=0
FAIL=0

# 测试一个域名 (IPv4 / A 记录)
# $1 = 域名, $2 = 期望 (sinkhole|real), $3 = 描述
test_domain() {
  _domain="$1"
  _expected="$2"
  _desc="$3"

  _actual_ip=$($DIG +short +time=3 +tries=1 @"$ROUTER_IP" "$_domain" 2>/dev/null \
    | grep -E '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' | head -1)

  if [ -z "$_actual_ip" ]; then
    _got="NXDOMAIN"
  elif [ "$_actual_ip" = "127.0.0.1" ]; then
    _got="sinkhole"
  else
    _got="real ($_actual_ip)"
  fi

  if [ "$_expected" = "sinkhole" ] && [ "$_actual_ip" = "127.0.0.1" ]; then
    _result="✅"
    PASS=$((PASS + 1))
  elif [ "$_expected" = "real" ] && [ -n "$_actual_ip" ] && [ "$_actual_ip" != "127.0.0.1" ]; then
    _result="✅"
    PASS=$((PASS + 1))
  else
    _result="❌"
    FAIL=$((FAIL + 1))
  fi

  printf "  %s  %-48s  期望: %-9s  实际: %-25s  %s\n" \
    "$_result" "$_domain" "$_expected" "$_got" "$_desc"
}

# 测试一个域名的 IPv6 / AAAA 记录
# $1 = 域名, $2 = 期望 (sinkhole=:: 或 real=非 ::), $3 = 描述
# 期望 sinkhole 时,接受 :: 或 NODATA (空) 都算 PASS,
# 因为 Tesla 当前没开 IPv6,空响应也是安全的
test_domain_v6() {
  _domain="$1"
  _expected="$2"
  _desc="$3"

  _actual_ip=$($DIG +short +time=3 +tries=1 AAAA @"$ROUTER_IP" "$_domain" 2>/dev/null \
    | grep -E '^[0-9a-f:]+$' | head -1)

  if [ -z "$_actual_ip" ]; then
    _got="NODATA"
  elif [ "$_actual_ip" = "::" ] || [ "$_actual_ip" = "::1" ]; then
    _got="sinkhole ($_actual_ip)"
  else
    _got="real ($_actual_ip)"
  fi

  # 只对 sinkhole 期望做验证 (real 不测,因为 Tesla 暂时没 AAAA)
  if [ "$_expected" = "sinkhole" ]; then
    # PASS 条件: 显式 sinkhole (::) OR NODATA (空响应,Tesla 没开 IPv6)
    if [ "$_actual_ip" = "::" ] || [ "$_actual_ip" = "::1" ] || [ -z "$_actual_ip" ]; then
      _result="✅"
      PASS=$((PASS + 1))
    else
      _result="❌"
      FAIL=$((FAIL + 1))
    fi
  else
    _result="ℹ"
  fi

  printf "  %s  %-48s  AAAA 期望: %-7s 实际: %-22s  %s\n" \
    "$_result" "$_domain" "$_expected" "$_got" "$_desc"
}

echo "── 系统例外 (任何模式都必须放行) ──"
test_domain "connman.vn.cloud.tesla.cn"          "real"     "WiFi 验证"
test_domain "nav-prd-maps.tesla.cn"              "real"     "中国导航"
test_domain "maps-cn-prd.go.tesla.services"      "real"     "导航 CNAME 上游"

echo ""
echo "── 危险通道 (任何模式都必须 sinkhole) ──"
test_domain "telemetry-prd.vn.cloud.tesla.cn"           "sinkhole" "vehicle state 上报"
test_domain "apigateway-x2-trigger.tesla.cn"            "sinkhole" "trigger 推送通道"
test_domain "api-prd.vn.cloud.tesla.cn"                 "sinkhole" "REST API"
test_domain "hermes-x2-api.prd.vn.cloud.tesla.cn"       "sinkhole" "x2 命令通道"
test_domain "hermes-stream-prd.vn.cloud.tesla.cn"       "sinkhole" "流命令通道"
test_domain "ownership.tesla.cn"                        "sinkhole" "权益管理"
test_domain "str04-prd.tesla.services.vn.cloud.tesla.cn" "sinkhole" "state 存储"

echo ""
if [ "$MODE" = "aggressive" ]; then
  echo "── 激进版例外 (mode=aggressive 应该放行) ──"
  test_domain "hermes-prd.vn.cloud.tesla.cn"            "real"     "手机 App 控车"
  test_domain "signaling.vn.cloud.tesla.cn"             "real"     "Hermes 信令"
  test_domain "media-server-me.tesla.cn"                "real"     "媒体服务"
  test_domain "www.tesla.cn"                            "real"     "官网"
else
  echo "── 激进版例外 (mode=$MODE, 应该 sinkhole) ──"
  test_domain "hermes-prd.vn.cloud.tesla.cn"            "sinkhole" "手机 App 控车"
  test_domain "signaling.vn.cloud.tesla.cn"             "sinkhole" "Hermes 信令"
  test_domain "media-server-me.tesla.cn"                "sinkhole" "媒体服务"
  test_domain "www.tesla.cn"                            "sinkhole" "官网"
fi

echo ""
echo "── 非 Tesla 域名 (DNS 全开, 应该正常解析) ──"
test_domain "baidu.com"   "real"  "百度 (上游 DNS 转发)"
test_domain "qq.com"      "real"  "腾讯"
test_domain "163.com"     "real"  "网易"

echo ""
echo "── IPv6 sinkhole (AAAA 记录,防止车机走 v6 绕过) ──"
test_domain_v6 "tesla.cn"                              "sinkhole" "根域 AAAA"
test_domain_v6 "tesla.com"                             "sinkhole" "根域 AAAA"
test_domain_v6 "teslamotors.com"                       "sinkhole" "根域 AAAA"
test_domain_v6 "tesla.services"                        "sinkhole" "根域 AAAA"
test_domain_v6 "api-prd.vn.cloud.tesla.cn"             "sinkhole" "REST API AAAA"
test_domain_v6 "hermes-x2-api.prd.vn.cloud.tesla.cn"   "sinkhole" "x2 命令通道 AAAA"
test_domain_v6 "telemetry-prd.vn.cloud.tesla.cn"       "sinkhole" "vehicle state 上报 AAAA"

echo ""
echo "=========================================="
if [ "$FAIL" = "0" ]; then
  echo "  ✅ 全部通过: $PASS 项"
  echo "=========================================="
  exit 0
else
  echo "  ❌ 失败: $FAIL 项 (通过 $PASS 项)"
  echo "=========================================="
  exit 1
fi
