// include/web_ui.h
// Tesla CAN Mod – Tesla-style single-page web console (v3)
// - 3-tab layout: Dashboard / Settings / System
// - Real-time CAN signal dashboard with instrument panel
// - Segment controls (no native <select>)
// - Custom modal & toast (no alert / confirm)
// - Bilingual (zh / en), localStorage persistence
// - Optimised for Tesla in-car browser (1280×1080 viewport)
#pragma once

const char WEB_UI_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#000000">
<title>Tesla CAN Mod</title>
<style>
:root{
  --bg:#000;--card:rgba(28,28,30,.92);--card2:rgba(38,38,40,.85);
  --border:rgba(255,255,255,.08);--border2:rgba(255,255,255,.12);
  --text:#fff;--text2:#8e8e93;--text3:#6e6e73;
  --blue:#3478f6;--green:#30d158;--red:#ff453a;--amber:#ffd60a;
  --tesla:#e31937;--radius:14px;--radius-sm:10px;
  --overlay-bg:rgba(0,0,0,.06);--overlay-bg2:rgba(0,0,0,.03);
  --overlay-border:rgba(255,255,255,.06);--overlay-border2:rgba(255,255,255,.04);
  --log-bg:rgba(0,0,0,.5);--log-text:#a8a8b3;--log-ts:#5e5e63
}
:root.light{
  --bg:#f2f2f7;--card:rgba(255,255,255,.94);--card2:rgba(245,245,247,.9);
  --border:rgba(0,0,0,.06);--border2:rgba(0,0,0,.1);
  --text:#1c1c1e;--text2:#636366;--text3:#8e8e93;
  --blue:#007aff;--green:#34c759;--red:#ff3b30;--amber:#ff9500;
  --tesla:#e31937;--overlay-bg:rgba(0,0,0,.04);--overlay-bg2:rgba(0,0,0,.02);
  --overlay-border:rgba(0,0,0,.06);--overlay-border2:rgba(0,0,0,.04);
  --log-bg:rgba(0,0,0,.04);--log-text:#3a3a3c;--log-ts:#8e8e93
}
*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{height:100%;background:var(--bg);color:var(--text)}
body{
  font-family:-apple-system,BlinkMacSystemFont,"SF Pro Display","PingFang SC","Helvetica Neue",sans-serif;
  font-size:16px;line-height:1.5;-webkit-font-smoothing:antialiased;
  display:flex;flex-direction:column;overflow:hidden
}

/* ── App shell ──────────────────────────────────── */
.header{
  flex:0 0 auto;display:flex;align-items:center;justify-content:space-between;
  padding:10px 20px;gap:12px;
  background:linear-gradient(180deg,rgba(20,20,22,.98) 0%,rgba(10,10,12,.95) 100%);
  border-bottom:1px solid var(--border);z-index:50
}
.light .header{
  background:linear-gradient(180deg,rgba(255,255,255,.98) 0%,rgba(245,245,247,.95) 100%)
}
.main{
  flex:1 1 auto;overflow-y:auto;overflow-x:hidden;
  -webkit-overflow-scrolling:touch;
  padding:16px 16px 24px;
  scrollbar-width:thin;scrollbar-color:rgba(255,255,255,.1) transparent
}
.main::-webkit-scrollbar{width:6px}
.main::-webkit-scrollbar-thumb{background:rgba(255,255,255,.1);border-radius:3px}
.light .main{scrollbar-color:rgba(0,0,0,.15) transparent}
.light .main::-webkit-scrollbar-thumb{background:rgba(0,0,0,.15)}
.tab-bar{
  flex:0 0 auto;display:flex;
  background:rgba(18,18,20,.96);
  backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);
  border-top:1px solid var(--border);
  padding:6px 0 env(safe-area-inset-bottom,8px);z-index:50
}
.light .tab-bar{background:rgba(255,255,255,.92)}

/* ── Header ─────────────────────────────────────── */
.brand{display:flex;align-items:center;gap:10px}
.logo{
  width:34px;height:34px;border-radius:8px;
  background:linear-gradient(135deg,var(--tesla),#a3142b);
  display:flex;align-items:center;justify-content:center;
  font-weight:900;font-size:17px;color:#fff;
  box-shadow:0 2px 12px rgba(227,25,55,.4)
}
.brand-name{font-weight:700;font-size:16px;letter-spacing:.4px}
.brand-sub{font-size:10px;color:var(--text2);margin-top:1px}
.hdr-right{display:flex;align-items:center;gap:8px}
.pill-btn{
  display:inline-flex;align-items:center;gap:4px;
  background:rgba(255,255,255,.07);color:var(--text);
  border:1px solid var(--border2);border-radius:8px;
  padding:6px 12px;font-size:11px;font-weight:700;
  cursor:pointer;transition:background .2s;white-space:nowrap;
  font-family:inherit;text-decoration:none
}
.pill-btn:hover{background:rgba(255,255,255,.12)}
.light .pill-btn{background:rgba(0,0,0,.04)}
.light .pill-btn:hover{background:rgba(0,0,0,.08)}
.pill-btn.nav{background:rgba(52,120,246,.12);border-color:rgba(52,120,246,.3);color:var(--blue)}
.conn-dot{
  width:6px;height:6px;border-radius:50%;
  background:var(--green);box-shadow:0 0 6px var(--green);
  display:inline-block
}
.conn-dot.off{background:var(--red);box-shadow:0 0 6px var(--red)}

/* ── Tab bar ────────────────────────────────────── */
.tab{
  flex:1;display:flex;flex-direction:column;align-items:center;gap:4px;
  padding:10px 4px 6px;background:none;border:none;
  color:var(--text3);font-size:14px;font-weight:600;
  cursor:pointer;transition:color .2s;font-family:inherit;
  letter-spacing:.3px
}
.tab svg{width:32px;height:32px;fill:currentColor;transition:fill .2s}
.tab.active{color:var(--blue)}
.tab:active{opacity:.7}

/* ── Pages ──────────────────────────────────────── */
.page{display:none}
.page.active{display:block;animation:fadeUp .2s ease;will-change:opacity,transform}
@keyframes fadeUp{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}
.grid{display:grid;gap:14px;grid-template-columns:1fr}
@media(min-width:768px){.grid{grid-template-columns:1fr 1fr}}
@media(min-width:1200px){.grid{grid-template-columns:1fr 1fr 1fr;gap:16px}}
.span-full{grid-column:1/-1}

/* ── Cards ──────────────────────────────────────── */
.card{
  background:var(--card);
  border:1px solid var(--border);
  border-radius:var(--radius);
  padding:18px 20px;
  position:relative;
  contain:content
}
.card-title{
  font-size:18px;color:var(--text2);font-weight:700;
  text-transform:uppercase;letter-spacing:1.5px;margin-bottom:16px;
  display:flex;align-items:center;gap:8px
}
.dev-tag{
  font-size:9px;padding:2px 6px;border-radius:4px;
  background:rgba(255,214,10,.12);color:var(--amber);
  font-weight:700;letter-spacing:.5px
}

/* ── Speed hero display ────────────────────────── */
.speed-hero{
  display:flex;align-items:center;justify-content:center;gap:24px;
  padding:16px 0 12px;position:relative
}
.speed-center{text-align:center;position:relative}
.speed-num{
  font-size:96px;font-weight:800;line-height:1;
  font-variant-numeric:tabular-nums;letter-spacing:-2px;
  color:var(--text)
}
.speed-unit{font-size:16px;color:var(--text2);font-weight:600;letter-spacing:1px;margin-top:2px}
.gear-badge{
  display:flex;align-items:center;justify-content:center;
  width:52px;height:52px;border-radius:14px;
  background:rgba(255,255,255,.06);border:2px solid rgba(255,255,255,.1);
  font-size:28px;font-weight:800;color:var(--text);
  flex-shrink:0
}
.gear-badge.gear-d{border-color:rgba(48,209,88,.35);color:var(--green)}
.gear-badge.gear-r{border-color:rgba(255,69,58,.35);color:var(--red)}
.gear-badge.gear-n{border-color:rgba(255,214,10,.35);color:var(--amber)}

/* ── FSD pill (stats bar) ──────────────────────── */
.stats-bar{
  display:flex;align-items:center;gap:14px;
  padding:10px 16px;
  background:var(--card);border:1px solid var(--border);
  border-radius:var(--radius);flex-wrap:wrap
}
.fsd-pill{
  display:flex;align-items:center;gap:8px;
  padding:7px 16px;border-radius:20px;
  background:rgba(255,255,255,.06);border:2px solid rgba(255,255,255,.1);
  font-weight:700;font-size:15px;transition:all .3s
}
.fsd-pill.on{background:rgba(48,209,88,.1);border-color:rgba(48,209,88,.35)}
.fsd-pill.on .fsd-val{color:var(--green)}
.fsd-lbl{font-size:11px;color:var(--text2);letter-spacing:1px;text-transform:uppercase}
.fsd-val{font-size:16px;font-weight:800}
.fsd-sep{color:var(--text3)}
.stats-inline{display:flex;gap:18px;flex-wrap:wrap;margin-left:auto}
.si{display:flex;align-items:baseline;gap:5px}
.si-v{font-size:15px;font-weight:700;font-variant-numeric:tabular-nums}
.si-v.c-green{color:var(--green)}.si-v.c-blue{color:var(--blue)}
.si-v.c-amber{color:var(--amber)}.si-v.c-red{color:var(--red)}
.si-l{font-size:11px;color:var(--text3);font-weight:600}

/* ── Light state pill ──────────────────────────── */
.light-pill{
  display:inline-flex;align-items:center;gap:6px;
  padding:5px 12px;border-radius:20px;
  background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);
  font-size:12px;font-weight:700;color:var(--text2)
}
.light-pill.on{background:rgba(255,214,10,.08);border-color:rgba(255,214,10,.25);color:var(--amber)}
.light-icon{font-size:14px}

/* ── Enhanced Dashboard 6-cell grid ────────────── */
.ed-grid{display:grid;grid-template-columns:repeat(6,1fr);gap:10px}
@media(max-width:768px){.ed-grid{grid-template-columns:repeat(3,1fr)}}
@media(max-width:480px){.ed-grid{grid-template-columns:repeat(2,1fr)}}
.ed-cell{
  background:rgba(255,255,255,.03);border:1px solid rgba(255,255,255,.04);
  border-radius:var(--radius-sm);padding:14px 10px;text-align:center;
  position:relative;overflow:hidden
}
.ed-val{font-size:24px;font-weight:800;color:var(--text);font-variant-numeric:tabular-nums;position:relative;z-index:1}
.ed-lbl{font-size:11px;color:var(--text3);text-transform:uppercase;letter-spacing:.8px;margin-top:4px;font-weight:600;position:relative;z-index:1}

