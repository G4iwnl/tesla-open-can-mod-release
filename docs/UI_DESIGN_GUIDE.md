# Tesla CAN Mod - UI 设计规范与风格指南

## 一、设计定位

为 Tesla 车载浏览器（Chromium 内核，viewport 约 1280×1080）设计的嵌入式 Web App。通过 ESP32 WiFi AP 本地访问，零外部依赖，完全自包含。风格对齐 Tesla 车机原生界面：纯黑深色主题、极简扁平、大触控目标、高对比度。

## 二、技术约束

| 项目 | 说明 |
|------|------|
| 浏览器 | Tesla MCU Chromium（79~136+），桌面模式渲染 |
| 屏幕 | Model 3/Y: 15.4" 1920×1200，浏览器窗口约 1280×1080 |
| 性能 | MCU2 Intel Atom / MCU3 AMD Ryzen，性能远低于桌面 |
| 交互 | 纯触控，无键盘快捷键，无鼠标 |
| 部署 | HTML/CSS/JS 全部内联在 C++ 头文件 `include/web/web_ui.h` 中，以 `R"rawliteral(...)"` 包裹 |
| 依赖 | 零外部 CDN、零框架，原生 JS + 内联 CSS |
| 双语 | 中文/英文，localStorage 持久化切换 |

## 三、色彩体系

```css
:root {
  --bg: #000;                          /* 纯黑背景 */
  --card: rgba(28, 28, 30, .92);       /* 卡片背景，微透明 */
  --card2: rgba(38, 38, 40, .85);      /* 二级卡片 */
  --border: rgba(255, 255, 255, .08);  /* 卡片边框 */
  --border2: rgba(255, 255, 255, .12); /* 输入框/交互元素边框 */
  --text: #fff;                        /* 主文字 */
  --text2: #8e8e93;                    /* 次要文字 */
  --text3: #6e6e73;                    /* 弱化文字 */
  --blue: #3478f6;                     /* 强调蓝：选中态、链接、进度 */
  --green: #30d158;                    /* 成功/在线/FSD 开启 */
  --red: #ff453a;                      /* 错误/危险/删除 */
  --amber: #ffd60a;                    /* 警告/提示 */
  --tesla: #e31937;                    /* Tesla 品牌红：Logo、危险按钮 */
}
```

### 使用规则
- 背景永远纯黑 `#000`，减少驾驶眩光
- 卡片使用半透明深灰，不要纯色块
- 选中/激活状态统一用 `--blue`
- 成功/开启用 `--green`，错误/关闭用 `--red`
- 次要信息用 `--text2`，最弱信息用 `--text3`

## 四、字体与排版

```css
font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display", "PingFang SC", "Helvetica Neue", sans-serif;
```

### 字号层级（车机优化，整体偏大）

| 元素 | 字号 | 字重 | 说明 |
|------|------|------|------|
| 模式卡片文字 | 22px | 800 | 大按钮上的两字标签（保守、激进等） |
| 统计数字 | 28px | 800 | 仪表盘数值 |
| 增强仪表数值 | 24px | 800 | 参数格子中的数值 |
| Modal 标题 | 19px | 700 | 弹窗标题 |
| 卡片标题 | 16px | 700 | 区域标题，大写+字间距 |
| 行名称 | 16px | 600 | 设置项名称 |
| 正文/描述 | 16px | 400 | 基础正文 |
| Segment 按钮 | 15px | 600 | 分段选择器 |
| Toast/提示 | 15px | 600 | 通知消息 |
| 行描述 | 13px | 400 | 设置项副标题 |
| 标签/徽章 | 11px | 700 | 大写标签、统计标签 |
| DEV 标签 | 9px | 700 | 开发中标记 |

### 排版规则
- `letter-spacing: 2px` 用于模式卡片文字，增加辨识度
- `letter-spacing: 1.5px` 用于卡片标题
- `font-variant-numeric: tabular-nums` 用于所有数字，等宽对齐
- `text-transform: uppercase` 用于卡片标题和标签

