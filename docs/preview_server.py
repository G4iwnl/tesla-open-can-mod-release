#!/usr/bin/env python3
"""
Local preview server for Tesla CAN Mod web UI.
Serves the HTML from web_ui.h with mock API responses.
Usage: python3 docs/preview_server.py
Then open http://<your-ip>:8080 in Tesla browser or local browser.
"""
import http.server
import json
import re
import time
import os
import socket

PORT = 9090
START = time.time()

# Extract HTML from web_ui.h
def load_html():
    ui_path = os.path.join(os.path.dirname(__file__), '..', 'include', 'web', 'web_ui.h')
    with open(ui_path, 'r', encoding='utf-8') as f:
        content = f.read()
    m = re.search(r'R"rawliteral\((.*)\)rawliteral"', content, re.DOTALL)
    if m:
        return m.group(1).encode('utf-8')
    return b'<h1>Failed to extract HTML from web_ui.h</h1>'

HTML = load_html()

MOCK_STATUS = {
    "fsd_enabled": True,
    "bypass_tlssc_requirement": True,
    "isa_speed_chime_suppress": False,
    "emergency_vehicle_detection": True,
    "enhanced_autopilot": True,
    "nag_killer": False,
    "china_mode": False,
    "profile_mode_auto": False,
    "preheat": False,
    "hw_mode": 2,
    "speed_profile": 1,
    "speed_offset": 0,
    "enable_print": True,
    "uptime_s": 0,
    "log_head": 0,
    "version": "2.1.0-preview",
    "ap_ssid": "TeslaCAN",
    "sta_ssid": "MyWiFi",
    "sta_ip": "192.168.2.253",
    "sta_connected": True,
    "features": {
        "bypass_tlssc_requirement": {"supported": True, "enabled": True, "build_enabled": True},
        "isa_speed_chime_suppress": {"supported": True, "enabled": False, "build_enabled": True},
        "emergency_vehicle_detection": {"supported": True, "enabled": True, "build_enabled": True},
        "enhanced_autopilot": {"supported": True, "enabled": True, "build_enabled": True},
        "nag_killer": {"supported": False, "enabled": False, "build_enabled": True},
        "china_mode": {"supported": True, "enabled": False, "build_enabled": True},
        "profile_mode_auto": {"supported": True, "enabled": False, "build_enabled": True},
        "preheat": {"supported": True, "enabled": False, "build_enabled": True},
        "ota": {"supported": True, "enabled": True, "build_enabled": True}
    },
    "logs": [],
    "can": {
        "state": "RUNNING",
        "rx_errors": 0,
        "tx_errors": 0,
        "bus_errors": 0,
        "rx_missed": 0,
        "rx_queued": 12,
        "frames_received": 0,
        "frames_sent": 0
    },
    "monitor": {"inited": True, "enabled": True, "entries": 0, "capacity": 256}
}

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(f"[{time.strftime('%H:%M:%S')}] {fmt % args}")

    def _json(self, data, code=200):
        body = json.dumps(data).encode()
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', len(body))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == '/' or self.path.startswith('/?'):
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', len(HTML))
            self.end_headers()
            self.wfile.write(HTML)
        elif self.path.startswith('/api/status'):
            status = dict(MOCK_STATUS)
            elapsed = int(time.time() - START)
            status['uptime_s'] = elapsed
            status['can']['frames_received'] = elapsed * 847
            status['can']['frames_sent'] = elapsed * 12
            # Simulate some log entries
            if elapsed > 2 and elapsed % 5 == 0:
                status['logs'] = [{"msg": f"[MOCK] Frame modified at t={elapsed}s", "ts": elapsed * 1000}]
                status['log_head'] = elapsed
            self._json(status)
        elif self.path.startswith('/api/'):
            self._json({"ok": True})
        else:
            self.send_error(404)

    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length) if length else b''
        path = self.path

        if '/api/hw-mode' in path:
            try:
                d = json.loads(body)
                MOCK_STATUS['hw_mode'] = d.get('mode', 1)
            except: pass
        elif '/api/speed-profile' in path:
            try:
                d = json.loads(body)
                MOCK_STATUS['speed_profile'] = d.get('value', 1)
            except: pass
        elif '/api/profile-mode-auto' in path:
            try:
                d = json.loads(body)
                MOCK_STATUS['profile_mode_auto'] = d.get('enabled', False)
            except: pass
        elif '/api/bypass-tlssc' in path:
            try:
                d = json.loads(body)
                MOCK_STATUS['features']['bypass_tlssc_requirement']['enabled'] = d.get('enabled', False)
                MOCK_STATUS['fsd_enabled'] = d.get('enabled', False)
            except: pass
        elif '/api/enable-print' in path:
            try:
                d = json.loads(body)
                MOCK_STATUS['enable_print'] = d.get('enabled', True)
            except: pass

        self._json({"ok": True})

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

if __name__ == '__main__':
    ip = get_local_ip()
    server = http.server.HTTPServer(('0.0.0.0', PORT), Handler)
    print(f"\n  Tesla CAN Mod UI Preview Server")
    print(f"  ================================")
    print(f"  Local:   http://127.0.0.1:{PORT}")
    print(f"  Network: http://{ip}:{PORT}")
    print(f"\n  Open the Network URL in Tesla browser to preview.\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