/* Progress bar inside ed-cell */
.ed-bar{
  position:absolute;bottom:0;left:0;height:4px;border-radius:0 2px 0 0;
  transition:width .3s ease
}
.ed-bar.green{background:linear-gradient(90deg,#1a7a35,var(--green))}
.ed-bar.red{background:linear-gradient(90deg,#7a1a1a,var(--red))}
.ed-bar.yellow{background:linear-gradient(90deg,#7a6a1a,#e6c200)}
.ed-bar.blue{background:linear-gradient(90deg,#1a3a7a,var(--blue))}

/* Hands-on level dots */
.hands-dots{display:flex;gap:5px;justify-content:center;margin-top:2px}
.hands-dot{
  width:10px;height:10px;border-radius:3px;
  background:rgba(255,255,255,.1);transition:background .3s
}
.hands-dot.active{background:var(--blue)}
.hands-dot.active.lv2{background:var(--amber)}
.hands-dot.active.lv3{background:var(--red)}

/* Follow distance bars */
.follow-bars{display:flex;gap:3px;justify-content:center;align-items:flex-end;height:24px}
.follow-bar{
  width:6px;border-radius:2px;
  background:rgba(255,255,255,.08);transition:background .3s,height .3s
}
.follow-bar.active{background:var(--blue)}

/* ── Chart ─────────────────────────────────────── */
.chart-wrap{position:relative;width:100%;height:160px;margin-top:8px}
.chart-wrap canvas{width:100%;height:100%;border-radius:8px;background:rgba(0,0,0,.15)}
.light .chart-wrap canvas{background:rgba(0,0,0,.03)}
.chart-legend{display:flex;gap:16px;justify-content:center;margin-top:6px;font-size:11px;font-weight:600;color:var(--text2)}
.chart-legend .cl-dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:4px}

/* ── CAN frame viewer ──────────────────────────── */
.can-table{width:100%;border-collapse:collapse;font-size:12px;font-variant-numeric:tabular-nums}
.can-table th{text-align:left;color:var(--text2);font-weight:700;padding:6px 8px;border-bottom:1px solid var(--border);font-size:10px;text-transform:uppercase;letter-spacing:.5px}
.can-table td{padding:5px 8px;border-bottom:1px solid rgba(255,255,255,.03);font-family:"SF Mono",Menlo,monospace;white-space:nowrap}
.light .can-table td{border-bottom:1px solid rgba(0,0,0,.04)}
.can-table tr:hover td{background:rgba(52,120,246,.06)}
.can-filter{display:flex;gap:8px;margin-bottom:10px;align-items:center}
.can-filter input{background:rgba(255,255,255,.06);border:1px solid var(--border2);border-radius:8px;padding:8px 12px;color:var(--text);font-size:13px;font-family:inherit;flex:1;max-width:240px}
.light .can-filter input{background:rgba(0,0,0,.04)}
.can-scroll{max-height:320px;overflow-y:auto;scrollbar-width:thin}

/* ── Speed limits compact row ──────────────────── */
.sl-row{
  display:flex;gap:10px;flex-wrap:wrap;align-items:center;
  padding:10px 0
}
.sl-chip{
  display:flex;flex-direction:column;align-items:center;
  background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.06);
  border-radius:var(--radius-sm);padding:8px 14px;min-width:70px
}
.sl-chip-val{font-size:20px;font-weight:800;font-variant-numeric:tabular-nums;color:var(--text)}
.sl-chip-lbl{font-size:9px;color:var(--text3);text-transform:uppercase;letter-spacing:.5px;font-weight:600;margin-top:2px}
.sl-chip.active{border-color:var(--blue);background:rgba(52,120,246,.12)}
.sl-chip:active{transform:scale(.95)}

/* ── Bottom stats bar ──────────────────────────── */
.bottom-stats{
  display:flex;align-items:center;gap:16px;flex-wrap:wrap;
  padding:10px 16px;
  background:rgba(255,255,255,.02);border:1px solid rgba(255,255,255,.04);
  border-radius:var(--radius-sm);margin-top:14px;
  font-size:12px;color:var(--text3)
}
.bottom-stats .bs-item{display:flex;align-items:center;gap:5px}
.bottom-stats .bs-val{font-weight:700;color:var(--text2);font-variant-numeric:tabular-nums}
.bottom-stats .bs-lbl{font-weight:600}

/* ── Mode cards (speed profile) ─────────────────── */
.mode-row{display:grid;grid-template-columns:repeat(5,1fr);gap:10px}
.mode-row-6{display:grid;grid-template-columns:repeat(6,1fr);gap:10px}
@media(max-width:560px){.mode-row,.mode-row-6{grid-template-columns:repeat(3,1fr)}}
.mode-card{
  background:rgba(255,255,255,.04);border:2px solid rgba(255,255,255,.06);
  border-radius:14px;padding:18px 8px;text-align:center;
  cursor:pointer;transition:all .2s;user-select:none
}
.mode-card:active{transform:scale(.96)}
.mode-card.active{border-color:var(--blue);background:rgba(52,120,246,.1)}
.mode-card.disabled{opacity:.35;pointer-events:none}
.mode-name{font-size:22px;font-weight:800;color:var(--text);letter-spacing:2px}

/* ── Segment control ────────────────────────────── */
.seg{
  display:inline-flex;background:rgba(255,255,255,.06);
  border-radius:var(--radius-sm);padding:3px;gap:2px
}
.seg-btn{
  padding:12px 22px;border:none;border-radius:8px;
  background:transparent;color:var(--text2);
  font-size:16px;font-weight:600;cursor:pointer;
  transition:all .2s;font-family:inherit;white-space:nowrap
}
.seg-btn.active{background:rgba(255,255,255,.14);color:var(--text)}
.seg-btn:active{opacity:.7}
.seg-btn.disabled{opacity:.3;pointer-events:none}

/* ── Rows / Toggles ─────────────────────────────── */
.row{
  display:flex;align-items:center;justify-content:space-between;
  gap:14px;padding:12px 0;border-bottom:1px solid rgba(255,255,255,.05)
}
.row:last-child{border-bottom:none}
.row-info{flex:1 1 auto;min-width:0}
.row-name{font-size:18px;font-weight:600;color:var(--text)}
.row-meta{font-size:14px;color:var(--text3);margin-top:3px;line-height:1.4}
.row-ctrl{flex:0 0 auto}

.toggle{display:inline-block;position:relative;width:52px;height:30px;cursor:pointer;vertical-align:middle}
.toggle input{display:none}
.toggle-track{
  position:absolute;inset:0;background:rgba(255,255,255,.12);
  border-radius:26px;transition:.25s
}
.light .toggle-track{background:rgba(0,0,0,.1)}
.toggle-track::before{
  content:"";position:absolute;width:26px;height:26px;left:2px;top:2px;
  background:#fff;border-radius:50%;transition:.25s;
  box-shadow:0 2px 6px rgba(0,0,0,.4)
}
input:checked+.toggle-track{background:var(--green)}
input:checked+.toggle-track::before{transform:translateX(22px)}
input:disabled+.toggle-track{opacity:.35;cursor:not-allowed}

/* ── Buttons ────────────────────────────────────── */
.btn{
  width:100%;padding:13px 16px;border:none;border-radius:11px;
  font-size:14px;font-weight:700;cursor:pointer;
  letter-spacing:.4px;font-family:inherit;transition:all .2s
}
.btn-primary{background:var(--green);color:#0a0a0a}
.btn-primary:hover:not(:disabled){filter:brightness(1.1)}
.btn-primary:disabled{opacity:.35;cursor:not-allowed}
.btn-danger{background:var(--tesla);color:#fff}
.btn-danger:hover:not(:disabled){filter:brightness(1.1)}
.btn-danger:disabled{opacity:.35;cursor:not-allowed}

/* ── Inputs ─────────────────────────────────────── */
.txt{
  background:rgba(255,255,255,.06);color:var(--text);
  border:1px solid var(--border2);border-radius:var(--radius-sm);
  padding:11px 14px;font-size:14px;width:100%;
  font-family:inherit;transition:border-color .2s
}
.txt:focus{outline:none;border-color:var(--blue);background:rgba(255,255,255,.08)}
.txt::placeholder{color:var(--text3)}
.field-stack{display:flex;flex-direction:column;gap:5px;margin:8px 0}
.field-stack label{font-size:12px;color:var(--text2);font-weight:600;letter-spacing:.3px}
.sr-row{display:flex;gap:6px;align-items:center;margin-bottom:6px}
.sr-row .txt{width:auto;flex:1;padding:8px 10px;font-size:14px;text-align:center}
.sr-del{min-width:32px;height:32px;padding:0;display:flex;align-items:center;justify-content:center;font-size:14px;color:var(--text3)}
.sr-del:hover{color:#ff453a}

/* ── File / OTA ─────────────────────────────────── */
.file-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-bottom:12px}
.file-pick{
  display:inline-block;background:rgba(255,255,255,.08);color:var(--text);
  border:1px solid var(--border2);border-radius:8px;
  padding:9px 16px;font-size:12px;font-weight:700;cursor:pointer
}
.file-pick:hover{background:rgba(255,255,255,.14)}
.file-name{font-size:12px;color:var(--text2);flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.progress{width:100%;height:5px;background:rgba(255,255,255,.08);border-radius:3px;margin-top:12px;overflow:hidden;display:none}
.progress.active{display:block}
.progress-bar{height:100%;background:var(--green);border-radius:3px;width:0%;transition:width .25s}

/* ── Tags ───────────────────────────────────────── */
.tag{
  display:inline-flex;align-items:center;gap:5px;
  font-size:11px;font-weight:700;padding:3px 8px;
  border-radius:6px;background:rgba(255,255,255,.06);
  color:var(--text2);text-transform:uppercase;letter-spacing:.5px
}
.tag.on{background:rgba(48,209,88,.12);color:var(--green)}
.tag.err{background:rgba(255,69,58,.12);color:var(--red)}
.dot-sm{width:6px;height:6px;border-radius:50%;display:inline-block;background:var(--text3)}
.dot-sm.on{background:var(--green);box-shadow:0 0 4px var(--green)}
.dot-sm.err{background:var(--red);box-shadow:0 0 4px var(--red)}

/* ── Log ────────────────────────────────────────── */
.log{
  background:rgba(0,0,0,.5);border-radius:var(--radius-sm);
  padding:12px 14px;height:280px;overflow-y:auto;
  font-family:"SF Mono",Monaco,Menlo,Consolas,monospace;
  font-size:12px;line-height:1.55;border:1px solid rgba(255,255,255,.05)
}
.log::-webkit-scrollbar{width:6px}
.log::-webkit-scrollbar-thumb{background:rgba(255,255,255,.08);border-radius:3px}
.log-line{padding:1px 0;color:#a8a8b3;word-break:break-word}
.log-line .ts{color:#5e5e63;margin-right:8px;font-weight:600}

/* ── Hint ───────────────────────────────────────── */
.hint{
  font-size:14px;color:var(--text3);line-height:1.45;
  padding:12px 16px;background:rgba(255,255,255,.03);
  border-radius:8px;border-left:3px solid var(--amber);margin-top:10px
}

/* ── Speed-limit assistant ──────────────────────── */
.sl-modes{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:16px}
.sl-mode{
  background:rgba(255,255,255,.04);border:2px solid rgba(255,255,255,.06);
  border-radius:14px;padding:18px 8px;text-align:center;
  cursor:pointer;transition:all .2s;user-select:none
}
.sl-mode:active{transform:scale(.96)}
.sl-mode.active{border-color:var(--blue);background:rgba(52,120,246,.1)}
.sl-mode-name{font-size:22px;font-weight:800;color:var(--text);letter-spacing:2px}
.sl-panel{display:none;animation:fadeUp .2s ease}
.sl-panel.active{display:block}
.sl-info{font-size:15px;color:var(--text2);line-height:1.5;padding:14px 16px;background:rgba(255,255,255,.03);border-radius:10px}
.rule-list{margin-top:12px}
.rule-item{
  display:flex;align-items:center;gap:10px;padding:10px 14px;
  background:rgba(255,255,255,.04);border-radius:10px;margin-top:6px;
  font-size:16px;font-weight:600;font-variant-numeric:tabular-nums;
  transition:background .15s
}
.rule-item:active{background:rgba(255,255,255,.08)}
.rule-range{color:var(--text);flex:1}
.rule-arrow{color:var(--text3);font-size:18px}
.rule-target{color:var(--blue);min-width:80px;text-align:right}
.rule-del{
  width:28px;height:28px;border-radius:7px;border:none;
  background:rgba(255,69,58,.12);color:var(--red);
  font-size:16px;font-weight:700;cursor:pointer;
  display:flex;align-items:center;justify-content:center;
  transition:all .15s;flex-shrink:0
}
.rule-del:active{background:rgba(255,69,58,.25);transform:scale(.9)}
.rule-add{
  display:flex;align-items:center;justify-content:center;gap:6px;
  width:100%;padding:14px;margin-top:10px;border:2px dashed rgba(255,255,255,.1);
  border-radius:10px;background:none;color:var(--text2);
  font-size:15px;font-weight:600;cursor:pointer;font-family:inherit;
  transition:all .2s
}
.rule-add:active{border-color:var(--blue);color:var(--blue)}
.modal-form{text-align:left;display:flex;flex-direction:column;gap:12px}
.modal-form label{font-size:12px;color:var(--text2);font-weight:600}
.modal-form .txt{margin-top:4px}

/* ── Modal ──────────────────────────────────────── */
.modal-overlay{
  position:fixed;inset:0;z-index:200;
  display:none;align-items:center;justify-content:center;
  background:rgba(0,0,0,.6);backdrop-filter:blur(8px);-webkit-backdrop-filter:blur(8px)
}
.modal-overlay.show{display:flex}
.modal-box{
  background:rgba(44,44,46,.98);border:1px solid rgba(255,255,255,.12);
  border-radius:18px;padding:28px 24px 20px;
  max-width:380px;width:90%;text-align:center;
  box-shadow:0 20px 60px rgba(0,0,0,.6);
  animation:modalIn .2s ease
}
@keyframes modalIn{from{opacity:0;transform:scale(.92)}to{opacity:1;transform:none}}
.modal-title{font-size:19px;font-weight:700;margin-bottom:12px}
.modal-body{font-size:15px;color:var(--text2);line-height:1.5;margin-bottom:20px}
.modal-body .txt{margin-top:8px;text-align:left}
.modal-actions{display:flex;gap:10px}
.modal-btn{
  flex:1;padding:12px;border:none;border-radius:11px;
  font-size:14px;font-weight:700;cursor:pointer;font-family:inherit;transition:all .15s
}
.modal-btn:active{transform:scale(.97)}
.modal-btn.cancel{background:rgba(255,255,255,.1);color:var(--text)}
.modal-btn.primary{background:var(--blue);color:#fff}
.modal-btn.danger{background:var(--red);color:#fff}

/* ── Toast ──────────────────────────────────────── */
.toast{
  position:fixed;top:-60px;left:50%;transform:translateX(-50%);
  background:rgba(28,28,30,.95);backdrop-filter:blur(16px);-webkit-backdrop-filter:blur(16px);
  color:var(--text);padding:12px 24px;border-radius:12px;
  font-size:15px;font-weight:600;z-index:300;
  transition:top .3s ease;border:1px solid var(--border2);
  white-space:nowrap;max-width:90%;
  will-change:top
}
.toast.show{top:16px}
.toast.ok{border-color:rgba(48,209,88,.25)}
.toast.err{border-color:rgba(255,69,58,.25)}

/* ── Connection error ───────────────────────────── */
.conn-err{
  position:fixed;top:0;left:0;right:0;
  background:var(--red);color:#fff;font-size:12px;
  padding:8px 16px;text-align:center;font-weight:700;
  z-index:100;display:none
}
.conn-err.show{display:block}

/* ── Responsive fine-tune ───────────────────────── */
@media(min-width:1200px){
  .main{padding:20px 24px 28px}
  .header{padding:10px 24px}
  .speed-num{font-size:108px}
}
@media(max-width:480px){
  .stats-inline{gap:10px}
  .speed-num{font-size:72px}
  .sl-row{gap:6px}
  .sl-chip{padding:6px 10px;min-width:56px}
  .sl-chip-val{font-size:16px}
}

/* ── Light theme overrides ──────────────────────── */
.light .gear-badge{background:rgba(0,0,0,.04);border-color:rgba(0,0,0,.1)}
.light .stats-bar{background:var(--card)}
.light .fsd-pill{background:rgba(0,0,0,.04);border-color:rgba(0,0,0,.1)}
.light .ed-cell{background:rgba(0,0,0,.03);border-color:rgba(0,0,0,.04)}
.light .hands-dot{background:rgba(0,0,0,.1)}
.light .follow-bar{background:rgba(0,0,0,.08)}
.light .sl-chip{background:rgba(0,0,0,.03);border-color:rgba(0,0,0,.06)}
.light .bottom-stats{background:rgba(0,0,0,.02);border-color:rgba(0,0,0,.04)}
.light .mode-card{background:rgba(0,0,0,.03);border-color:rgba(0,0,0,.06)}
.light .seg{background:rgba(0,0,0,.05)}
.light .seg-btn.active{background:rgba(0,0,0,.1)}
.light .row{border-bottom-color:rgba(0,0,0,.06)}
.light .txt{background:rgba(0,0,0,.04);border-color:rgba(0,0,0,.1)}
.light .txt:focus{background:rgba(0,0,0,.06)}
.light .txt::placeholder{color:var(--text3)}
.light .file-pick{background:rgba(0,0,0,.05);border-color:rgba(0,0,0,.1)}
.light .file-pick:hover{background:rgba(0,0,0,.08)}
.light .tag{background:rgba(0,0,0,.05)}
.light .progress{background:rgba(0,0,0,.06)}
.light .log{background:var(--log-bg);border-color:rgba(0,0,0,.06)}
.light .log-line{color:var(--log-text)}
.light .log-line .ts{color:var(--log-ts)}
.light .hint{background:rgba(0,0,0,.03)}
.light .sl-mode{background:rgba(0,0,0,.03);border-color:rgba(0,0,0,.06)}
.light .rule-item{background:rgba(0,0,0,.03)}
.light .rule-item:active{background:rgba(0,0,0,.06)}
.light .rule-add{border-color:rgba(0,0,0,.12)}
.light .modal-box{background:rgba(255,255,255,.98);border-color:rgba(0,0,0,.1);box-shadow:0 20px 60px rgba(0,0,0,.15)}
.light .modal-btn.cancel{background:rgba(0,0,0,.06)}
.light .toast{background:rgba(255,255,255,.95)}
.light .ed-bar.green{background:linear-gradient(90deg,#a8e6b4,var(--green))}
.light .ed-bar.red{background:linear-gradient(90deg,#f5a8a8,var(--red))}
.light .ed-bar.yellow{background:linear-gradient(90deg,#f5e6a8,#e6c200)}
.light .ed-bar.blue{background:linear-gradient(90deg,#a8c8f5,var(--blue))}
.light .pill-btn.nav{background:rgba(0,122,255,.08);border-color:rgba(0,122,255,.2)}
.light .dev-tag{background:rgba(255,149,0,.1);color:var(--amber)}
.light .light-pill{background:rgba(0,0,0,.03);border-color:rgba(0,0,0,.06)}
.light .light-pill.on{background:rgba(255,149,0,.08);border-color:rgba(255,149,0,.2)}
.light .btn-primary{color:#fff}
</style>
</head>
<body>

<!-- ═══ Modal ═══ -->
<div class="modal-overlay" id="modal">
  <div class="modal-box">
    <div class="modal-title" id="mTitle"></div>
    <div class="modal-body" id="mBody"></div>
    <div class="modal-actions">
      <button class="modal-btn cancel" id="mCancel" onclick="mDismiss(false)"></button>
      <button class="modal-btn primary" id="mOk" onclick="mDismiss(true)"></button>
    </div>
  </div>
</div>

<!-- ═══ Toast ═══ -->
<div class="toast" id="toast"></div>

<!-- ═══ Connection error bar ═══ -->
<div class="conn-err" id="connErr"><span id="errText"></span></div>

<!-- ═══ Header ═══ -->
<header class="header">
  <div class="brand">
    <div class="logo">T</div>
    <div>
      <div class="brand-name">Tesla CAN Mod</div>
      <div class="brand-sub"><span id="iVer">v---</span> · <span id="iIp">--</span> · <span id="iVp">--</span></div>
    </div>
  </div>
  <div class="hdr-right">
    <div class="fsd-pill" id="fsdBadge" style="padding:5px 12px;font-size:13px">
      <span class="fsd-lbl" style="font-size:10px">FSD</span>
      <span class="fsd-val" id="fsdVal" style="font-size:14px">OFF</span>
      <span class="fsd-sep">·</span>
      <span id="hwLabel" style="font-size:12px">--</span>
    </div>
    <span class="pill-btn" style="cursor:default;font-variant-numeric:tabular-nums;gap:10px;font-size:13px;color:var(--text3)">
      <span>RX<b style="color:var(--text);margin-left:3px" id="bsRx">0</b></span>
      <span>TX<b style="color:var(--text);margin-left:3px" id="bsTx">0</b></span>
      <span>ERR<b style="color:var(--text);margin-left:3px" id="bsErr">0</b></span>
      <span id="bsCanState" style="color:var(--green)">--</span>
      <span id="bsUp" style="color:var(--text2)">0s</span>
    </span>
    <span class="pill-btn" style="cursor:default"><span class="conn-dot off" id="connDot"></span> <span id="iConn">--</span></span>
    <button class="pill-btn" id="iLangBtn" onclick="toggleLang()">EN</button>
  </div>
</header>

<!-- ═══ Main scrollable area ═══ -->
<div class="main">

  <!-- ──── PAGE: Dashboard ──── -->
  <div class="page active" id="pageDash">
    <div class="grid">

      <!-- OTA protection warning banner -->
      <div class="card span-full" id="otaBanner" style="display:none;background:var(--warn-bg,#fff3cd);border:1px solid var(--warn-border,#ffc107);padding:12px 16px;text-align:center">
        <span style="font-weight:700;color:var(--warn-text,#856404)" id="iOtaPause">车辆 OTA 进行中，CAN 修改已暂停</span>
      </div>

      <!-- ═══ Card 1: 实时信号 ═══ -->
      <div class="card span-full">
        <div class="card-title" id="iCardED">实时信号</div>
        <div class="mode-row" style="grid-template-columns:repeat(6,1fr)">
          <div class="ed-cell" id="cellAccel">
            <div class="ed-val" id="edAccel">0%</div>
            <div class="ed-bar green" id="barAccel" style="width:0%"></div>
            <div class="ed-lbl" id="iEdAccel">油门深度</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="slFused">--</div>
            <div class="ed-lbl" id="iSlFused">融合限速</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="slMap">--</div>
            <div class="ed-lbl" id="iSlMap">地图限速</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="slVehicle">--</div>
            <div class="ed-lbl" id="iSlVehicle">车辆限速</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="edHands">--</div>
            <div class="hands-dots" id="handsDots">
              <div class="hands-dot" data-lv="1"></div>
              <div class="hands-dot" data-lv="2"></div>
              <div class="hands-dot" data-lv="3"></div>
            </div>
            <div class="ed-lbl" id="iEdHands">方向盘握力</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="edFollow">--</div>
            <div class="follow-bars" id="followBars">
              <div class="follow-bar" data-lv="1" style="height:6px"></div>
              <div class="follow-bar" data-lv="2" style="height:9px"></div>
              <div class="follow-bar" data-lv="3" style="height:12px"></div>
              <div class="follow-bar" data-lv="4" style="height:15px"></div>
              <div class="follow-bar" data-lv="5" style="height:18px"></div>
              <div class="follow-bar" data-lv="6" style="height:21px"></div>
              <div class="follow-bar" data-lv="7" style="height:24px"></div>
            </div>
            <div class="ed-lbl" id="iEdFollow">跟车距离</div>
          </div>
        </div>
      </div>

      <!-- ═══ Card 1b: 电池状态 ═══ -->
      <div class="card span-full">
        <div class="card-title" id="iCardBat">电池状态</div>
        <div class="mode-row" style="grid-template-columns:repeat(5,1fr)">
          <div class="ed-cell">
            <div class="ed-val" id="batSoc">--%</div>
            <div class="ed-bar green" id="barSoc" style="width:0%"></div>
            <div class="ed-lbl" id="iBatSoc">电量</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="batVolt">--</div>
            <div class="ed-lbl" id="iBatVolt">电压 (V)</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="batCurr">--</div>
            <div class="ed-lbl" id="iBatCurr">电流 (A)</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="batTMin">--</div>
            <div class="ed-lbl" id="iBatTMin">最低温度</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="batTMax">--</div>
            <div class="ed-lbl" id="iBatTMax">最高温度</div>
          </div>
        </div>
      </div>

      <!-- ═══ Card 1c: 实时曲线 ═══ -->
      <div class="card span-full">
        <div class="card-title" id="iCardChart">实时曲线</div>
        <div class="chart-wrap"><canvas id="chartCanvas"></canvas></div>
        <div class="chart-legend">
          <span><span class="cl-dot" style="background:#3478f6"></span><span id="iChartSpeed">车速</span></span>
          <span><span class="cl-dot" style="background:#30d158"></span><span id="iChartSoc">电量</span></span>
          <span><span class="cl-dot" style="background:#ff9500"></span><span id="iChartPower">功率</span></span>
        </div>
      </div>

      <!-- ═══ Card 1d: CAN 帧查看器 ═══ -->
      <div class="card span-full">
        <div class="card-title" id="iCardCanView">CAN 帧查看器</div>
        <div class="can-filter">
          <input type="text" id="canFilterInput" placeholder="Filter ID (e.g. 0x132 or 306)">
          <span style="font-size:12px;color:var(--text3)" id="canIdCount">0 IDs</span>
        </div>
        <div class="can-scroll" id="canTableWrap">
          <table class="can-table">
            <thead><tr><th>ID</th><th>DEC</th><th>DLC</th><th>DATA</th><th>COUNT</th><th>Hz</th></tr></thead>
            <tbody id="canTableBody"></tbody>
          </table>
        </div>
      </div>

      <!-- ═══ Card 2: 限速与速度偏移比 — copied from Card 3 structure ═══ -->
      <div class="card span-full" id="offsetCard" style="display:none">
        <div class="card-title" id="iCardSLimits">限速与速度偏移比</div>
        <div class="row" style="border-bottom:none">
          <div class="row-info"><span class="row-name">模式</span></div>
          <div class="row-ctrl">
            <div class="seg" id="sgOffsetMode">
              <button class="seg-btn active" data-val="manual" onclick="setOffsetMode('manual')">手动</button>
              <button class="seg-btn" data-val="smart" onclick="setOffsetMode('smart')">智能</button>
            </div>
          </div>
        </div>
        <div id="offsetManual">
          <div class="mode-row-6" id="offsetBtns">
            <div class="mode-card active" data-off="0" onclick="setOffset(0)"><div class="mode-name">0</div></div>
            <div class="mode-card" data-off="10" onclick="setOffset(10)"><div class="mode-name">10</div></div>
            <div class="mode-card" data-off="20" onclick="setOffset(20)"><div class="mode-name">20</div></div>
            <div class="mode-card" data-off="30" onclick="setOffset(30)"><div class="mode-name">30</div></div>
            <div class="mode-card" data-off="40" onclick="setOffset(40)"><div class="mode-name">40</div></div>
            <div class="mode-card" data-off="50" onclick="setOffset(50)"><div class="mode-name">50</div></div>
          </div>
        </div>
        <div id="offsetSmart" style="display:none">
          <div class="mode-row" id="smartRulesText"></div>
          <div class="row" style="border-bottom:none;margin-top:10px">
            <div class="row-info"><span class="row-name">当前偏移: <span style="color:var(--blue)" id="smartCurPct">--</span>%</span></div>
            <div class="row-ctrl"><button class="pill-btn" onclick="editSmartRules()" id="iSmartEdit">编辑规则</button></div>
          </div>
        </div>
      </div>

      <!-- ═══ Card 3: 驾驶模式 ═══ -->
      <div class="card span-full">
        <div class="card-title" id="iCardMode">驾驶模式</div>
        <div class="row" style="border-bottom:none">
          <div class="row-info"><span class="row-name" id="iLblSrc">档位源</span></div>
          <div class="row-ctrl">
            <div class="seg" id="sgProfSrc">
              <button class="seg-btn active" data-val="1" id="sgSrcAuto">自动</button>
              <button class="seg-btn" data-val="0" id="sgSrcMan">手动</button>
            </div>
          </div>
        </div>
        <div class="mode-row" id="modeRow">
          <div class="mode-card" data-val="3"><div class="mode-name" id="iModeMax">激进</div></div>
          <div class="mode-card" data-val="2"><div class="mode-name" id="iModeHurry">适中</div></div>
          <div class="mode-card active" data-val="1"><div class="mode-name" id="iModeNorm">默认</div></div>
          <div class="mode-card" data-val="0"><div class="mode-name" id="iModeChill">保守</div></div>
          <div class="mode-card" data-val="4"><div class="mode-name" id="iModeSloth">树懒</div></div>
        </div>
      </div>

      <!-- Speed Limit Assistant (hidden) -->
      <div class="card span-full" style="display:none">
        <div class="card-title"><span id="iCardSL">SPEED LIMIT ASSISTANT</span> <span class="dev-tag">DEV</span></div>

        <div class="sl-modes" id="slModes">
          <div class="sl-mode active" data-sl="off"><div class="sl-mode-name" id="iSLOff">关闭</div></div>
          <div class="sl-mode" data-sl="manual"><div class="sl-mode-name" id="iSLManName">手动</div></div>
          <div class="sl-mode" data-sl="smart"><div class="sl-mode-name" id="iSLSmartName">智能</div></div>
        </div>

        <!-- Panel: OFF -->
        <div class="sl-panel active" id="slPanelOff">
          <div class="sl-info" id="iSLOffInfo">限速助手已关闭，FSD 使用默认限速逻辑。</div>
        </div>

        <!-- Panel: Manual Priority -->
        <div class="sl-panel" id="slPanelManual">
          <div class="sl-info" id="iSLManInfo">FSD 限速以手动滚轮设定为准。经过限速标牌时不自动切换，始终保持你手动设定的速度。</div>
        </div>

        <!-- Panel: Smart Mapping -->
        <div class="sl-panel" id="slPanelSmart">
          <div class="sl-info" id="iSLSmartInfo">根据自定义规则自动调整检测到的限速值，让 FSD 按你的偏好行驶。</div>
          <div class="rule-list" id="ruleList"></div>
          <button class="rule-add" id="ruleAddBtn" onclick="addRule()">+ <span id="iSLAddRule">Add Rule</span></button>
        </div>
      </div>

    </div>
  </div>

  <!-- ──── PAGE: Settings ──── -->
  <div class="page" id="pageSet">
    <div class="grid">

      <!-- Hardware -->
      <div class="card span-full">
        <div class="card-title" id="iCardHW">HARDWARE</div>
        <div class="row">
          <div class="row-info">
            <span class="row-name" id="iLblHW">Hardware Version</span>
            <span class="row-meta" id="iMetaHW">Switch requires reboot</span>
          </div>
          <div class="row-ctrl">
            <div class="seg" id="sgHW">
              <button class="seg-btn" data-val="0">LEGACY</button>
              <button class="seg-btn active" data-val="1">HW3</button>
              <button class="seg-btn" data-val="2">HW4</button>
            </div>
          </div>
        </div>
        <div class="hint" id="iHwHint">HW4 硬件 + 固件 2026.8.x 或更旧 (FSD V13) 请选 HW3</div>
      </div>

      <!-- Feature Toggles -->
      <div class="card span-full">
        <div class="card-title" id="iCardFeat">FEATURES</div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblBypass">绕过 TLSSC 要求</span><span class="row-meta" id="iMetaBypass">无需开启交通灯停车标志控制</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tFsd" onchange="togFeat('/api/bypass-tlssc',this,'confirmBypass')"><span class="toggle-track"></span></label></div>
        </div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblIsa">ISA 限速提示音抑制</span><span class="row-meta" id="iMetaIsa">仅 HW4, 关闭限速 chime</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tIsa" onchange="togFeat('/api/isa-speed-chime-suppress',this)"><span class="toggle-track"></span></label></div>
        </div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblEvd">紧急车辆检测</span><span class="row-meta" id="iMetaEvd">FSD V14 警车/救护车识别</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tEvd" onchange="togFeat('/api/emergency-vehicle-detection',this)"><span class="toggle-track"></span></label></div>
        </div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblEap">增强 Autopilot</span><span class="row-meta" id="iMetaEap">EAP 功能</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tEap" onchange="togFeat('/api/enhanced-autopilot',this)"><span class="toggle-track"></span></label></div>
        </div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblNag">Autosteer Nag Killer</span><span class="row-meta" id="iMetaNag">需要 X179 接线</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tNag" onchange="togFeat('/api/nag-killer',this)"><span class="toggle-track"></span></label></div>
        </div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblCn">中国模式</span><span class="row-meta" id="iMetaCn">国行车机绕过 UI 选择检查</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tCn" onchange="togFeat('/api/china-mode',this)"><span class="toggle-track"></span></label></div>
        </div>
      </div>

      <!-- Module Enhancements — Smart Preheat -->
      <div class="card span-full">
        <div class="card-title" id="iCardLab">MODULE ENHANCEMENTS</div>

        <div class="row">
          <div class="row-info"><span class="row-name" id="iLblPreheat">预热模式</span><span class="row-meta" id="iMetaPreheat">注入 UI_tripPlanning (0x082) 500ms 周期</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tPreheat" onchange="togFeat('/api/preheat',this,'confirmPreheat')"><span class="toggle-track"></span></label></div>
        </div>

        <!-- Preheat status panel (visible when active) -->
        <div id="preheatPanel" style="display:none;padding:8px 16px 12px">
          <div class="mode-row" style="grid-template-columns:repeat(4,1fr);margin-bottom:8px">
            <div class="ed-cell">
              <div class="ed-val" id="phElapsed">--</div>
              <div class="ed-lbl" id="iPhElapsed">已运行</div>
            </div>
            <div class="ed-cell">
              <div class="ed-val" id="phTempMin">--</div>
              <div class="ed-lbl" id="iPhTempMin">电池最低温</div>
            </div>
            <div class="ed-cell">
              <div class="ed-val" id="phTempMax">--</div>
              <div class="ed-lbl" id="iPhTempMax">电池最高温</div>
            </div>
            <div class="ed-cell">
              <div class="ed-val" id="phSoc">--</div>
              <div class="ed-lbl" id="iPhSoc">电量</div>
            </div>
          </div>
          <div style="display:flex;gap:12px;flex-wrap:wrap;align-items:center;margin-top:4px">
            <label style="font-size:12px;color:var(--text2)" id="iPhAutoStopLbl">自动停止温度</label>
            <select class="txt" id="phAutoStopTemp" onchange="savePreheatConfig()" style="width:80px;padding:6px 8px;font-size:14px">
              <option value="-5">-5°C</option>
              <option value="0">0°C</option>
              <option value="5">5°C</option>
              <option value="10" selected>10°C</option>
              <option value="15">15°C</option>
              <option value="20">20°C</option>
              <option value="25">25°C</option>
            </select>
            <label style="font-size:12px;color:var(--text2)" id="iPhMaxDurLbl">最长时间</label>
            <select class="txt" id="phMaxDur" onchange="savePreheatConfig()" style="width:80px;padding:6px 8px;font-size:14px">
              <option value="10">10 min</option>
              <option value="20">20 min</option>
              <option value="30" selected>30 min</option>
              <option value="45">45 min</option>
              <option value="60">60 min</option>
            </select>
          </div>
        </div>

        <div class="hint" id="iLabHint">测试区功能必须接到真实 CAN 总线。空闲启用会导致 TX 错误飙升。</div>
      </div>

    </div>
  </div>

  <!-- ──── PAGE: System ──── -->
  <div class="page" id="pageSys">
    <div class="grid">

      <!-- System Info -->
      <div class="card span-full">
        <div class="card-title" id="iCardSysInfo">系统信息</div>
        <div class="mode-row" style="grid-template-columns:repeat(5,1fr)">
          <div class="ed-cell">
            <div class="ed-val" id="sysChipTemp">--</div>
            <div class="ed-lbl" id="iSysChipTemp">芯片温度</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="sysLatency">--</div>
            <div class="ed-lbl" id="iSysLatency">处理延迟</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="sysLatencyMax">--</div>
            <div class="ed-lbl" id="iSysLatencyMax">最大延迟</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="sysFreeHeap">--</div>
            <div class="ed-lbl" id="iSysFreeHeap">可用内存</div>
          </div>
          <div class="ed-cell">
            <div class="ed-val" id="sysFreePsram">--</div>
            <div class="ed-lbl" id="iSysFreePsram">PSRAM</div>
          </div>
        </div>
      </div>

      <!-- WiFi -->
      <div class="card">
        <div class="card-title" id="iCardWifi">WiFi 设置</div>
        <div class="field-stack">
          <label id="iLblSsid" style="font-size:14px">SSID 热点名称</label>
          <input class="txt" type="text" id="wifiSsid" maxlength="32" placeholder="TeslaCAN" style="font-size:16px;padding:13px 16px">
        </div>
        <div class="field-stack">
          <label id="iLblPass" style="font-size:14px">密码 (留空保持不变)</label>
          <input class="txt" type="password" id="wifiPass" maxlength="63" placeholder="≥ 8 位" style="font-size:16px;padding:13px 16px">
        </div>
        <button class="btn btn-primary" id="wifiBtn" onclick="saveWifi()" style="margin-top:12px;font-size:16px;padding:15px 16px">保存并重启</button>
        <div class="hint" id="iWifiHint">修改后设备会重启, 固定 IP 不变</div>
      </div>

      <!-- OTA -->
      <div class="card">
        <div class="card-title" id="iCardOta">固件升级 OTA</div>
        <div class="file-row">
          <label class="file-pick" for="otaFile" id="iLblPick" style="font-size:14px;padding:11px 18px">选择文件</label>
          <input type="file" id="otaFile" accept=".bin" style="display:none" onchange="onFile(this)">
          <span class="file-name" id="fileName" style="font-size:14px">未选择</span>
        </div>
        <button class="btn btn-danger" id="otaBtn" disabled onclick="doOta()" style="font-size:16px;padding:15px 16px">上传固件</button>
        <div class="progress" id="prog"><div class="progress-bar" id="progBar"></div></div>
        <div style="font-size:12px;text-align:center;margin-top:8px;min-height:14px;font-weight:600" id="otaMsg"></div>
      </div>

      <!-- Logging toggle + Live log -->
      <div class="card span-full">
        <div class="card-title" id="iCardDrive">行车数据记录</div>
        <div class="row" style="padding-top:0">
          <div class="row-info"><span class="row-name" id="iLblDriveRec">记录行车数据</span><span class="row-meta" id="iMetaDriveRec">记录 CAN 信号快照至内存</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tDriveRec" onchange="toggleDriveRec()"><span class="toggle-track"></span></label></div>
        </div>
        <div style="display:flex;gap:8px;padding:8px 16px 12px">
          <button class="btn btn-primary" style="flex:1;font-size:14px;padding:10px" id="driveDownBtn" onclick="window.location.href='/api/drive-data/csv'"><span id="iDriveDownload">下载 CSV</span></button>
          <button class="btn btn-danger" style="flex:1;font-size:14px;padding:10px" id="driveClearBtn" onclick="clearDriveData()"><span id="iDriveClear">清除</span></button>
        </div>
        <div style="text-align:center;font-size:12px;color:var(--text3);padding-bottom:8px" id="driveStatus">--</div>
      </div>

      <!-- Logging toggle + Live log (original) -->
      <div class="card span-full">
        <div class="card-title" id="iCardLog">实时日志</div>
        <div class="row" style="padding-top:0">
          <div class="row-info"><span class="row-name" id="iLblLog">启用日志</span><span class="row-meta" id="iMetaLog">串口与 web 日志</span></div>
          <div class="row-ctrl"><label class="toggle"><input type="checkbox" id="tLog" checked onchange="togLog(this)"><span class="toggle-track"></span></label></div>
        </div>
        <div class="log" id="logBox">
          <div class="log-line" id="logEmpty" style="color:var(--text3);font-style:italic"><span class="ts">--:--</span> Waiting...</div>
        </div>
      </div>

    </div>
  </div>

</div>

<!-- ═══ Tab bar ═══ -->
<nav class="tab-bar">
  <button class="tab active" data-page="pageDash" onclick="switchTab(this)">
    <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm-1-13h2v6h-2zm0 8h2v2h-2z"/><circle cx="12" cy="12" r="3" fill="none" stroke="currentColor" stroke-width="1.5"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3" stroke="currentColor" stroke-width="1.5" fill="none"/></svg>
    <span id="iTabDash">Dashboard</span>
  </button>
  <button class="tab" data-page="pageSet" onclick="switchTab(this)">
    <svg viewBox="0 0 24 24"><path d="M3 17v2h6v-2H3zM3 5v2h10V5H3zm10 16v-2h8v-2h-8v-2h-2v6h2zM7 9v2H3v2h4v2h2V9H7zm14 4v-2H11v2h10zm-6-4h2V7h4V5h-4V3h-2v6z"/></svg>
    <span id="iTabSet">Settings</span>
  </button>
  <button class="tab" data-page="pageSys" onclick="switchTab(this)">
    <svg viewBox="0 0 24 24"><path d="M20 18c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2H4c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2H0v2h24v-2h-4zM4 6h16v10H4V6z"/></svg>
    <span id="iTabSys">System</span>
  </button>
</nav>

<script>
/* ═══════════════════════════════════════════════════
   i18n
   ═══════════════════════════════════════════════════ */
var lang=localStorage.getItem('lang')||'zh';
var T={
zh:{
  tabDash:'仪表盘',tabSet:'设置',tabSys:'系统',
  navBtn:'联网',
  connOn:'在线',connOff:'离线',connErr:'连接断开,正在重试…',
  lblFsd:'FSD 状态',fsdOn:'ON',fsdOff:'OFF',
  lblMod:'已修改',lblRcv:'已接收',lblErr:'错误',lblUp:'运行时间',
  uptH:'时',uptM:'分',uptS:'秒',

  cardMode:'驾驶模式',lblSrc:'档位源',
  srcAuto:'自动 (拨杆)',srcMan:'手动',
  modeChill:'保守',modeNorm:'默认',modeHurry:'适中',modeMax:'激进',modeSloth:'树懒',

  cardSL:'限速助手',
  slOff:'关闭',slOffDesc:'关闭',slOffInfo:'限速助手已关闭，FSD 使用默认限速逻辑。',
  slManName:'手动',slManShort:'手动优先',slManInfo:'FSD 限速以手动滚轮设定为准。经过限速标牌时不自动切换，始终保持你手动设定的速度。',
  slSmartName:'智能',slSmartShort:'智能映射',slSmartInfo:'根据自定义规则自动调整检测到的限速值，让 FSD 按你的偏好行驶。',
  slAddRule:'添加规则',slEditRule:'编辑规则',slDelConfirm:'删除这条规则？',
  slRuleMin:'检测限速下限 (km/h)',slRuleMax:'检测限速上限 (km/h)',slRuleTarget:'实际设定速度 (km/h)',
  slRuleErr:'请输入有效数值，且下限必须小于上限',

  cardED:'实时信号',
  edAccel:'油门深度',edBrake:'制动力矩',edTorque:'电机扭矩',
  edHands:'方向盘握力',edSteer:'转向扭矩',edFollow:'跟车距离',

  cardSLimits:'限速与速度偏移',
  slFused:'融合',slMap:'地图',slVehicle:'车辆',slDasSet:'DAS设定',slAcc:'ACC限速',

  lightOff:'关闭',lightLow:'近光',lightFlash:'闪光',lightHigh:'远光',

  handsNone:'无',handsLight:'轻',handsMedium:'中',handsFirm:'紧',

  gearP:'P',gearR:'R',gearN:'N',gearD:'D',gearInv:'--',gearCreep:'C',

  cardHW:'硬件配置',
  lblHW:'硬件版本',metaHW:'切换需重启, 5 秒后自动重连',
  hwHint:'⚠ HW4 硬件 + 固件 2026.8.x 或更旧 (FSD V13) 请选 HW3',

  cardFeat:'功能开关',
  lblBypass:'绕过 TLSSC 要求',metaBypass:'无需开启交通灯停车标志控制',
  lblIsa:'ISA 限速提示音抑制',metaIsa:'仅 HW4, 关闭限速 chime',
  lblEvd:'紧急车辆检测',metaEvd:'FSD V14 警车/救护车识别',
  lblEap:'增强 Autopilot',metaEap:'EAP 功能',
  lblNag:'Autosteer Nag Killer',metaNag:'需要 X179 接线',
  lblCn:'中国模式',metaCn:'国行车机绕过 UI 选择检查',

  cardLab:'模块增强',
  lblPreheat:'预热模式',metaPreheat:'注入 UI_tripPlanning (0x082) 500ms 周期',
  labHint:'⚠ 测试区功能必须接到真实 CAN 总线。空闲启用会导致 TX 错误飙升。',

  cardWifi:'WiFi 设置',
  lblSsid:'SSID 热点名称',lblPass:'密码 (留空保持不变)',
  wifiSave:'保存并重启',wifiHint:'修改后设备会重启, 固定 IP 不变',
  wifiSsidErr:'SSID 不能为空',wifiPassErr:'密码至少 8 位',
  wifiOk:'已保存, 重启中…',wifiFail:'保存失败: ',

  cardOta:'固件升级 OTA',
  lblPick:'选择文件',noFile:'未选择',otaBtn:'上传固件',
  otaUploading:'上传中 ',otaOk:'升级完成, 设备重启中…',otaFail:'升级失败: ',otaConn:'OTA 连接失败',

  cardCan:'CAN 总线诊断',lblCanState:'总线状态',
  canRx:'RX',canTx:'TX 已修改',canErrRx:'RX 错误',canErrTx:'TX 错误',canBus:'总线错误',canMiss:'RX 丢帧',
  canRunning:'运行中',canStopped:'已停止',canBusOff:'总线关闭',canRecover:'恢复中',canNoDrv:'无驱动',canUnk:'未知',

  cardLog:'实时日志',lblLog:'启用日志',metaLog:'串口与 web 日志',

  confirmBypass:'确定绕过 TLSSC 要求？启用后无需在车机上开启"交通灯和停车标志控制"即可启用 FSD。',
  confirmPreheat:'启用预热将持续向 CAN 总线注入帧 (0x082, 500ms)，触发电池预热。建议出行前几分钟启用。继续？',
  confirmHwReboot:'切换硬件版本需要重启设备，继续？',
  otaConfirm:'确定刷入此固件并重启设备？',

  modalOk:'确认',modalCancel:'取消',modalErr:'操作失败',
  smartEditTitle:'编辑智能偏移规则',
  langBtn:'EN',

  bsCanState:'CAN',bsUp:'运行',bsErr:'错误',

  cardBat:'电池状态',batSoc:'电量',batVolt:'电压 (V)',batCurr:'电流 (A)',batTMin:'最低温度',batTMax:'最高温度',
  otaPause:'车辆 OTA 进行中，CAN 修改已暂停',
  phElapsed:'已运行',phTempMin:'电池最低温',phTempMax:'电池最高温',phSoc:'电量',phAutoStopLbl:'自动停止温度',phMaxDurLbl:'最长时间',
  cardDrive:'行车数据记录',lblDriveRec:'记录行车数据',metaDriveRec:'记录 CAN 信号快照至内存',driveDownload:'下载 CSV',driveClear:'清除',driveEmpty:'无数据',driveRows:'条记录',
  cardChart:'实时曲线',chartSpeed:'车速',chartSoc:'电量',chartPower:'功率',
  cardCanView:'CAN 帧查看器',
  cardSysInfo:'系统信息',sysChipTemp:'芯片温度',sysLatency:'处理延迟',sysLatencyMax:'最大延迟',sysFreeHeap:'可用内存',sysFreePsram:'PSRAM'
},
en:{
  tabDash:'Dashboard',tabSet:'Settings',tabSys:'System',
  navBtn:'NET',
  connOn:'Online',connOff:'Offline',connErr:'Connection lost, retrying…',
  lblFsd:'FSD STATE',fsdOn:'ON',fsdOff:'OFF',
  lblMod:'MODIFIED',lblRcv:'RECEIVED',lblErr:'ERRORS',lblUp:'UPTIME',
  uptH:'h',uptM:'m',uptS:'s',

  cardMode:'DRIVING MODE',lblSrc:'Source',
  srcAuto:'Auto (Stalk)',srcMan:'Manual',
  modeChill:'Chill',modeNorm:'Normal',modeHurry:'Hurry',modeMax:'Max',modeSloth:'Sloth',

  cardSL:'SPEED LIMIT ASSISTANT',
  slOff:'OFF',slOffDesc:'Disabled',slOffInfo:'Speed limit assistant is off. FSD uses default speed logic.',
  slManName:'Manual',slManShort:'Priority',slManInfo:'FSD respects your scroll-wheel speed. Passing a speed sign will NOT override your manual limit.',
  slSmartName:'Smart',slSmartShort:'Mapping',slSmartInfo:'Custom rules to auto-adjust detected speed limits so FSD drives at your preferred pace.',
  slAddRule:'Add Rule',slEditRule:'Edit Rule',slDelConfirm:'Delete this rule?',
  slRuleMin:'Detected speed min (km/h)',slRuleMax:'Detected speed max (km/h)',slRuleTarget:'Target speed (km/h)',
  slRuleErr:'Enter valid numbers and ensure min < max',

  cardED:'REAL-TIME SIGNALS',
  edAccel:'THROTTLE',edBrake:'BRAKE Nm',edTorque:'TORQUE Nm',
  edHands:'HANDS ON',edSteer:'STEER Nm',edFollow:'FOLLOW',

  cardSLimits:'SPEED LIMITS & OFFSET',
  slFused:'FUSED',slMap:'MAP',slVehicle:'VEHICLE',slDasSet:'DAS SET',slAcc:'ACC LIM',

  lightOff:'OFF',lightLow:'LOW',lightFlash:'FLASH',lightHigh:'HIGH',

  handsNone:'NONE',handsLight:'LIGHT',handsMedium:'MED',handsFirm:'FIRM',

  gearP:'P',gearR:'R',gearN:'N',gearD:'D',gearInv:'--',gearCreep:'C',

  cardHW:'HARDWARE',
  lblHW:'Hardware Version',metaHW:'Switch requires reboot, ~5s downtime',
  hwHint:'HW4 hardware + firmware 2026.8.x or older (FSD V13) → choose HW3',

  cardFeat:'FEATURES',
  lblBypass:'Bypass TLSSC',metaBypass:'No Traffic Light & Stop Sign Control toggle needed',
  lblIsa:'ISA Speed Chime Suppress',metaIsa:'HW4 only, mute speed limit chime',
  lblEvd:'Emergency Vehicle Detection',metaEvd:'FSD V14 emergency vehicle awareness',
  lblEap:'Enhanced Autopilot',metaEap:'EAP features',
  lblNag:'Autosteer Nag Killer',metaNag:'Requires X179 wiring',
  lblCn:'China Mode',metaCn:'Bypass UI selection check on China firmware',

  cardLab:'MODULE ENHANCEMENTS',
  lblPreheat:'Preheat',metaPreheat:'Inject UI_tripPlanning (0x082) every 500ms',
  labHint:'Beta features require a real CAN bus. Enabling on desk will spike TX errors.',

  cardWifi:'WIFI SETTINGS',
  lblSsid:'SSID',lblPass:'Password (blank = keep current)',
  wifiSave:'Save & Reboot',wifiHint:'Device will reboot after save, static IP unchanged',
  wifiSsidErr:'SSID required',wifiPassErr:'Password must be >= 8 chars',
  wifiOk:'Saved, rebooting…',wifiFail:'Save failed: ',

  cardOta:'FIRMWARE OTA',
  lblPick:'Choose File',noFile:'No file',otaBtn:'Upload Firmware',
  otaUploading:'Uploading ',otaOk:'Upload complete, rebooting…',otaFail:'Upload failed: ',otaConn:'OTA connection error',

  cardCan:'CAN BUS DIAGNOSTICS',lblCanState:'Bus State',
  canRx:'RX',canTx:'TX MOD',canErrRx:'RX ERR',canErrTx:'TX ERR',canBus:'BUS ERR',canMiss:'RX MISS',
  canRunning:'RUNNING',canStopped:'STOPPED',canBusOff:'BUS_OFF',canRecover:'RECOVERING',canNoDrv:'NO_DRIVER',canUnk:'UNKNOWN',

  cardLog:'LIVE LOG',lblLog:'Enable Logging',metaLog:'Serial & web log',

  confirmBypass:'Bypass TLSSC requirement? You will NOT need to enable "Traffic Light & Stop Sign Control" on the vehicle UI to activate FSD.',
  confirmPreheat:'Preheat will continuously inject frames (0x082, 500ms) to trigger battery preheat. Recommended only a few minutes before departure. Continue?',
  confirmHwReboot:'Switching hardware version requires a reboot. Continue?',
  otaConfirm:'Flash this firmware and reboot the device?',

  modalOk:'OK',modalCancel:'Cancel',modalErr:'Operation failed',
  smartEditTitle:'Edit Smart Offset Rules',
  langBtn:'中文',

  bsCanState:'CAN',bsUp:'UP',bsErr:'ERR',

  cardBat:'BATTERY STATUS',batSoc:'SOC',batVolt:'VOLTAGE (V)',batCurr:'CURRENT (A)',batTMin:'TEMP MIN',batTMax:'TEMP MAX',
  otaPause:'Vehicle OTA in progress — CAN mods paused',
  phElapsed:'ELAPSED',phTempMin:'BAT TEMP MIN',phTempMax:'BAT TEMP MAX',phSoc:'SOC',phAutoStopLbl:'Auto-stop temp',phMaxDurLbl:'Max duration',
  cardDrive:'DRIVE DATA RECORDING',lblDriveRec:'Record Drive Data',metaDriveRec:'Snapshot CAN signals to memory',driveDownload:'Download CSV',driveClear:'Clear',driveEmpty:'No data',driveRows:'rows',
  cardChart:'LIVE CHART',chartSpeed:'Speed',chartSoc:'SOC',chartPower:'Power',
  cardCanView:'CAN FRAME VIEWER',
  cardSysInfo:'SYSTEM INFO',sysChipTemp:'CHIP TEMP',sysLatency:'LATENCY',sysLatencyMax:'MAX LATENCY',sysFreeHeap:'FREE HEAP',sysFreePsram:'PSRAM'
}
};

/* ═══════════════════════════════════════════════════
   Utilities
   ═══════════════════════════════════════════════════ */
var _nullEl={};_nullEl.textContent='';_nullEl.innerHTML='';_nullEl.style={};_nullEl.classList={toggle:function(){},add:function(){},remove:function(){}};_nullEl.className='';_nullEl.value='';_nullEl.checked=false;_nullEl.dataset={};
function $(id){return document.getElementById(id)||_nullEl}
function t(k){return T[lang][k]||k}

function fmtUp(s){
  var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;
  if(h>0)return h+t('uptH')+m+t('uptM');
  if(m>0)return m+t('uptM')+sec+t('uptS');
  return sec+t('uptS');
}

/* ═══════════════════════════════════════════════════
   Tab navigation
   ═══════════════════════════════════════════════════ */
function switchTab(btn){
  document.querySelectorAll('.tab').forEach(function(b){b.classList.remove('active')});
  document.querySelectorAll('.page').forEach(function(p){p.classList.remove('active')});
  btn.classList.add('active');
  $(btn.dataset.page).classList.add('active');
}

/* ═══════════════════════════════════════════════════
   Modal (replaces alert / confirm)
   ═══════════════════════════════════════════════════ */
var _mResolve=null;
function showModal(title,body,hasCancel,isHtml){
  $('mTitle').textContent=title;
  if(isHtml){$('mBody').innerHTML=body;}
  else if(typeof body==='string'){$('mBody').textContent=body;}
  else{$('mBody').textContent='';$('mBody').appendChild(body);}
  $('mCancel').style.display=hasCancel?'':'none';
  $('mOk').textContent=t('modalOk');
  $('mCancel').textContent=t('modalCancel');
  $('mOk').className='modal-btn primary';
  $('modal').classList.add('show');
  return new Promise(function(r){_mResolve=r});
}
function mDismiss(v){$('modal').classList.remove('show');if(_mResolve){_mResolve(v);_mResolve=null}}

/* ═══════════════════════════════════════════════════
   Toast
   ═══════════════════════════════════════════════════ */
var _toastTimer=null;
function toast(msg,type){
  var el=$('toast');
  el.textContent=msg;
  el.className='toast show '+(type||'');
  clearTimeout(_toastTimer);
  _toastTimer=setTimeout(function(){el.classList.remove('show')},3000);
}

/* ═══════════════════════════════════════════════════
   Segment controls
   ═══════════════════════════════════════════════════ */
function segVal(groupId){
  var active=document.querySelector('#'+groupId+' .seg-btn.active');
  return active?active.dataset.val:null;
}
function segSet(groupId,val){
  document.querySelectorAll('#'+groupId+' .seg-btn').forEach(function(b){
    b.classList.toggle('active',b.dataset.val===String(val));
  });
}

/* Profile source segment */
document.querySelectorAll('#sgProfSrc .seg-btn').forEach(function(btn){
  btn.addEventListener('click',function(){
    var v=btn.dataset.val;
    segSet('sgProfSrc',v);
    var auto=v==='1';
    updateModeCardsState(auto);
    fetch('/api/profile-mode-auto',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:auto})});
  });
});

/* HW mode segment */
document.querySelectorAll('#sgHW .seg-btn').forEach(function(btn){
  btn.addEventListener('click',async function(){
    var prev=segVal('sgHW');
    var v=btn.dataset.val;
    if(v===prev)return;
    segSet('sgHW',v);
    var ok=await showModal(t('lblHW'),t('confirmHwReboot'),true);
    if(!ok){segSet('sgHW',prev);return;}
    try{
      var r=await fetch('/api/hw-mode',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:parseInt(v)})});
      if(!r.ok)throw new Error();
    }catch(e){segSet('sgHW',prev);toast(t('modalErr'),'err');}
  });
});

/* Speed profile mode cards */
document.querySelectorAll('#modeRow .mode-card').forEach(function(card){
  card.addEventListener('click',function(){
    if(card.classList.contains('disabled'))return;
    var v=card.dataset.val;
    document.querySelectorAll('#modeRow .mode-card').forEach(function(c){c.classList.remove('active')});
    card.classList.add('active');
    fetch('/api/speed-profile',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:parseInt(v)})});
  });
});

function updateModeCardsState(isAuto){
  document.querySelectorAll('#modeRow .mode-card').forEach(function(c){
    c.classList.toggle('disabled',isAuto);
  });
}

/* Speed offset safety warning — only show once per session */
var offsetWarningShown=false;
async function showOffsetWarning(){
  if(offsetWarningShown)return true;
  var ok=await showModal(
    '⚠ 安全警告',
    '<div style="text-align:left;line-height:1.8;font-size:20px">'
    +'<b style="color:var(--red)">速度偏移会使 FSD 以高于限速的速度行驶！</b><br><br>'
    +'启用后请务必：<br>'
    +'• 始终关注前方路况<br>'
    +'• 随时准备接管方向盘<br>'
    +'• 经过学校、居民区等区域时手动降速<br>'
    +'• 对因超速产生的后果自行负责<br><br>'
    +'<span style="color:var(--amber)">你确认已了解风险并愿意继续？</span>'
    +'</div>',
    true, true
  );
  if(ok)offsetWarningShown=true;
  return ok;
}

/* Speed offset: manual buttons — UI percentage × 4 = CAN value */
async function setOffset(pct){
  if(pct>0){
    var ok=await showOffsetWarning();
    if(!ok)return;
  }
  document.querySelectorAll('#offsetBtns .mode-card').forEach(function(b){
    b.classList.toggle('active',parseInt(b.dataset.off)===pct);
  });
  smartOffsetEnabled=false;
  fetch('/api/smart-offset',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:false})});
  fetch('/api/speed-offset',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:pct*4,manual:true})});
  setOffsetMode('manual');
}

var smartOffsetEnabled=false;
async function setOffsetMode(mode){
  var isManual=mode==='manual';
  if(!isManual){
    var ok=await showOffsetWarning();
    if(!ok)return;
  }
  document.querySelectorAll('#sgOffsetMode .seg-btn').forEach(function(b){
    b.classList.toggle('active',b.dataset.val===mode);
  });
  $('offsetManual').style.display=isManual?'':'none';
  $('offsetSmart').style.display=isManual?'none':'';
  if(!isManual){
    smartOffsetEnabled=true;
    fetch('/api/smart-offset',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:true})});
    loadSmartRules();
  }
}

function loadSmartRules(){
  fetch('/api/smart-offset').then(function(r){return r.json()}).then(function(d){
    smartOffsetEnabled=d.enabled;
    $('smartCurPct').textContent=d.current_pct;
    var html='';
    for(var i=0;i<d.rules.length;i++){
      var r=d.rules[i];
      var prev=i>0?d.rules[i-1].maxSpeed:0;
      var range=r.maxSpeed>=999?(prev+'+'):(prev+'~'+r.maxSpeed);
      html+='<div class="mode-card" style="pointer-events:none"><div class="mode-name" style="font-size:18px;color:var(--blue)">+'+r.offsetPct+'</div><div style="font-size:11px;color:var(--text3);margin-top:4px">'+range+'</div></div>';
    }
    $('smartRulesText').innerHTML=html;
  });
}

async function editSmartRules(){
  var r=await fetch('/api/smart-offset');
  var d=await r.json();
  var rows='';
  for(var i=0;i<d.rules.length;i++){
    rows+=smartRuleRow(i,d.rules[i].maxSpeed,d.rules[i].offsetPct);
  }
  var html='<div class="modal-form">'
    +'<div style="display:flex;gap:6px;font-size:12px;color:var(--text3);font-weight:600">'
    +'<span style="flex:1">限速上限 (km/h)</span><span style="flex:1">偏移 %</span><span style="width:32px"></span></div>'
    +'<div id="srRows">'+rows+'</div>'
    +'<button class="pill-btn" style="align-self:flex-start;margin-top:4px" onclick="addSmartRow()">+ 添加规则</button>'
    +'<div style="font-size:12px;color:var(--text3)">限速&lt;上限值时使用对应偏移百分比，最后一条建议设为999作为兜底</div>'
    +'</div>';
  var ok=await showModal(t('smartEditTitle')||'编辑智能偏移规则',html,true,true);
  if(!ok)return;
  var rules=parseSmartRows();
  if(rules.length>0){
    await fetch('/api/smart-offset',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({rules:rules,enabled:true})});
    loadSmartRules();
    toast('规则已更新');
  }
}
function smartRuleRow(i,ms,op){
  return '<div class="sr-row" data-idx="'+i+'">'
    +'<input type="number" class="txt sr-spd" value="'+ms+'" min="1" max="999" placeholder="限速">'
    +'<input type="number" class="txt sr-pct" value="'+op+'" min="0" max="50" placeholder="%">'
    +'<button class="pill-btn sr-del" onclick="delSmartRow(this)" title="删除">✕</button>'
    +'</div>';
}
function addSmartRow(){
  var cont=$('srRows');
  var n=cont.querySelectorAll('.sr-row').length;
  if(n>=8){toast('最多8条规则','err');return;}
  cont.insertAdjacentHTML('beforeend',smartRuleRow(n,999,5));
}
function delSmartRow(btn){
  var cont=$('srRows');
  if(cont.querySelectorAll('.sr-row').length<=1){toast('至少保留1条规则','err');return;}
  btn.closest('.sr-row').remove();
}
function parseSmartRows(){
  var rules=[];
  document.querySelectorAll('#srRows .sr-row').forEach(function(row){
    var ms=parseInt(row.querySelector('.sr-spd').value);
    var op=parseInt(row.querySelector('.sr-pct').value);
    if(!isNaN(ms)&&!isNaN(op)){
      if(ms<1)ms=1;if(ms>999)ms=999;
      if(op<0)op=0;if(op>50)op=50;
      rules.push({maxSpeed:ms,offsetPct:op});
    }
  });
  rules.sort(function(a,b){return a.maxSpeed-b.maxSpeed});
  return rules;
}

/* ═══════════════════════════════════════════════════
   CAN Live polling (500ms)
   ═══════════════════════════════════════════════════ */
var GEAR_MAP=['--','P','R','N','D','C'];

function getGearClass(g){
  if(g===4||g===5)return 'gear-d';
  if(g===2)return 'gear-r';
  if(g===3)return 'gear-n';
  return '';
}

function getLightText(v){
  if(v===0x02)return t('lightLow');
  if(v===0x03)return t('lightFlash');
  if(v===0x06||v===0x07)return t('lightHigh');
  return t('lightOff');
}

function getHandsText(v){
  if(v===1)return t('handsLight');
  if(v===2)return t('handsMedium');
  if(v===3)return t('handsFirm');
  return t('handsNone');
}

function updateDashboard(sig){
  if(!sig)return;

  /* Throttle */
  var accel=Math.round(sig.di_accelPedal_pct||0);
  $('edAccel').textContent=accel+'%';
  $('barAccel').style.width=Math.min(accel,100)+'%';

  /* Brake/Torque removed — encoding unverified on X179 */

  /* Hands on level */
  var hands=sig.handsOnLevel||0;
  $('edHands').textContent=getHandsText(hands);
  if(!window._handsDots)window._handsDots=[].slice.call(document.querySelectorAll('#handsDots .hands-dot'));
  window._handsDots.forEach(function(dot){
    var lv=parseInt(dot.dataset.lv);
    var isActive=lv<=hands;
    dot.className='hands-dot'+(isActive?' active':'')+(isActive&&hands>=2?' lv'+hands:'');
  });

  /* Steering torque */
  var steer=sig.torsionBarTorque_Nm||0;
  $('edSteer').textContent=steer.toFixed(1);

  /* Follow distance */
  var follow=sig.followDistance||0;
  $('edFollow').textContent=follow>0?follow:'--';
  if(!window._followBars)window._followBars=[].slice.call(document.querySelectorAll('#followBars .follow-bar'));
  window._followBars.forEach(function(bar){
    var lv=parseInt(bar.dataset.lv);
    bar.classList.toggle('active',lv<=follow);
  });

  /* Speed limits */
  $('slFused').textContent=sig.fusedSpeedLimit_kph?Math.round(sig.fusedSpeedLimit_kph):'--';
  $('slMap').textContent=sig.mapSpeedLimit_kph?Math.round(sig.mapSpeedLimit_kph):'--';
  $('slVehicle').textContent=sig.vehicleSpeedLimit_kph?Math.round(sig.vehicleSpeedLimit_kph):'--';
  if($('slDasSet'))$('slDasSet').textContent=sig.dasSetSpeed_kph?Math.round(sig.dasSetSpeed_kph):'--';
  var accLim=sig.accSpeedLimit_mph||0;
  if($('slAcc'))$('slAcc').textContent=(accLim>0&&accLim<200)?Math.round(accLim):'--';

  /* FSD status from /api/status (handler state, not raw CAN frame) */
  /* updated in pollStatus() instead */

  /* Auto theme: follow headlights */
  if(theme==='auto'&&sig.lightState!==undefined){
    var newVal=(sig.lightState>0)?'dark':'light';
    if(newVal!==window._autoThemeVal){window._autoThemeVal=newVal;applyTheme();}
  }

  /* Battery (BMS) */
  var soc=sig.bmsSoc_pct;
  if(soc!==undefined&&soc>0){
    $('batSoc').textContent=soc.toFixed(1)+'%';
    $('barSoc').style.width=Math.min(soc,100)+'%';
    $('barSoc').className='ed-bar '+(soc<20?'red':soc<50?'yellow':'green');
  }
  if(sig.bmsVoltage_V!==undefined&&sig.bmsVoltage_V>0)$('batVolt').textContent=sig.bmsVoltage_V.toFixed(1);
  if(sig.bmsCurrent_A!==undefined)$('batCurr').textContent=sig.bmsCurrent_A.toFixed(1);
  if(sig.bmsTempMin_C!==undefined&&sig.bmsTempMin_C>-40)$('batTMin').textContent=sig.bmsTempMin_C+'\u00b0C';
  if(sig.bmsTempMax_C!==undefined&&sig.bmsTempMax_C>-40)$('batTMax').textContent=sig.bmsTempMax_C+'\u00b0C';

  /* OTA protection banner */
  var otaBanner=$('otaBanner');
  if(otaBanner){
    otaBanner.style.display=(sig.otaInProgress&&sig.otaInProgress>0)?'block':'none';
  }

  /* Chart: push speed / SOC / power (kW from V*A/1000) */
  var chartSpeedVal=sig.vehicleSpeed_kph||0;
  var chartSocVal=sig.bmsSoc_pct||0;
  var chartPowerVal=0;
  if(sig.bmsVoltage_V&&sig.bmsCurrent_A)chartPowerVal=Math.abs(sig.bmsVoltage_V*sig.bmsCurrent_A/1000);
  pushChart(chartSpeedVal,chartSocVal,chartPowerVal);
}

var canLiveErrCount=0;
async function pollCanLive(){
  try{
    var r=await fetch('/api/can-live');
    if(!r.ok)throw new Error();
    var d=await r.json();
    canLiveErrCount=0;
    if(d.signals)updateDashboard(d.signals);
    if(d.frames)updateCanTable(d.frames);
  }catch(e){
    canLiveErrCount++;
  }
}

/* ═══════════════════════════════════════════════════
   Preheat Status Polling
   ═══════════════════════════════════════════════════ */
var _phTimer=null;
function fmtElapsed(s){
  var m=Math.floor(s/60),sec=s%60;
  return m>0?(m+t('uptM')+sec+t('uptS')):(sec+t('uptS'));
}
async function pollPreheatStatus(){
  try{
    var r=await fetch('/api/preheat/status');
    var d=await r.json();
    var panel=$('preheatPanel');
    if(d.active){
      panel.style.display='block';
      $('phElapsed').textContent=fmtElapsed(d.elapsed_s);
      $('phTempMin').textContent=d.bat_temp_min>-40?(d.bat_temp_min+'\u00b0C'):'--';
      $('phTempMax').textContent=d.bat_temp_max>-40?(d.bat_temp_max+'\u00b0C'):'--';
      $('phSoc').textContent=d.bat_soc>0?d.bat_soc.toFixed(1)+'%':'--';
      $('phAutoStopTemp').value=String(d.auto_stop_temp);
      $('phMaxDur').value=String(d.max_duration_min);
    }else{
      panel.style.display='none';
      // If preheat was auto-stopped, uncheck the toggle
      $('tPreheat').checked=false;
    }
  }catch(e){}
}
function startPreheatPoll(){
  if(!_phTimer)_phTimer=setInterval(pollPreheatStatus,2000);
  pollPreheatStatus();
}
function stopPreheatPoll(){
  if(_phTimer){clearInterval(_phTimer);_phTimer=null;}
  $('preheatPanel').style.display='none';
}
async function savePreheatConfig(){
  try{
    await fetch('/api/preheat/config',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({auto_stop_temp:parseInt($('phAutoStopTemp').value),max_duration_min:parseInt($('phMaxDur').value)})});
  }catch(e){}
}

/* ═══════════════════════════════════════════════════
   Drive Data Recording
   ═══════════════════════════════════════════════════ */
async function toggleDriveRec(){
  try{
    var r=await fetch('/api/drive-data/toggle',{method:'POST'});
    var d=await r.json();
    $('tDriveRec').checked=d.recording;
  }catch(e){$('tDriveRec').checked=false;}
  pollDriveStatus();
}
async function clearDriveData(){
  try{await fetch('/api/drive-data/clear',{method:'POST'});}catch(e){}
  pollDriveStatus();
}
async function pollDriveStatus(){
  try{
    var r=await fetch('/api/drive-data/status');
    var d=await r.json();
    $('tDriveRec').checked=d.recording;
    $('driveStatus').textContent=d.rows>0?(d.rows+' '+t('driveRows')+' / '+d.max+' max'):t('driveEmpty');
  }catch(e){}
}

/* ═══════════════════════════════════════════════════
   Status polling (2s)
   ═══════════════════════════════════════════════════ */
var logSince=0,errCount=0,connected=false,ssidLoaded=false;

function setConn(ok){
  connected=ok;
  $('connDot').classList.toggle('off',!ok);
  $('iConn').textContent=ok?t('connOn'):t('connOff');
}

function setFeatToggle(id,feat){
  var el=$(id);if(!el)return;
  if(!feat){
    if(el.checked)el.checked=false;
    if(!el.disabled)el.disabled=true;
    return;
  }
  var en=!!feat.enabled,sup=!!feat.supported;
  if(el.checked!==en)el.checked=en;
  if(el.disabled===sup)el.disabled=!sup;
}

function getCanStateText(state){
  if(state==='RUNNING'||state===0)return t('canRunning');
  if(state==='STOPPED'||state===1)return t('canStopped');
  if(state==='BUS_OFF'||state===2)return t('canBusOff');
  if(state==='RECOVERING'||state===3)return t('canRecover');
  if(state==='NO_DRIVER')return t('canNoDrv');
  return t('canUnk');
}

async function poll(){
  try{
    var r=await fetch('/api/status?log_since='+logSince);
    if(!r.ok)throw new Error();
    var d=await r.json(),f=d.features||{};

    setConn(true);$('connErr').classList.remove('show');errCount=0;

    /* HW label + FSD status (from handler, not raw CAN) */
    $('hwLabel').textContent=['LEGACY','HW3','HW4'][d.hw_mode]||'--';
    var fsdOn=!!d.fsd_enabled;
    $('fsdBadge').classList.toggle('on',fsdOn);
    $('fsdVal').textContent=fsdOn?t('fsdOn'):t('fsdOff');

    /* Speed offset: show on HW3 and HW4 */
    var hasOffset=d.hw_mode===1||d.hw_mode===2;
    $('offsetCard').style.display=hasOffset?'block':'none';
    if(hasOffset){
      var isSmart=!!d.smart_offset;
      if(!window._offModeLoaded){
        window._offModeLoaded=true;
        setOffsetMode(isSmart?'smart':'manual');
      }
      if(isSmart){
        $('smartCurPct').textContent=Math.round((d.speed_offset||0)/4);
      }else{
        var soPct=Math.round((d.speed_offset||0)/4);
        if(!window._offBtns)window._offBtns=[].slice.call(document.querySelectorAll('#offsetBtns .mode-card'));
        window._offBtns.forEach(function(b){
          b.classList.toggle('active',parseInt(b.dataset.off)===soPct);
        });
      }
    }

    /* Bottom stats */
    $('bsTx').textContent=(d.can&&d.can.frames_sent)||0;
    $('bsRx').textContent=(d.can&&d.can.frames_received)||0;
    var tErr=d.can?(d.can.rx_errors+d.can.tx_errors+d.can.bus_errors+d.can.rx_missed):0;
    $('bsErr').textContent=tErr;
    $('bsUp').textContent=fmtUp(d.uptime_s||0);
    if(d.can&&typeof d.can.state!=='undefined'){
      $('bsCanState').textContent=getCanStateText(d.can.state);
    }

    /* System stats */
    if(d.chip_temp_c!==undefined)$('sysChipTemp').textContent=d.chip_temp_c.toFixed(1)+'\u00b0C';
    if(d.handler){
      $('sysLatency').textContent=(d.handler.latency_us/1000).toFixed(1)+'ms';
      $('sysLatencyMax').textContent=(d.handler.latency_max_us/1000).toFixed(1)+'ms';
    }
    if(d.free_heap!==undefined)$('sysFreeHeap').textContent=Math.round(d.free_heap/1024)+'KB';
    if(d.free_psram!==undefined)$('sysFreePsram').textContent=Math.round(d.free_psram/1024)+'KB';

    /* Header */
    if(d.version)$('iVer').textContent='v'+d.version;
    if(d.sta_ip)$('iIp').textContent=d.sta_ip;

    /* HW segment */
    segSet('sgHW',d.hw_mode);

    /* Profile source + mode cards — only sync on first load */
    if(!window._profSrcLoaded){
      window._profSrcLoaded=true;
      var isAuto=!!d.profile_mode_auto;
      segSet('sgProfSrc',isAuto?'1':'0');
      updateModeCardsState(isAuto);
    }
    if(!window._modeCards)window._modeCards=[].slice.call(document.querySelectorAll('#modeRow .mode-card'));
    window._modeCards.forEach(function(c){
      c.classList.toggle('active',c.dataset.val===String(d.speed_profile));
    });

    /* Feature toggles — only update when changed */
    setFeatToggle('tFsd',f.bypass_tlssc_requirement);
    setFeatToggle('tIsa',f.isa_speed_chime_suppress);
    setFeatToggle('tEvd',f.emergency_vehicle_detection);
    setFeatToggle('tEap',f.enhanced_autopilot);
    setFeatToggle('tNag',f.nag_killer);
    setFeatToggle('tCn',f.china_mode);
    setFeatToggle('tPreheat',f.preheat);
    if(f.preheat&&f.preheat.enabled)startPreheatPoll();
    else stopPreheatPoll();
    var logChecked=!!d.enable_print;
    if($('tLog').checked!==logChecked)$('tLog').checked=logChecked;

    /* WiFi */
    if(d.sta_ssid&&!ssidLoaded){$('wifiSsid').value=d.sta_ssid;ssidLoaded=true;}

    /* Log — batch insert via DocumentFragment */
    if(d.logs&&d.logs.length){
      var box=$('logBox'),empty=$('logEmpty');
      if(empty)empty.remove();
      var frag=document.createDocumentFragment();
      for(var i=d.logs.length-1;i>=0;i--){
        var line=document.createElement('div');line.className='log-line';
        var ts=document.createElement('span');ts.className='ts';
        ts.textContent=fmtUp(Math.floor(d.logs[i].ts/1000));
        line.appendChild(ts);line.appendChild(document.createTextNode(d.logs[i].msg));
        frag.appendChild(line);
      }
      box.insertBefore(frag,box.firstChild);
      while(box.children.length>200)box.removeChild(box.lastChild);
    }
    logSince=d.log_head;
  }catch(e){
    errCount++;
    if(errCount>3){setConn(false);$('connErr').classList.add('show');}
  }
}

/* ═══════════════════════════════════════════════════
   Feature toggle handler
   ═══════════════════════════════════════════════════ */
async function togFeat(path,el,confirmKey){
  if(el.disabled)return;
  if(confirmKey&&el.checked){
    var ok=await showModal(t('modalOk'),t(confirmKey),true);
    if(!ok){el.checked=false;return;}
  }
  try{
    var r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});
    if(!r.ok){
      var txt='';
      try{var j=await r.json();txt=j.err||j.error||r.statusText;}catch(_){txt=r.statusText;}
      el.checked=!el.checked;
      toast(t('modalErr')+': '+txt,'err');
    }
  }catch(e){el.checked=!el.checked;}
}

