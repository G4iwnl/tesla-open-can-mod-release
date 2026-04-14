#!/bin/sh
# Tesla ISO - uhttpd 启动器 v1.0.2
# 监听 8888, 服务 /www-tesla, CGI 在 /www-tesla/cgi-bin/
#
# OpenWrt 标准 web server: uhttpd
# (BusyBox httpd applet 在某些路由器固件中被裁掉, 不能依赖)
#
# 用法:
#   tesla-iso-httpd-init.sh start    启动
#   tesla-iso-httpd-init.sh stop     停止
#   tesla-iso-httpd-init.sh restart  重启
#   tesla-iso-httpd-init.sh status   状态

PIDFILE=/tmp/tesla-iso-httpd.pid
WWWROOT=/www-tesla
PORT=8888
SERVER_NAME=tesla-iso
# 进程匹配模式 (用 pgrep -f 找我们自己的实例, 区别于厂商 80 端口的实例)
MATCH_PATTERN="uhttpd.*-h $WWWROOT"

start() {
  if is_running; then
    echo "tesla-iso-httpd already running (pid=$(cat $PIDFILE 2>/dev/null))"
    return 0
  fi
  if [ ! -d "$WWWROOT" ]; then
    echo "ERROR: $WWWROOT does not exist"
    return 1
  fi
  if ! command -v uhttpd >/dev/null 2>&1; then
    echo "ERROR: uhttpd not found"
    return 1
  fi
  # uhttpd 默认就是 daemon 模式 (没 -f)
  #   -h DIR     document root
  #   -r NAME    server name
  #   -x PREFIX  cgi url prefix (URL 以这个开头的会按 CGI 跑)
  #   -p PORT    listen port
  #   -t SEC     network timeout
  #   -T SEC     cgi timeout
  uhttpd -h "$WWWROOT" -r "$SERVER_NAME" -x /cgi-bin -p "0.0.0.0:$PORT" -t 30 -T 30 -A 1
  sleep 1
  _pid=$(pgrep -f "$MATCH_PATTERN" 2>/dev/null | head -1)
  if [ -n "$_pid" ]; then
    echo "$_pid" > $PIDFILE
    echo "tesla-iso-httpd started (pid=$_pid, port=$PORT)"
    return 0
  else
    echo "ERROR: uhttpd failed to start"
    return 1
  fi
}

stop() {
  if [ -f "$PIDFILE" ]; then
    _pid=$(cat $PIDFILE 2>/dev/null)
    if [ -n "$_pid" ] && kill -0 "$_pid" 2>/dev/null; then
      kill "$_pid" 2>/dev/null
      sleep 1
      kill -9 "$_pid" 2>/dev/null
    fi
    rm -f $PIDFILE
  fi
  # 兜底: 杀所有匹配的我们自己的 uhttpd 实例 (注意不能误杀厂商的 /www 实例)
  pkill -f "$MATCH_PATTERN" 2>/dev/null
  echo "tesla-iso-httpd stopped"
}

restart() {
  stop
  sleep 1
  start
}

is_running() {
  [ -f "$PIDFILE" ] || return 1
  _pid=$(cat $PIDFILE 2>/dev/null)
  [ -n "$_pid" ] || return 1
  kill -0 "$_pid" 2>/dev/null
}

status() {
  if is_running; then
    echo "running (pid=$(cat $PIDFILE), port=$PORT)"
    return 0
  else
    # 退路: 看看是否有匹配的 uhttpd 进程但 pidfile 没记
    _pid=$(pgrep -f "$MATCH_PATTERN" 2>/dev/null | head -1)
    if [ -n "$_pid" ]; then
      echo "$_pid" > $PIDFILE
      echo "running (pid=$_pid, port=$PORT) [pidfile recovered]"
      return 0
    fi
    echo "stopped"
    return 1
  fi
}

case "$1" in
  start)   start ;;
  stop)    stop ;;
  restart) restart ;;
  status)  status ;;
  *)
    echo "usage: $0 {start|stop|restart|status}"
    exit 1
    ;;
esac
