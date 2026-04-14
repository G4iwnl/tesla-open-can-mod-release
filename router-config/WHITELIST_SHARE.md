# Tesla 隔离 WiFi 白名单方案（中国版 HW4 + FSD v13）

> 用于保护买断 FSD 的 Tesla Model 3/Y 不被 Tesla 远程推送 OTA 或吊销证书，同时保留导航、音乐、微信等常用功能。

## 背景

中国大陆 Tesla 会通过 OTA 更新或远程推送指令"校正"破解的车辆（禁用 FSD、吊销证书）。方案目标：
- **彻底屏蔽** Tesla 后端的命令通道、遥测、API
- **保留** 导航、音乐、微信推送、WiFi 验证等合法使用的第三方服务
- **自适应** Tesla 查询新域名时自动加白（无需人工干预）

路由器：任意 OpenWrt/LEDE 路由器（我用的是 Hongdian TR341，BusyBox + dnsmasq 即可）

---

## 三层防御架构

### 第 1 层：DNS 白名单（dnsmasq）

**白名单模式**：`no-resolv` 让 dnsmasq **不**转发未知域名，只有明确定义的域名才能解析。

- 所有 `*.tesla.cn / *.tesla.com / *.teslamotors.com / *.tesla.services` **sinkhole 到 `127.0.0.1`**（`address=/tesla.cn/127.0.0.1`）
- 用最长后缀匹配**精确放行**个别 Tesla 子域（`server=/xxx.tesla.cn/114.114.114.114`）
- 非 Tesla 的白名单域名用 `server=/X/114.114.114.114` 转发到公共 DNS

### 第 2 层：iptables IP 白名单（DNSONLY 链）

`FORWARD` 链第 1 位挂 `DNSONLY` 自定义链，**只放行白名单 IP，末尾 DROP**。
- 基础规则：state RELATED/ESTABLISHED、DNS 53、DHCP 67-68、NTP 123、路由器自己
- 动态规则：从脚本解析得到的 IP 加进来
- 兜底：**DROP**

### 第 3 层：IPv6 完全封堵

`ip6tables FORWARD` 第 1 位直接 `DROP all`。理由：dnsmasq 会返回 IPv6 AAAA 记录，Tesla 可能通过 IPv6 绕过 iptables v4 规则。**一刀切彻底禁掉 IPv6 转发**。

---

## 白名单域名列表

### ✅ Tesla 精确放行（2 个）

| 域名 | 用途 | 策略 |
|---|---|---|
| `connman.vn.cloud.tesla.cn` | **WiFi 验证**（Tesla 连接 WiFi 的 captive portal 检测） | `/etc/hosts` 硬绑固定 IP `116.129.226.175` |
| `nav-prd-maps.tesla.cn` | **Tesla 中国导航**（路径规划、地图瓦片、路口放大图） | `server=/nav-prd-maps.tesla.cn/114.114.114.114`（最长匹配胜过 `address=/tesla.cn/`）|

**⚠️ 只有这两个 Tesla 子域可以放行。**

### ✅ 后缀白名单（16 条，覆盖所有子域）

| 后缀 | 覆盖的服务 |
|---|---|
| **百度系** |
| `baidu.com` | 百度地图（`api.map.baidu.com`、`lc.map.baidu.com`、`newclient.map.baidu.com` 等）、百度语音 |
| `baidu.cn` | 百度 |
| `bdimg.com` | 百度图片 CDN（地图瓦片 `mapapip0.bdimg.com`、`automap0.bdimg.com`） |
| `bdstatic.com` | 百度静态资源 |
| `bdycdn.cn` | 百度云 CDN（QQ 音乐 CDN 回源 `isure6-stream-qqmusic.a.bdycdn.cn`） |
| `bcebos.com` | **百度云对象存储 BOS**（路口放大图 `enlargeroad-view.su.bcebos.com`） |
| **腾讯系** |
| `qq.com` | QQ 主站、QQ 音乐（`y.qq.com`、`qplaycloud.y.qq.com`、`isure6.stream.qqmusic.qq.com`）、腾讯 AI（`gw.tai.qq.com`）、wecar（`wecarplat.map.qq.com`、`mqtthalley.wecar.map.qq.com`）、微信 AE DNS（`aedns.weixin.qq.com`） |
| `tencent.com` | 腾讯通用 |
| `tencentmusic.com` | 腾讯音乐 |
| `gtimg.cn` | 腾讯图片 CDN（`y.gtimg.cn`） |
| `tencent-cloud.net` | 腾讯云 |
| `weixin.com` | 微信 Long Poll（`longcloud.weixin.com`）— 微信消息推送必需 |
| **网易系** |
| `163.com` | 网易云音乐 API（`openapi.music.163.com`） |
| `126.net` | 网易云音乐图片（`p1.music.126.net`、`p2.music.126.net`） |
| **其他** |
| `apple.com` | 苹果（CarPlay 等） |
| `aliyun.com` | 阿里云 |
| `ntsc.ac.cn` | 中国国家授时中心（NTP 对时） |

