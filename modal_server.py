#!/usr/bin/env python3
"""
MAKOTO EXECUTOR - Modal VPS Server
Hospeda scripts e permite execução remota
"""

import os
import json
import time
import hashlib
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path

PORT = 8080
SCRIPTS_DIR = Path('./scripts')
SCRIPTS_DIR.mkdir(exist_ok=True)

SCRIPT_REGISTRY = {}
EXECUTION_LOG = []
SERVER_START = time.time()

class MakotoHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.send_dashboard()
        elif self.path == '/api/status':
            self.send_status()
        elif self.path == '/api/scripts':
            self.send_scripts()
        elif self.path.startswith('/api/script/'):
            self.send_script(self.path.split('/')[-1])
        else:
            self.send_404()

    def do_POST(self):
        if self.path == '/api/execute':
            self.handle_execute()
        elif self.path == '/api/script/create':
            self.handle_create()
        else:
            self.send_404()

    def send_dashboard(self):
        html = """<!DOCTYPE html>
<html><head><title>MAKOTO EXECUTOR</title>
<style>
:root { --red: #E53935; --dark-red: #B71C1C; --black: #0a0a0a; --dark: #1a1a1a; --white: #fff; }
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Segoe UI', sans-serif; background: var(--black); color: var(--white); min-height: 100vh; }
.header { background: linear-gradient(135deg, var(--dark-red), var(--red)); padding: 20px; text-align: center; border-bottom: 3px solid var(--red); }
.header h1 { font-size: 2em; text-transform: uppercase; letter-spacing: 3px; }
.container { max-width: 1200px; margin: 0 auto; padding: 20px; }
.card { background: var(--dark); border-radius: 10px; padding: 20px; margin-bottom: 20px; border: 1px solid #333; }
.card h2 { color: var(--red); margin-bottom: 15px; }
.btn { background: var(--red); color: var(--white); border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-weight: bold; text-transform: uppercase; }
.btn:hover { background: var(--dark-red); }
input, textarea { width: 100%; padding: 12px; background: #2a2a2a; border: 1px solid #444; border-radius: 5px; color: var(--white); margin-bottom: 10px; }
.status { display: inline-block; width: 12px; height: 12px; border-radius: 50%; background: #4CAF50; margin-right: 8px; }
</style></head><body>
<div class="header"><h1>MAKOTO EXECUTOR</h1><p>Remote Script Server</p></div>
<div class="container">
<div class="card"><h2>Status</h2><p><span class="status"></span> Online | Scripts: <span id="count">0</span> | Uptime: <span id="uptime">0s</span></p></div>
<div class="card"><h2>Quick Execute</h2><input id="url" placeholder="Script URL..."><button class="btn" onclick="execute()">Execute</button></div>
<div class="card"><h2>Scripts</h2><div id="list"></div></div>
</div>
<script>
async function load() {
    const s = await (await fetch('/api/status')).json();
    document.getElementById('count').textContent = s.count;
    document.getElementById('uptime').textContent = s.uptime;
    const l = await (await fetch('/api/scripts')).json();
    document.getElementById('list').innerHTML = l.scripts.map(s =>
        `<div style="padding:10px;background:#2a2a2a;margin:5px 0;border-radius:5px;display:flex;justify-content:space-between">
            <span>${s.name}</span><button class="btn" onclick="exec('${s.id}')">Run</button>
        </div>`).join('');
}
async function execute() {
    const url = document.getElementById('url').value;
    await fetch('/api/execute', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({url})});
}
async function exec(id) {
    await fetch('/api/execute', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({script_id:id})});
}
load(); setInterval(load, 5000);
</script></body></html>"""
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(html.encode())

    def send_status(self):
        self.send_json({'status':'online', 'count':len(SCRIPT_REGISTRY), 'uptime':f"{int(time.time()-SERVER_START)}s"})

    def send_scripts(self):
        self.send_json({'scripts':[{'id':k,'name':v['name']} for k,v in SCRIPT_REGISTRY.items()]})

    def send_script(self, sid):
        if sid in SCRIPT_REGISTRY:
            self.send_json(SCRIPT_REGISTRY[sid])
        else:
            self.send_404()

    def handle_execute(self):
        length = int(self.headers.get('Content-Length', 0))
        data = json.loads(self.rfile.read(length))
        EXECUTION_LOG.append({'time': time.strftime('%H:%M:%S'), 'action': data})
        self.send_json({'status':'success'})

    def handle_create(self):
        length = int(self.headers.get('Content-Length', 0))
        data = json.loads(self.rfile.read(length))
        sid = hashlib.md5(f"{data['name']}{time.time()}".encode()).hexdigest()[:12]
        SCRIPT_REGISTRY[sid] = {'name': data['name'], 'url': data.get('url',''), 'created': time.strftime('%Y-%m-%d %H:%M:%S')}
        self.send_json({'status':'success', 'id':sid})

    def send_json(self, data):
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def send_404(self):
        self.send_response(404)
        self.end_headers()

    def log_message(self, fmt, *args):
        pass

if __name__ == '__main__':
    print(f"[MAKOTO] Server starting on port {PORT}...")
    HTTPServer(('0.0.0.0', PORT), MakotoHandler).serve_forever()