async function togLog(el){
  try{var r=await fetch('/api/enable-print',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});if(!r.ok)throw 0;}catch(e){el.checked=!el.checked;}
}

/* ═══════════════════════════════════════════════════
   WiFi
   ═══════════════════════════════════════════════════ */
async function saveWifi(){
  var ssid=$('wifiSsid').value.trim(),pass=$('wifiPass').value;
  if(!ssid){toast(t('wifiSsidErr'),'err');return;}
  if(pass.length>0&&pass.length<8){toast(t('wifiPassErr'),'err');return;}
  $('wifiBtn').disabled=true;
  var body={ssid:ssid};if(pass.length>=8)body.pass=pass;
  try{
    var r=await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(!r.ok){var txt=await r.text();toast(t('wifiFail')+txt,'err');$('wifiBtn').disabled=false;return;}
    toast(t('wifiOk'),'ok');
  }catch(e){toast(t('wifiFail')+'connection','err');$('wifiBtn').disabled=false;}
}

/* ═══════════════════════════════════════════════════
   OTA
   ═══════════════════════════════════════════════════ */
function onFile(inp){
  var fn=$('fileName');
  if(inp.files[0]){fn.textContent=inp.files[0].name;$('otaBtn').disabled=false;otaFileMd5='';
    computeFileMd5(inp.files[0]).then(function(h){otaFileMd5=h;});
  }
  else{fn.textContent=t('noFile');$('otaBtn').disabled=true;otaFileMd5='';}
}