### 规则总数
- **后缀白名单 16 条** × 每条覆盖 X 和 *.X
- **Tesla 精确 2 条**
- 所有 Tesla 其他子域 → **127.0.0.1 黑洞**

---

## ⛔ 必须屏蔽的 Tesla 子域

以下是 Tesla 的**核心命令和控制通道**，放行任何一个都可能导致 FSD 被远程吊销：

| 域名 | 用途 | 屏蔽原因 |
|---|---|---|
| `hermes-prd.vn.cloud.tesla.cn` | Hermes 实时命令通道（WebSocket） | **最危险**：Tesla 远程下发任意命令（锁车、OTA、禁用 FSD） |
| `hermes-stream-prd.vn.cloud.tesla.cn` | Hermes 流式 | 同上 |
| `hermes-x2-api.prd.vn.cloud.tesla.cn` | Hermes v2 API | 同上，新版车型的命令通道 |
| `api-prd.vn.cloud.tesla.cn` | Tesla REST API | **证书下发走这里**，Tesla 吊销证书要用它 |
| `telemetry-prd.vn.cloud.tesla.cn` | 遥测上报 | 上报车辆状态给 Tesla |
| `media-server-me.tesla.cn` | 媒体服务 | 语音音频流可能走这里 |
| `www.tesla.cn` | Tesla 官网 | 不必要 |

### 自动拦截（dnsmasq 最长前缀匹配）
`address=/tesla.cn/127.0.0.1` 一条规则覆盖所有 `*.tesla.cn`，即使 Tesla 未来上线新的子域（`ota-prd.tesla.cn` 之类）也会被自动屏蔽。同理覆盖 `tesla.com / teslamotors.com / tesla.services`。

---

## 动态白名单脚本（核心自动化）

每分钟 `cron` 跑一次 `/etc/tesla_whitelist.sh`：

1. 读 `/tmp/dns.log`，过滤 Tesla 设备（192.168.2.193）查询过的所有域名
2. **分类**：
   - Tesla 域名 → 必须**精确等于** TESLA_EXACT 列表才放行，否则屏蔽
   - 非 Tesla 域名 → 按**后缀**匹配 SUFFIX_WHITELIST
3. 对放行的域名做 `nslookup`，拿到 IP 列表
4. **合并**历史持久化 IP（避免 CDN 漂移时旧 IP 被丢）
5. **重建** iptables DNSONLY 链，原子化应用
6. 内容变化时**才**写回 `/etc/whitelist_ips.txt`（避免 flash 损耗）
7. PID 锁防止并发

**关键设计**：
- 脚本从 Tesla 实际查询的 dns.log 学习 → Tesla 用什么子域，脚本就解析什么子域 → **不需要人工维护完整子域列表**
- 重启后 `/etc/firewall.user` 从 `/etc/whitelist_ips.txt` 加载历史 IP → **无 bootstrap 问题**
- 连接 connman 固定 IP 硬编码进 iptables → **即使脚本挂了 WiFi 验证也能通过**

---

## ✅ 已验证工作的功能

| 功能 | 状态 |
|---|---|
| WiFi 连接和验证 | ✅ |
| 百度地图实时导航、路径规划 | ✅ |
| Tesla 中国地图（nav-prd-maps 后端）| ✅ |
| 路口放大图（3D 车道级引导）| ✅ |
| QQ 音乐、网易云音乐（在线点播 + 歌词 + 封面） | ✅ |
| 车载微信消息推送 | ✅ |
| NTP 对时 | ✅ |
| Tesla FSD 正常启用 | ✅ |

## ❌ 不工作的功能（设计如此，换 FSD 安全）

| 功能 | 原因 |
|---|---|
| **"小 T" 语音命令** | 走 `hermes-x2-api`（危险命令通道），不能放行 |
| **Tesla 手机 App 远程控制** | 走 Hermes |
| **Tesla 账户/车辆状态同步** | 走 api-prd |
| **Tesla 实时路况** | 走 telemetry |
| **Tesla OTA 更新检查** | 走 api-prd（这正是我们要的！）|

