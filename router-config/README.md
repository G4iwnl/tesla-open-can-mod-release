# Tesla WiFi 隔离 - 路由器配置包

一键部署 Tesla 专用 WiFi 白名单，保护买断 FSD 不被 Tesla 远程吊销。

**⚠️ 警告**：本方案会阻断 Tesla 官方云服务（OTA、App 远程控制、语音命令、遥测）。使用前请仔细阅读 `WHITELIST_SHARE.md` 了解完整原理。

---

## 适用条件

- **路由器**：OpenWrt / LEDE / 其他 BusyBox + iptables 的路由器（我测试用的是 Hongdian TR341）
- **必需命令**：`iptables`、`ip6tables`、`dnsmasq`、`crond`、`nslookup`、`sed`、`grep`、`awk`、`sort`、`uniq`
- **路由器管理方式**：SSH 或 Telnet（文件传输可用 `wget`、`scp`、U 盘、Web 文件管理器）
- **网络拓扑**：给 Tesla 专门建一个隔离 WiFi，Tesla 只连这个，不连任何其他 WiFi/APN

---

## 快速部署（3 步）

### 步骤 1：把整个目录传到路由器的 `/tmp/`

先解压 `tesla-wifi-isolation.tar.gz`：
- **Windows 10/11**：PowerShell 里跑 `tar -xzf tesla-wifi-isolation.tar.gz`（Win10 1803+ 自带 tar）；或者用 7-Zip 右键解压
- **Mac/Linux**：`tar -xzf tesla-wifi-isolation.tar.gz`

得到 `tesla-wifi-isolation/` 目录。接下来选一种方式传到路由器：

---

#### 💻 方案 A（**Windows 用户强烈推荐**）：HFS HTTP 文件服务器

**HFS（HTTP File Server）是一个免安装的单文件 EXE（约 600KB），拖文件即可共享**。

**步骤：**

1. **下载 HFS**
   - 官网：`https://www.rejetto.com/hfs/?f=dl`
   - 选 **HFS 2.x 经典版**（`hfs.exe`，绿色版不用安装）
   - 或直接搜索 "HFS rejetto download"

2. **运行 HFS**
   - 双击 `hfs.exe`
   - 如果 Windows Defender 弹窗，选"允许"或"运行"
   - HFS 启动后窗口顶部会显示一个 URL，例如 `http://192.168.1.100/`
   - **记下这个 IP**（即你电脑的 LAN IP）

3. **共享文件**
   - 把**整个 `tesla-wifi-isolation` 目录**拖进 HFS 左边"Virtual File System"区
   - 选 "Real folder" 或 "Virtual folder"，都行

4. **登录路由器**（用 PuTTY）
   - 下载 PuTTY：`https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html` → `putty.exe`（免安装）
   - 双击 `putty.exe`
   - Host Name 填路由器 LAN IP（例如 `192.168.2.254`）
   - **Connection type** 选 `Telnet`（如果路由器是 telnet）或 `SSH`
   - Port `23`（telnet）或 `22`（ssh）
   - 点 **Open**
   - 输入路由器的用户名密码登录

5. **在路由器上执行下载**
   ```
   cd /tmp
   mkdir tesla-wifi-isolation && cd tesla-wifi-isolation
   wget http://<你Windows电脑的IP>/tesla-wifi-isolation/dnsmasq.conf
   wget http://<你Windows电脑的IP>/tesla-wifi-isolation/firewall.user
   wget http://<你Windows电脑的IP>/tesla-wifi-isolation/tesla_whitelist.sh
   wget http://<你Windows电脑的IP>/tesla-wifi-isolation/install.sh
   wget http://<你Windows电脑的IP>/tesla-wifi-isolation/uninstall.sh
   ```
   把 `<你Windows电脑的IP>` 替换成 HFS 显示的那个 IP（比如 `192.168.1.100`）

**⚠️ Windows 防火墙可能会拦 HFS**：第一次启动时 Windows 会问"允许网络访问吗"，选"**专用网络**"允许。如果忘记点了，去"Windows 安全中心 → 防火墙 → 允许应用"放行 hfs.exe。

---

#### 💻 方案 B（Mac/Linux/WSL 用户）：Python HTTP 服务器