## 五、间距与圆角

### 间距（8px 网格）
- 页面内边距：16px（移动）/ 20-24px（桌面）
- 卡片间距：14-16px（grid gap）
- 卡片内边距：18px 20px
- 行间距：12px padding-top/bottom
- 元素间 gap：8-14px

### 圆角
- 卡片：14px (`--radius`)
- 输入框/子元素：10px (`--radius-sm`)
- 模式卡片：14px
- Segment 控件外框：10px，内按钮 8px
- Modal：18px
- Toast：12px
- 按钮：11px
- FSD 药丸：20px（圆润）
- 标签/徽章：6px

## 六、组件规范

### 1. 底部 Tab 导航
- 固定在底部，3 个 Tab：仪表盘 / 设置 / 系统
- 背景：`rgba(18,18,20,.96)` + `backdrop-filter: blur(20px)`
- 激活态颜色：`--blue`
- 图标 22px SVG + 10px 文字标签
- padding：`6px 0 env(safe-area-inset-bottom, 8px)`

### 2. 模式卡片（Driving Mode / Speed Limit）
- 网格布局：驾驶模式 5 列，限速模式 3 列
- 每个卡片只放**两个大字**，22px 800 weight
- 背景：`rgba(255,255,255,.04)`
- 边框：`2px solid rgba(255,255,255,.06)`
- 激活态：`border-color: var(--blue); background: rgba(52,120,246,.1)`
- 触控反馈：`:active { transform: scale(.96) }`
- 禁用态：`opacity: .35; pointer-events: none`

### 3. Segment 分段控件（替代所有 `<select>` 下拉框）
- 外框：`rgba(255,255,255,.06)` 背景 + 3px padding
- 按钮：透明底 → 激活态 `rgba(255,255,255,.14)` 白底
- 文字：次要色 → 激活态白色
- 用于：档位源（自动/手动）、硬件版本（LEGACY/HW3/HW4）

### 4. Toggle 开关
- 46×26px，iOS 风格
- 关闭：`rgba(255,255,255,.12)` 轨道
- 开启：`var(--green)` 轨道
- 圆点：22px 白色圆形 + 阴影

### 5. 自定义 Modal（替代所有 `alert()` / `confirm()`）
- 全屏遮罩：`rgba(0,0,0,.6)` + `backdrop-filter: blur(8px)`
- 弹窗盒子：`rgba(44,44,46,.98)` + 18px 圆角
- 入场动画：0.2s scale(0.92→1) + opacity
- 支持纯文本 body 和 HTML body（用于表单输入）
- 按钮：取消（灰底）+ 确认（蓝底）/ 危险（红底）
- Promise-based API：`await showModal(title, body, hasCancel, isHtml)`

### 6. Toast 通知
- 固定顶部，slide-down 动画
- `backdrop-filter: blur(16px)` 毛玻璃
- 3 秒自动消失
- 成功态：绿色边框 / 错误态：红色边框

### 7. 规则列表（限速映射）
- 每条规则一行：`[30-50 km/h] → [60 km/h] [×]`
- 点击数字区域 → 编辑（弹 Modal 表单）
- 点击红色 × → 删除（带确认）
- 底部虚线框 "+ 添加规则" 按钮
- 数据存 localStorage，键名 `slRules`

### 8. FSD 状态药丸
- 内联 pill 样式，不占大面积
- 关闭态：灰色边框
- 开启态：绿色边框 + 绿色背景微透
- 显示：FSD ON/OFF · HW版本

## 七、页面架构

