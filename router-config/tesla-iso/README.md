# Tesla ISO v1.0 — Tesla 隔离 WiFi 方案

> **国内首个公开的 Tesla 隔离 WiFi 工程化方案**。
> 用一台 OpenWrt/BusyBox 路由器隔离 Tesla 车机的网络，**防止 Tesla 远程封禁通过 CAN 总线启用的 FSD 功能**，同时让客人/其他设备完全正常上网。
> 自带 web 控制台，支持升级包一键升级。

---

## ⚠️ 适用场景与免责声明

- **适用**：HW3/HW4 + 已通过 CAN 总线方式启用 FSD 的 Tesla Model 3/Y/S/X (中国大陆)
- **不适用**：未做过 CAN 修改的原厂车，或者依赖 Tesla 官方手机 App 控车的车主
- **风险**：本方案屏蔽 Tesla 官方所有云服务，可能违反 Tesla 用户协议；车主自担风险
- **不保证**：Tesla 仍可能通过未知通道下发封禁命令，本方案不承诺 100% 防御

---

## 这个方案做什么 / 不做什么

### ✅ 做的事
- 屏蔽所有 `*.tesla.cn / *.tesla.com / *.teslamotors.com / *.tesla.services` 的 DNS 解析
- 例外放行两个**必需**的 Tesla 子域：
  - `connman.vn.cloud.tesla.cn` —— Tesla WiFi 验证 (不放行连不上 WiFi)
  - `nav-prd-maps.tesla.cn` —— Tesla 中国导航后端 (不放行没有路径规划)
- 客人/其他设备完全正常上网 (DNS 全开，只 sinkhole tesla 一家)
- Web 控制台可视化管理例外、查看待审域名、升级
- 升级包一键升级 + 自动备份 + 失败回滚

### ❌ 不做的事
- 不能让 Tesla 手机 App 控车 —— 手机 App 命令通道走 hermes，必须屏蔽
- 不能保留 Tesla 语音助手"小 T" —— 走 hermes-x2-api
- 不能保留 Tesla 远程实时路况 —— 走 telemetry/hermes
- 不能让车机收到 OTA 更新通知 —— **这正是我们要的**

---

## 工作原理

### Tesla 是怎么禁掉 FSD 的？

基于反向工程社区的拼图 (Tristan Rice / timdorr / Black Hat 2023 等)，Tesla 通过 CAN 破解启用 FSD 后，禁用流程大致是：

```
[1] 车机 vehicle state 变 (CAN 改写 register, fsd_enabled=true)
    ↓
[2] 车机周期性 telemetry 上报到 telemetry-prd.vn.cloud.tesla.cn
    "我现在 fsd_enabled=true"
    ↓
[3] Tesla 后端比对授权数据库, 发现不一致
    ↓
[4] 后台运营 (tbx-mothership principal) 调用 Odin RPC:
    PROC_ICE_X_SET-VEHICLE-CONFIG
    ↓
[5] 命令通过 hermes 长连接 (mTLS WebSocket) 下发到车机
    域名: hermes-prd / hermes-x2-api / apigateway-x2-trigger
    ↓
[6] 车机 hermes_proxy 转发到本地 Odin/DebugService
    ↓
[7] DebugService(4035) 改写 /var/lib/.../vehicle_config 文件
    ↓
[8] CarServer(7654) 重启 → 重新加载 vehicle config → CAN 寄存器复位
    ↓
车机屏幕显示"你的辅助驾驶已恢复初始状态"
```

### 我们的方案怎么破

**两层防御**：

1. **拦上行 [2]**：DNS sinkhole 让车机解析 `telemetry-prd.vn.cloud.tesla.cn` 拿到 127.0.0.1，上报失败 → Tesla 永远不知道你 CAN 改了 FSD
2. **拦下行 [5]**：DNS sinkhole 让车机解析所有 hermes 相关域名拿到 127.0.0.1，命令送不到 → 即使检测到也下不来

**关键洞察**：所有这些通道都在 `*.tesla.cn` / `*.tesla.com` / `*.tesla.services` 之下。**4 条根域 sinkhole 一刀切就全砍了**，不需要维护具体子域清单，无论 Tesla 后端怎么改架构、加新子域。

**为什么这样能 work**：CAN 修改是在 ICE register 上的，开车时 register 是 enabled。Tesla 推送 vehicle config 文件改写是异步的，**车机重启时才重新加载**。只要你装好隔离 WiFi 后 Tesla 再也送不到任何 vehicle config 修改命令，重启时加载的就还是原始 vehicle config，CAN 修改的 register 依然有效。

### 工信部新规背书