async function doOta(){
  var file=$('otaFile').files[0];if(!file)return;
  var ok=await showModal(t('cardOta'),t('otaConfirm'),true);
  if(!ok)return;
  var btn=$('otaBtn'),msg=$('otaMsg'),prog=$('prog'),bar=$('progBar');
  btn.disabled=true;msg.textContent=t('otaUploading')+'0%';msg.style.color='var(--text2)';
  prog.classList.add('active');bar.style.width='0%';
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/api/ota');
  xhr.setRequestHeader('Content-Type','application/octet-stream');
  if(otaFileMd5)xhr.setRequestHeader('X-Firmware-MD5',otaFileMd5);
  xhr.upload.onprogress=function(ev){
    if(ev.lengthComputable){var p=Math.round(ev.loaded/ev.total*100);bar.style.width=p+'%';msg.textContent=t('otaUploading')+p+'%';}
  };
  xhr.onload=function(){
    if(xhr.status>=200&&xhr.status<300){msg.textContent=t('otaOk');msg.style.color='var(--green)';}
    else{msg.textContent=t('otaFail')+(xhr.responseText||xhr.statusText);msg.style.color='var(--red)';btn.disabled=false;}
  };
  xhr.onerror=function(){msg.textContent=t('otaConn');msg.style.color='var(--red)';btn.disabled=false;};
  xhr.send(file);
}