```
┌─ Header ──────────────────────────────────────────┐
│ [T] Tesla CAN Mod v2.x · IP · WxH   [NET] [●] [EN]│
├─ Main (scrollable) ──────────────────────────────────┤
│                                                      │
│  Tab 1: 仪表盘                                       │
│  ┌ Stats Bar: [FSD ON·HW4]  Mod:xx Rcv:xx Err:x ─┐ │
│  ├ Enhanced Dashboard: 6格一行 (限速/置信度/...) ──┤ │
│  ├ Driving Mode: 5个模式卡片 ─────────────────────┤ │
│  └ Speed Limit Assistant: 3模式+规则表 ───────────┘ │
│                                                      │
│  Tab 2: 设置                                         │
│  ┌ Hardware: HW版本 segment ──────────────────────┐ │
│  ├ Features: 6个功能 toggle ──────────────────────┤ │
│  └ Module Enhancements: 预热 toggle ──────────────┘ │
│                                                      │
│  Tab 3: 系统                                         │
│  ┌ WiFi: SSID + 密码 ────────────────────────────┐ │
│  ├ OTA: 文件上传 + 进度条 ────────────────────────┤ │
│  └ Live Log: 实时日志面板 ────────────────────────┘ │
│                                                      │
├─ Tab Bar (fixed) ────────────────────────────────────┤
│     [仪表盘]        [设置]         [系统]             │
└──────────────────────────────────────────────────────┘
```

### 设计原则
1. **一级页面放高频操作**：驾驶模式、限速助手、增强仪表
2. **二级页面放低频配置**：硬件版本、功能开关、WiFi
3. **三级页面放维护功能**：OTA、日志
4. **信息密度适度**：车机场景下信息要大、要少、要快速扫一眼能看懂
5. **触控友好**：最小触控目标 44px，模式卡片更大

## 八、交互规范

| 场景 | 方案 | 禁止 |
|------|------|------|
| 多选一 | Segment 分段控件 / 模式卡片 | `<select>` 下拉框 |
| 开关 | iOS 风格 Toggle | Checkbox |
| 确认操作 | 自定义 Modal 弹窗 | `alert()` / `confirm()` |
| 操作反馈 | Toast 通知（顶部滑入） | 内联状态文字 |
| 危险操作 | Modal 确认 + 红色按钮 | 直接执行 |
| 页面切换 | 底部 Tab + fade-up 动画 | 传统链接跳转 |

## 九、i18n 双语系统

- 语言切换存 `localStorage('lang')`，默认中文
- 所有可见文字通过 `T[lang].key` 获取
- HTML 元素通过 `id` 绑定，`applyLang()` 统一刷新
- 模式卡片只显示中文两字标签（保守/默认/适中/激进/蠕行）
- 英文模式下显示对应英文短词

## 十、文件结构

```
include/web/
  web_ui.h      ← 完整 HTML/CSS/JS，C++ raw string literal
  web_server.h  ← ESP32 HTTP 服务器 + API 端点

docs/
  preview_server.py  ← 本地预览服务器（Mock API）
  UI_DESIGN_GUIDE.md ← 本文件
```

### 本地预览
```bash
python3 docs/preview_server.py
# 访问 http://127.0.0.1:9090
```

## 十一、API 端点

| 方法 | 路径 | 用途 |
|------|------|------|
| GET | `/` | 返回 HTML 页面 |
| GET | `/api/status?log_since=N` | 完整系统状态 JSON |
| POST | `/api/bypass-tlssc` | 切换 TLSSC 绕过 |
| POST | `/api/isa-speed-chime-suppress` | 切换限速音抑制 |
| POST | `/api/emergency-vehicle-detection` | 切换紧急车辆检测 |
| POST | `/api/enhanced-autopilot` | 切换增强 AP |
| POST | `/api/nag-killer` | 切换 Nag Killer |
| POST | `/api/china-mode` | 切换中国模式 |
| POST | `/api/enable-print` | 切换日志 |
| POST | `/api/profile-mode-auto` | 切换速度档源 |
| POST | `/api/preheat` | 切换预热 |
| POST | `/api/hw-mode` | 切换硬件版本 |
| POST | `/api/speed-profile` | 设置速度档位 |
| POST | `/api/wifi` | WiFi 配置 |
| POST | `/api/ota` | 固件上传 |

所有 POST 请求体为 JSON，响应为 `{"ok": true}` 或错误 JSON。

---

*Tesla CAN Mod UI Design Guide v2 | 2026-04*