**替代方案**：
- 导航用手机百度/高德发送到车机 / 车内手动输入地址
- 远程控制用不着（自己开车就手动操作）
- 语音命令用**手机 Siri / 小爱**
- OTA → 我们就是要禁止它

---

## 工程注意事项（避坑指南）

### 1. OpenWrt 的 cron 可能被厂商改造
我用的 TR341 路由器里 `/etc/init.d/cron` 被重写了，**只服务"定时重启"功能**。正常的 `crond` 不会被 init 脚本启动。

**解决**：在 `/etc/firewall.user` 里加一行 `pgrep -x crond >/dev/null || crond -c /etc/crontabs -b`，每次防火墙重载（含开机）时启动 crond。

### 2. dnsmasq 的最长后缀匹配
`address=/tesla.cn/127.0.0.1` 和 `server=/nav-prd-maps.tesla.cn/114.114.114.114` 同时存在时，后者**更长**所以优先。可以用这个机制精确放行个别 Tesla 子域。

### 3. connman 必须用固定 IP（避免 CDN 漂移）
CloudFront/阿里云 CDN 会返回不同 IP，DNS TTL 到了之后 Tesla 会重新解析。如果新 IP 不在 iptables 白名单里 → WiFi 验证失败 → 车辆判定"无网络" → 断开 WiFi。

**解决**：`/etc/hosts` 硬绑 `116.129.226.175 connman.vn.cloud.tesla.cn`，`/etc/hosts` 在 dnsmasq 里优先级最高。

### 4. IPv6 必须封死
dnsmasq 会同时返回 A（v4）和 AAAA（v6）记录。即便没有 IPv6 上游，Tesla 也会尝试 IPv6 连接，浪费时间。更重要的是：**iptables v4 不管 IPv6 流量**。必须 `ip6tables -I FORWARD 1 -j DROP`。

### 5. iptables 规则积累问题
每次脚本跑都要先**删除整个 DNSONLY 链再重建**（`iptables -D FORWARD -j DNSONLY; iptables -F DNSONLY; iptables -X DNSONLY; iptables -N DNSONLY`），**不能**只 flush。否则多次运行会出现**规则重复累加**。

### 6. busybox `sort -u -o` 有 bug
`sort -u file -o file` 在某些 busybox 版本会静默失败，导致输出文件为空。用 `sort file | uniq > output` 规避。

### 7. 并发锁
cron 每分钟跑一次，但脚本可能跑超过 1 分钟（尤其是 nslookup 慢的时候）。**必须加 PID 锁**防止自己并发：
```
LOCKFILE=/tmp/tesla_whitelist.lock
if [ -f "$LOCKFILE" ] && [ -d "/proc/$(cat $LOCKFILE)" ]; then exit 0; fi
echo $$ > "$LOCKFILE"
trap 'rm -f "$LOCKFILE"' EXIT INT TERM
```

### 8. 持久化防 flash 损耗
`/etc/whitelist_ips.txt` 写 flash，每分钟跑一次会烧坏 flash。**只在内容变化时才写**：
```
cmp -s "$IPFILE" "$PERSIST_IPFILE" || cp "$IPFILE" "$PERSIST_IPFILE"
```

---

## 配置文件

部署到路由器的 4 个文件（已打包在项目 `router-config/` 目录）：

| 路由器路径 | 作用 |
|---|---|
| `/etc/dnsmasq.conf` | DNS 白名单 |
| `/etc/firewall.user` | 开机重建 iptables DNSONLY + IPv6 DROP + 启动 crond |
| `/etc/tesla_whitelist.sh` | 动态白名单脚本 |
| `/etc/crontabs/admin` | `* * * * * /etc/tesla_whitelist.sh` |
| `/etc/hosts` | 加一行 `116.129.226.175 connman.vn.cloud.tesla.cn` |
| `/etc/whitelist_ips.txt` | 持久化的白名单 IP（脚本自动维护）|

---

## 免责声明

- 本方案不保证能 100% 防止 Tesla 破坏 FSD，Tesla 可能通过未知渠道绕过
- 本方案屏蔽 Tesla 官方服务，可能违反 Tesla 用户协议
- 使用后 Tesla 官方 App 无法远程控制车辆、无法同步车辆状态
- 自担风险