/* ═══════════════════════════════════════════════════
   Speed Limit Assistant
   ═══════════════════════════════════════════════════ */
var slMode=localStorage.getItem('slMode')||'off';
var slRules=JSON.parse(localStorage.getItem('slRules')||'[{"min":30,"max":50,"target":60},{"min":50,"max":60,"target":80}]');

function setSLMode(mode){
  slMode=mode;localStorage.setItem('slMode',mode);
  document.querySelectorAll('#slModes .sl-mode').forEach(function(m){m.classList.toggle('active',m.dataset.sl===mode)});
  document.querySelectorAll('.sl-panel').forEach(function(p){p.classList.remove('active')});
  if(mode==='off')$('slPanelOff').classList.add('active');
  else if(mode==='manual')$('slPanelManual').classList.add('active');
  else if(mode==='smart'){$('slPanelSmart').classList.add('active');renderRules();}
}

document.querySelectorAll('#slModes .sl-mode').forEach(function(m){
  m.addEventListener('click',function(){setSLMode(m.dataset.sl)});
});

function saveRules(){localStorage.setItem('slRules',JSON.stringify(slRules))}

function renderRules(){
  var list=$('ruleList');list.innerHTML='';
  slRules.forEach(function(r,i){
    var el=document.createElement('div');el.className='rule-item';
    el.innerHTML='<span class="rule-range">'+r.min+' – '+r.max+' km/h</span>'
      +'<span class="rule-arrow">\u2192</span>'
      +'<span class="rule-target">'+r.target+' km/h</span>'
      +'<button class="rule-del" data-idx="'+i+'" title="Delete">\u00D7</button>';
    el.querySelector('.rule-range').addEventListener('click',function(){editRule(i)});
    el.querySelector('.rule-target').addEventListener('click',function(){editRule(i)});
    el.querySelector('.rule-del').addEventListener('click',function(e){e.stopPropagation();delRule(i)});
    list.appendChild(el);
  });
}