工信部 2026-02-28 发布《关于进一步加强智能网联汽车产品准入、召回及软件在线升级管理的通知》：**涉及自动驾驶/动力控制等核心功能的软件变更须提前 15 个工作日备案公示，未经备案不得推送**。

本方案的功能正是阻止特斯拉在未经工信部备案的情况下远程修改用户已购买的 FSD 功能状态，符合该文件精神。

---

## 装机前准备

### 硬件要求
- 一台 OpenWrt / LEDE / BusyBox 路由器（我用的是 Hongdian TR341，理论上任何带 dnsmasq + iptables + busybox httpd 的路由器都能用）
- 路由器有 telnet 或 SSH 访问

### 必需命令
路由器上应有: `dnsmasq` `iptables` `ip6tables` `httpd` (busybox) `tar` `sed` `awk` `grep`

### 网络拓扑
```
            ┌──────────┐
            │  Tesla   │ ── 只连本路由器 WiFi, 4G 数据已禁用
            │  车机    │
            └────┬─────┘
                 │
            ┌────▼─────┐  ←── 装这套方案
            │  路由器  │
            │ (TR341)  │
            └────┬─────┘
                 │
                 ▼
              互联网
```

---

## 安装步骤

### 1. 把整个 tesla-iso/ 目录传到路由器 /tmp/

参考 router-config/README.md 的方案 A/B/C/D 选一种（HFS / Python http.server / WinSCP / U 盘）。

或者直接拿打包好的 `tesla-iso-1.0.0.tar.gz` 解压：

```sh
# 在路由器上
cd /tmp
# 上传 tesla-iso-1.0.0.tar.gz 到这里
tar -xzf tesla-iso-1.0.0.tar.gz
cd tesla-iso-1.0.0
```

### 2. 跑安装脚本

```sh
sh install.sh
```

脚本会自动：
- ✅ 检查依赖命令
- ✅ 检测路由器 LAN IP
- ✅ 检测旧版 tesla-wifi-isolation 方案 (如果有), 自动清理
- ✅ 备份当前所有受管文件到 `/etc/tesla_iso_backup/preinstall_<时间戳>/`
- ✅ 部署新文件
- ✅ 重启 dnsmasq / 应用 firewall.user / 启动 web httpd
- ✅ 跑 13 项自检 (DNS 解析正确性 + 进程状态 + 防火墙规则)

最后会显示 web 控制台地址，例如 `http://192.168.2.254:8888`。

### 3. ⚠️ 关键步骤 — 设置 Tesla 车机

1. **Tesla 车机连接本路由器 WiFi**（不要连任何其他 WiFi）
2. **禁用车机 4G 数据**（设置 → 网络 → 数据，关闭）
3. **重启车机一次**（按住方向盘左右两个滚轮 10 秒）
   - 这一步**必须做**，是为了清除车机内的旧 DNS 缓存和已建立的 hermes 长连接
   - 不重启的话，缓存窗口期最长 5 分钟内仍可能通信

### 4. 打开 Web 控制台

浏览器访问 `http://<路由器IP>:8888`：

- **系统状态**：dnsmasq / crond / 设备数 / 例外数
- **例外列表**：默认两条系统例外（不可删），用户可加更多
- **待审**：从 dns.log 实时扫描，列出车机查过但被屏蔽的 tesla 子域
- **DNS 日志**：最近 30 行
- **升级 / 回滚**：上传新版 tar.gz 一键升级

---

## 升级流程

### 你拿到 v1.0.x 升级包后

1. 浏览器打开 web 控制台 `http://<路由器IP>:8888`
2. 翻到底部 "升级 / 回滚" 区块
3. 点 "选择文件" 选 `tesla-iso-1.0.x.tar.gz`
4. 点 "上传并升级"
5. 等几秒 → web 自动重启 → **手动刷新页面**
6. 看到顶部版本号变成 1.0.x = 升级成功

### 升级失败怎么办

升级脚本检测到 dnsmasq/httpd 没启动会自动回滚。如果手动想回滚：

- web 上 "历史版本" 区块列出所有备份
- 点对应版本的 [回滚] 按钮

---

## 常见问题

### Q1: 装完之后 Tesla 手机 App 不能控车了？
是的，**这是设计如此**。手机 App 控车的命令通道走 hermes，跟 Tesla 禁 FSD 的命令通道是同一条 mTLS WebSocket，网络层无法分离。要保命就放弃手机 App。

### Q2: 客人想用我的 WiFi 上网，能正常用吗？
能。客人手机/电脑连本路由器后，所有非 tesla 域名都能正常解析（百度/腾讯/google/whatever）。**只有 tesla.cn 等 4 个根域**被屏蔽，影响为零。