Mac/Linux 自带 Python3。在**本机**运行：
```
cd tesla-wifi-isolation/
python3 -m http.server 8899 --bind 0.0.0.0
```
然后 ssh/telnet 登录路由器后：
```
cd /tmp
mkdir tesla-wifi-isolation && cd tesla-wifi-isolation
wget http://<本机IP>:8899/dnsmasq.conf
wget http://<本机IP>:8899/firewall.user
wget http://<本机IP>:8899/tesla_whitelist.sh
wget http://<本机IP>:8899/install.sh
wget http://<本机IP>:8899/uninstall.sh
```

**Windows 用户如果装了 Python**，也可以用这个方案（PowerShell 里跑 `python -m http.server 8899`）。

---

#### 💻 方案 C：WinSCP（仅 SSH 路由器）

仅适用于路由器开了 **SSH** 的情况（telnet 不能用）：

1. 下载 WinSCP：`https://winscp.net/`
2. 新建站点：
   - 文件协议：SCP
   - 主机名：路由器 LAN IP
   - 端口 22
   - 用户名/密码：路由器账号
3. 连上后把 `tesla-wifi-isolation` 目录拖到路由器的 `/tmp/` 下

---

#### 💻 方案 D：U 盘（最简单，需要路由器有 USB 口）

1. 格式化 U 盘为 **FAT32** 或 ext4
2. 把 `tesla-wifi-isolation/` 目录拷到 U 盘
3. 插到路由器 USB 口
4. telnet/ssh 进路由器，挂载 U 盘：
   ```
   mkdir -p /mnt/usb
   mount /dev/sda1 /mnt/usb
   cp -r /mnt/usb/tesla-wifi-isolation /tmp/
   umount /mnt/usb
   ```

---

### 步骤 2：在路由器上运行安装脚本

```
cd /tmp/tesla-wifi-isolation
sh install.sh
```

脚本会自动：
- ✅ 备份现有配置到 `/etc/tesla-wifi-backup-<时间戳>/`
- ✅ 安装 `dnsmasq.conf`、`firewall.user`、`tesla_whitelist.sh` 到 `/etc/`
- ✅ 在 `/etc/hosts` 添加 connman 固定 IP
- ✅ 在 `/etc/crontabs/` 添加每分钟任务
- ✅ 重启 dnsmasq、启动 crond、应用 iptables 规则
- ✅ 运行 10+ 项功能测试（DNS 解析、iptables、crond、cron entry）

最后会打印测试结果。**所有测试必须显示 `[OK]`** 才算成功部署。

### 步骤 3：让 Tesla 连接路由器的 WiFi

- 在 Tesla 车机上搜索并连接这个路由器的 WiFi
- **不要连接任何其他 WiFi**（比如家里的、办公室的）
- **不要让 Tesla 使用 4G**（断掉 Tesla 的 4G SIM 或在设置里禁用数据）
- 脚本会自动识别 Tesla 的 MAC 地址，无需手动配置 IP

---

## 自动识别机制

脚本**不硬编码 Tesla 的 IP 或 MAC**，运行时按以下顺序自动识别：

1. **MAC OUI 前缀**：识别以下 Tesla 官方 MAC 前缀的设备
   ```
   4c:fc:aa  dc:44:27  18:cf:5e  68:ab:1e  ec:3a:4e
   98:ed:5c  98:ed:7e  0c:29:8f  54:26:96  e8:eb:1b
   cc:88:26  bc:d0:74  b0:41:1d  8c:f5:a3  74:e1:b6
   ```
2. **hostname 匹配**：`tesla`、`model-3`、`model3`、`modelS/X/Y` 等
3. **ARP 表 fallback**：`/proc/net/arp` 按同样的 MAC OUI 查找

如果你的 Tesla MAC 不在上述列表里，编辑 `/etc/tesla_whitelist.sh` 顶部的 `TESLA_MAC_OUIS` 变量添加。

路由器 LAN IP 也自动检测（`br-lan` 接口），无需硬编码。

---

## 验证部署成功

### 1. 查看 cron 自动分类日志
```
tail -f /tmp/whitelist.log
```
每分钟更新一次，显示当前 Tesla 查询的所有域名被分类成了"放行"或"阻断"。

### 2. 查看 DNS 查询流水
```
tail -f /tmp/dns.log
```
实时看 Tesla 在查什么域名。