async function addRule(){
  var html='<div class="modal-form">'
    +'<div><label>'+t('slRuleMin')+'</label><input class="txt" type="number" id="rfMin" min="0" max="200" value="60"></div>'
    +'<div><label>'+t('slRuleMax')+'</label><input class="txt" type="number" id="rfMax" min="0" max="200" value="80"></div>'
    +'<div><label>'+t('slRuleTarget')+'</label><input class="txt" type="number" id="rfTarget" min="0" max="200" value="100"></div>'
    +'</div>';
  var ok=await showModal(t('slAddRule'),html,true,true);
  if(!ok)return;
  var mn=parseInt($('rfMin').value),mx=parseInt($('rfMax').value),tg=parseInt($('rfTarget').value);
  if(isNaN(mn)||isNaN(mx)||isNaN(tg)||mn>=mx){toast(t('slRuleErr'),'err');return;}
  slRules.push({min:mn,max:mx,target:tg});saveRules();renderRules();
}

async function editRule(idx){
  var r=slRules[idx];
  var html='<div class="modal-form">'
    +'<div><label>'+t('slRuleMin')+'</label><input class="txt" type="number" id="rfMin" min="0" max="200" value="'+r.min+'"></div>'
    +'<div><label>'+t('slRuleMax')+'</label><input class="txt" type="number" id="rfMax" min="0" max="200" value="'+r.max+'"></div>'
    +'<div><label>'+t('slRuleTarget')+'</label><input class="txt" type="number" id="rfTarget" min="0" max="200" value="'+r.target+'"></div>'
    +'</div>';
  var ok=await showModal(t('slEditRule'),html,true,true);
  if(!ok)return;
  var mn=parseInt($('rfMin').value),mx=parseInt($('rfMax').value),tg=parseInt($('rfTarget').value);
  if(isNaN(mn)||isNaN(mx)||isNaN(tg)||mn>=mx){toast(t('slRuleErr'),'err');return;}
  slRules[idx]={min:mn,max:mx,target:tg};saveRules();renderRules();
}