### Q3: 装完后车机 WiFi 一直转圈连不上？
检查：
- `/etc/dnsmasq.d/tesla_iso.conf` 里是否有 `server=/connman.vn.cloud.tesla.cn/114.114.114.114`
- 在路由器上 `nslookup connman.vn.cloud.tesla.cn 127.0.0.1` 是否返回真实 IP
- 路由器 WAN 是否能上网

### Q4: 装完后导航不工作？
检查 `/etc/dnsmasq.d/tesla_iso.conf` 里有没有 `server=/nav-prd-maps.tesla.cn/...`。如果丢了，web 上重新加一条 nav-prd-maps.tesla.cn 例外即可。

### Q5: 我能不能关掉某些 tesla 屏蔽？
能。Web 控制台 → 例外列表 → 输入框输入完整域名 → [添加例外]。但**强烈不建议**放行任何带有 `hermes / api-prd / telemetry / trigger / x2 / config / mothership` 关键词的域名 —— 那会让 Tesla 重新获得 revoke 你 FSD 的能力。

### Q6: 装完后还能 OTA 升级吗？
**不能，这正是我们要的**。OTA 通道也走 *.tesla.cn (api-prd 拉清单 / firmware-bucket 拉包)，全被 sinkhole 了。**绝不要试图放行 OTA**，否则一次升级可能就会修复 CAN 漏洞 + 重置 vehicle config。

### Q7: Web 控制台能不能加密码？
v1.0 不支持。Web 通过 iptables INPUT 规则限制只允许 `br-lan` 接口入站，物理上只有连接到本路由器 WiFi 的设备能访问。安全足够。v1.1 可能加 basic auth。

### Q8: 我能不能通过这个方案让 FSD 永久免费？
**不能**。本方案只是阻止 Tesla 远程禁用**已经通过其他方式启用**的 FSD。它不会自己开 FSD。你必须先用 CAN 总线方式启用，再装这个保。

### Q9: 多久要升级一次？
不需要定期升级。**只在 Tesla 发现新的封禁路径后**，我会发新版本包给你，里面包含新的屏蔽规则或修复。你拿到包再升级即可。

### Q10: 路由器重启后还有效吗？
有效。所有配置都在 /etc/ 持久化分区，开机时 firewall.user 会自动重新挂载所有规则、启动 web httpd。

---

## 卸载

```sh
cd /tmp/tesla-iso-1.0.0
sh uninstall.sh
```

会从最早的 preinstall 备份恢复，把路由器还原到装方案前的状态。

---

## 文件清单

| 路径 | 作用 |
|---|---|
| `/etc/dnsmasq.conf` | 主 DNS 配置, 4 条 sinkhole + 上游 DNS + 加载 dnsmasq.d/ |
| `/etc/dnsmasq.d/tesla_iso.conf` | 例外放行列表, web 自动维护 |
| `/etc/firewall.user` | 开机执行: IPv6 DROP + 8888 LAN-only + 启 crond + 启 httpd |
| `/etc/tesla_iso/version` | 当前版本号 |
| `/etc/tesla_iso_backup/` | 备份目录 (preinstall_xxx + 历次升级前) |
| `/usr/sbin/tesla-iso-httpd-init.sh` | busybox httpd 启停脚本 |
| `/www-tesla/index.html` | 单页 web 控制台 |
| `/www-tesla/cgi-bin/api.sh` | 单 CGI 入口, 处理所有 web 请求 |

---

## 已知限制 / 路线图

- **v1.0 (当前)**：基础隔离方案 + web 控制台 + 升级机制
- **v1.x 计划**：
  - 探索 owner-api 半开放，让手机 App 至少能查询车辆基本状态 (无控车)
  - 增加 web 上的"添加自定义 dnsmasq 配置片段"高级模式
  - 失败检测加强 (web 上能看到哪些 tesla 域名被车机最近查过的频率趋势)
- **v2.0 远期**：
  - 如果能拿到精准的 revoke endpoint 情报，转向"放行所有 Tesla 服务、只精准拦 revoke"的精准模式
  - 这需要被禁过的车的 dns.log 数据 + 那些已经实现"手机控车保 FSD"方案的具体配置

---

## 反馈和贡献

如果你的车机 dns.log 里出现了任何疑似"Tesla 新加的 endpoint"（特别是命名带 `config` / `entitlement` / `feature` / `mothership` 的），请贴出来。每一份真实数据都帮我们把方案打磨得更精准。

---

## 致谢

- Tristan Rice (fn.lc) — Tesla Model 3 安全分析
- timdorr / lotharbach — Tesla Hermes 协议反向工程
- Black Hat 2023 TU Berlin — Tesla AMD voltage glitching
- 国内 Tesla 车主社区的 CAN 总线破解先驱