### 3. 查看 iptables 规则
```
iptables -L DNSONLY -n -v --line-numbers
```
看每条规则的命中计数，判断哪些 IP 有实际流量。

### 4. 查看当前白名单 IP
```
cat /etc/whitelist_ips.txt
```

---

## 常见问题

### Q1：Tesla 连上 WiFi 就掉？
可能是 `connman.vn.cloud.tesla.cn` 的 WiFi 验证失败。检查：
- `/etc/hosts` 里有没有 `116.129.226.175 connman.vn.cloud.tesla.cn`
- `iptables -L DNSONLY -n | grep 116.129.226.175` 有没有规则
- `ping 116.129.226.175` 是否通（如果不通说明这个 IP 失效了，需要手动绑定新的 IP 到 `/etc/hosts`）

### Q2：某个 App 功能挂了（比如音乐搜索不到）
查看 `/tmp/whitelist.log` 的 `Blocked domains` 段，找出没被匹配的域名。如果是合法的第三方服务（非 tesla），编辑 `/etc/tesla_whitelist.sh` 的 `SUFFIX_WHITELIST` 变量，同时在 `/etc/dnsmasq.conf` 追加 `server=/X/114.114.114.114`，重启 dnsmasq 后手动运行一次 `/etc/tesla_whitelist.sh`。

### Q3：路由器重启后配置还在吗？
在。所有文件都在 `/etc/`（持久化分区），`/etc/firewall.user` 会在开机时自动重建 iptables 规则并启动 crond。`/etc/whitelist_ips.txt` 保存了历史 IP 列表，重启后立刻生效，无 bootstrap 问题。

### Q4：想加新的域名到白名单
编辑 `/etc/tesla_whitelist.sh` 第 58 行的 `SUFFIX_WHITELIST`，同时在 `/etc/dnsmasq.conf` 追加对应的 `server=/X/114.114.114.114`，然后：
```
/etc/init.d/dnsmasq restart
/etc/tesla_whitelist.sh
```

### Q5：Tesla 的 OTA 检查被阻止了吗？
是的。`api-prd.vn.cloud.tesla.cn` 和 `hermes-*` 都被 sinkhole 到 `127.0.0.1`，这是 OTA 检查和下发的主通道。**这正是我们要的**。

### Q6：可以开启 Tesla 语音命令吗？
**不可以**。语音命令依赖 `hermes-x2-api.prd.vn.cloud.tesla.cn`（Tesla 实时命令通道），这个域名**绝对不能放行**，否则 Tesla 可以通过它推送任何命令，包括禁用 FSD。用手机 Siri 或车机硬件按键替代。

---

## 卸载

```
cd /tmp/tesla-wifi-isolation
sh uninstall.sh
```

会：
- 移除 iptables DNSONLY 链和 IPv6 DROP 规则
- 移除 cron 任务
- 删除 `/etc/tesla_whitelist.sh` 和相关临时文件
- **从最新备份恢复** `dnsmasq.conf`、`firewall.user`、`hosts`、`crontabs`
- 重启 dnsmasq 和 firewall

卸载脚本会交互式确认。

---

## 文件清单

| 文件 | 作用 |
|---|---|
| `dnsmasq.conf` | DNS 白名单：Tesla 子域 sinkhole 到 127.0.0.1，精确放行 connman 和 nav-prd-maps |
| `firewall.user` | 开机 bootstrap：重建 iptables DNSONLY 链、IPv6 DROP、启动 crond |
| `tesla_whitelist.sh` | 动态白名单脚本：从 dns.log 学习 Tesla 查询，自动解析 IP 加白 |
| `install.sh` | 一键安装脚本 |
| `uninstall.sh` | 一键卸载脚本 |
| `WHITELIST_SHARE.md` | 完整方案说明（原理、策略、已验证功能、踩坑指南）|
| `README.md` | 本文档 |

---

## 免责声明

- 本方案**不保证** 100% 防止 Tesla 破坏 FSD，Tesla 可能通过未知渠道绕过
- 本方案屏蔽 Tesla 官方服务，可能**违反 Tesla 用户协议**
- 使用后 Tesla 官方 App 无法远程控制车辆、无法同步车辆状态
- **自担风险**

---

## 致谢 / 反馈

方案基于对 Tesla 中国服务架构的反向研究。欢迎分享 bug 和改进建议。