async function delRule(idx){
  var ok=await showModal(t('slEditRule'),t('slDelConfirm'),true);
  if(!ok)return;
  slRules.splice(idx,1);saveRules();renderRules();
}

/* ═══════════════════════════════════════════════════
   i18n apply
   ═══════════════════════════════════════════════════ */
function applyLang(){
  document.documentElement.lang=lang;
  $('iLangBtn').textContent=t('langBtn');
  if($('iNavText'))$('iNavText').textContent=t('navBtn');
  $('iConn').textContent=connected?t('connOn'):t('connOff');
  $('errText').textContent=t('connErr');

  $('iTabDash').textContent=t('tabDash');
  $('iTabSet').textContent=t('tabSet');
  $('iTabSys').textContent=t('tabSys');

  $('iLblFsd').textContent=t('lblFsd');

  $('iCardED').textContent=t('cardED');
  $('iEdAccel').textContent=t('edAccel');
  if($('iEdBrake'))$('iEdBrake').textContent=t('edBrake');
  if($('iEdTorque'))$('iEdTorque').textContent=t('edTorque');
  $('iEdHands').textContent=t('edHands');
  $('iEdSteer').textContent=t('edSteer');
  $('iEdFollow').textContent=t('edFollow');

  $('iCardBat').textContent=t('cardBat');
  $('iBatSoc').textContent=t('batSoc');
  $('iBatVolt').textContent=t('batVolt');
  $('iBatCurr').textContent=t('batCurr');
  $('iBatTMin').textContent=t('batTMin');
  $('iBatTMax').textContent=t('batTMax');
  $('iOtaPause').textContent=t('otaPause');

  $('iCardSLimits').textContent=t('cardSLimits');
  $('iSlFused').textContent=t('slFused');
  $('iSlMap').textContent=t('slMap');
  $('iSlVehicle').textContent=t('slVehicle');
  if($('iSlDasSet'))$('iSlDasSet').textContent=t('slDasSet');
  if($('iSlAcc'))$('iSlAcc').textContent=t('slAcc');

  $('iCardMode').textContent=t('cardMode');$('iLblSrc').textContent=t('lblSrc');
  $('sgSrcAuto').textContent=t('srcAuto');$('sgSrcMan').textContent=t('srcMan');
  $('iModeChill').textContent=t('modeChill');$('iModeNorm').textContent=t('modeNorm');
  $('iModeHurry').textContent=t('modeHurry');$('iModeMax').textContent=t('modeMax');
  $('iModeSloth').textContent=t('modeSloth');

  $('iCardSL').textContent=t('cardSL');
  $('iSLOff').textContent=t('slOff');$('iSLOffInfo').textContent=t('slOffInfo');
  $('iSLManName').textContent=t('slManName');$('iSLManInfo').textContent=t('slManInfo');
  $('iSLSmartName').textContent=t('slSmartName');$('iSLSmartInfo').textContent=t('slSmartInfo');
  $('iSLAddRule').textContent=t('slAddRule');

  $('iCardHW').textContent=t('cardHW');
  $('iLblHW').textContent=t('lblHW');$('iMetaHW').textContent=t('metaHW');
  $('iHwHint').textContent=t('hwHint');

  $('iCardFeat').textContent=t('cardFeat');
  $('iLblBypass').textContent=t('lblBypass');$('iMetaBypass').textContent=t('metaBypass');
  $('iLblIsa').textContent=t('lblIsa');$('iMetaIsa').textContent=t('metaIsa');
  $('iLblEvd').textContent=t('lblEvd');$('iMetaEvd').textContent=t('metaEvd');
  $('iLblEap').textContent=t('lblEap');$('iMetaEap').textContent=t('metaEap');
  $('iLblNag').textContent=t('lblNag');$('iMetaNag').textContent=t('metaNag');
  $('iLblCn').textContent=t('lblCn');$('iMetaCn').textContent=t('metaCn');

  $('iCardLab').textContent=t('cardLab');
  $('iLblPreheat').textContent=t('lblPreheat');$('iMetaPreheat').textContent=t('metaPreheat');
  $('iLabHint').textContent=t('labHint');
  $('iPhElapsed').textContent=t('phElapsed');$('iPhTempMin').textContent=t('phTempMin');
  $('iPhTempMax').textContent=t('phTempMax');$('iPhSoc').textContent=t('phSoc');
  $('iPhAutoStopLbl').textContent=t('phAutoStopLbl');$('iPhMaxDurLbl').textContent=t('phMaxDurLbl');

  $('iCardWifi').textContent=t('cardWifi');
  $('iLblSsid').textContent=t('lblSsid');$('iLblPass').textContent=t('lblPass');
  $('wifiBtn').textContent=t('wifiSave');$('iWifiHint').textContent=t('wifiHint');

  $('iCardOta').textContent=t('cardOta');
  $('iLblPick').textContent=t('lblPick');$('otaBtn').textContent=t('otaBtn');

  $('iCardLog').textContent=t('cardLog');
  $('iLblLog').textContent=t('lblLog');$('iMetaLog').textContent=t('metaLog');

  $('iCardDrive').textContent=t('cardDrive');
  $('iLblDriveRec').textContent=t('lblDriveRec');$('iMetaDriveRec').textContent=t('metaDriveRec');
  $('iDriveDownload').textContent=t('driveDownload');$('iDriveClear').textContent=t('driveClear');

  $('iCardChart').textContent=t('cardChart');
  $('iChartSpeed').textContent=t('chartSpeed');$('iChartSoc').textContent=t('chartSoc');$('iChartPower').textContent=t('chartPower');
  $('iCardCanView').textContent=t('cardCanView');
  $('iCardSysInfo').textContent=t('cardSysInfo');
  $('iSysChipTemp').textContent=t('sysChipTemp');$('iSysLatency').textContent=t('sysLatency');
  $('iSysLatencyMax').textContent=t('sysLatencyMax');$('iSysFreeHeap').textContent=t('sysFreeHeap');
  $('iSysFreePsram').textContent=t('sysFreePsram');

  if($('iBsCanState'))$('iBsCanState').textContent=t('bsCanState');
  if($('iBsUp'))$('iBsUp').textContent=t('bsUp');
  if($('iBsErr'))$('iBsErr').textContent=t('bsErr');

  if(!$('otaFile').files.length)$('fileName').textContent=t('noFile');
}

function toggleLang(){
  lang=lang==='zh'?'en':'zh';
  localStorage.setItem('lang',lang);
  applyLang();
}

/* ═══════════════════════════════════════════════════
   Theme (auto-follow vehicle headlights)
   ═══════════════════════════════════════════════════ */
var theme='auto';
function applyTheme(){
  var actual=window._autoThemeVal||'dark';
  var isLight=actual==='light';
  document.documentElement.classList.toggle('light',isLight);
  document.body.classList.toggle('light',isLight);
  var mc=document.querySelector('meta[name="theme-color"]');
  if(mc)mc.setAttribute('content',isLight?'#f2f2f7':'#000000');
}

/* ═══════════════════════════════════════════════════
   Real-time Chart (Canvas 2D)
   ═══════════════════════════════════════════════════ */
var chartLen=120,chartSpd=[],chartSoc=[],chartPwr=[],_chartDirty=false,_chartRaf=false;
function pushChart(spd,soc,pwr){
  chartSpd.push(spd);chartSoc.push(soc);chartPwr.push(pwr);
  if(chartSpd.length>chartLen){chartSpd.shift();chartSoc.shift();chartPwr.shift();}
  _chartDirty=true;
  if(!_chartRaf){_chartRaf=true;requestAnimationFrame(drawChart);}
}
function drawChart(){
  _chartRaf=false;
  if(!_chartDirty)return;
  _chartDirty=false;
  var cv=$('chartCanvas');if(!cv||!cv.getContext)return;
  // Skip if Dashboard tab not active
  var pg=$('pageDash');if(pg&&!pg.classList.contains('active'))return;
  var dpr=window.devicePixelRatio||1;
  var w=cv.clientWidth,h=cv.clientHeight;
  if(cv.width!==w*dpr||cv.height!==h*dpr){cv.width=w*dpr;cv.height=h*dpr;}
  var ctx=cv.getContext('2d');
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.clearRect(0,0,w,h);
  if(chartSpd.length<2)return;
  var n=chartSpd.length,sx=w/(chartLen-1);
  function drawLine(arr,maxV,color){
    ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=1.5;ctx.lineJoin='round';
    for(var i=0;i<n;i++){
      var x=(chartLen-n+i)*sx;
      var y=h-Math.min(arr[i]/maxV,1)*h*0.9-h*0.05;
      if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
    }
    ctx.stroke();
  }
  drawLine(chartSpd,200,'#3478f6');
  drawLine(chartSoc,100,'#30d158');
  drawLine(chartPwr,100,'#ff9500');
}

/* ═══════════════════════════════════════════════════
   CAN Frame Viewer (throttled to 2fps max)
   ═══════════════════════════════════════════════════ */
var canFrames={},_canDirty=false,_canRafPending=false;
function updateCanTable(frames){
  if(!frames||!frames.length)return;
  var now=Date.now();
  for(var i=0;i<frames.length;i++){
    var f=frames[i];
    var key=f.id;
    if(!canFrames[key])canFrames[key]={id:f.id,dlc:f.dlc,data:f.data,count:f.count,hz:f.hz};
    else{var cf=canFrames[key];cf.dlc=f.dlc;cf.data=f.data;cf.count=f.count;cf.hz=f.hz;}
  }
  _canDirty=true;
  if(!_canRafPending){_canRafPending=true;requestAnimationFrame(renderCanTable);}
}
function renderCanTable(){
  _canRafPending=false;
  if(!_canDirty)return;
  _canDirty=false;
  var filter=$('canFilterInput').value.trim().toLowerCase();
  var keys=Object.keys(canFrames).sort(function(a,b){return a-b});
  var shown=0,parts=[];
  for(var k=0;k<keys.length;k++){
    var fr=canFrames[keys[k]];
    var hexId='0x'+fr.id.toString(16).toUpperCase().padStart(3,'0');
    var decId=String(fr.id);
    if(filter&&hexId.toLowerCase().indexOf(filter)===-1&&decId.indexOf(filter)===-1)continue;
    shown++;
    parts.push('<tr><td>'+hexId+'</td><td>'+decId+'</td><td>'+fr.dlc+'</td><td>'+fr.data+'</td><td>'+fr.count+'</td><td>'+fr.hz.toFixed(1)+'</td></tr>');
  }
  $('canTableBody').innerHTML=parts.join('');
  $('canIdCount').textContent=shown+' / '+keys.length+' IDs';
}

/* ═══════════════════════════════════════════════════
   OTA MD5 Hash (Web Crypto API)
   ═══════════════════════════════════════════════════ */
var otaFileMd5='';
async function computeFileMd5(file){
  try{
    var buf=await file.arrayBuffer();
    var hash=await crypto.subtle.digest('MD5',buf);
    var arr=new Uint8Array(hash);
    return Array.from(arr).map(function(b){return b.toString(16).padStart(2,'0')}).join('');
  }catch(e){return '';}
}

/* ═══════════════════════════════════════════════════
   Boot
   ═══════════════════════════════════════════════════ */
applyTheme();
applyLang();
setSLMode(slMode);
poll();
pollCanLive();
pollDriveStatus();
var _pollTimer=setInterval(poll,2000);
var _canTimer=setInterval(pollCanLive,1000);
document.addEventListener('visibilitychange',function(){
  if(document.hidden){clearInterval(_pollTimer);clearInterval(_canTimer);}
  else{poll();pollCanLive();_pollTimer=setInterval(poll,2000);_canTimer=setInterval(pollCanLive,1000);}
});
function updVp(){$('iVp').textContent=window.innerWidth+'×'+window.innerHeight}
updVp();window.addEventListener('resize',updVp);
</script>
</body>
</html>)rawliteral";
