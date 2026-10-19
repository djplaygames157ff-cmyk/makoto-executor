#!/usr/bin/env python3
"""
MAKOTO EXECUTOR - Server Host
Run on Modal VPS to host scripts remotely
"""

import os
import sys
import json
import time
import hashlib
import hmac
import subprocess
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import threading
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [MAKOTO] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

# MAKOTO Server Config
MAKOTO_PORT = 8080
MAKOTO_SECRET = os.environ.get('MAKOTO_SECRET', 'makoto-secret-key-change-me')
SCRIPTS_DIR = Path('./scripts')
SCRIPTS_DIR.mkdir(exist_ok=True)

# In-memory script storage
SCRIPT_REGISTRY = {}
EXECUTION_LOG = []

class MakotoHandler(BaseHTTPRequestHandler):
    """HTTP request handler for MAKOTO EXECUTOR server"""

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path == '/':
            self.send_index()
        elif parsed.path == '/api/status':
            self.send_status()
        elif parsed.path == '/api/scripts':
            self.send_scripts()
        elif parsed.path == '/api/logs':
            self.send_logs()
        elif parsed.path.startswith('/api/script/'):
            script_id = parsed.path.split('/')[-1]
            self.send_script(script_id)
        elif parsed.path == '/health':
            self.send_health()
        else:
            self.send_404()

    def do_POST(self):
        parsed = urlparse(self.path)

        if parsed.path == '/api/execute':
            self.handle_execute()
        elif parsed.path == '/api/upload':
            self.handle_upload()
        elif parsed.path == '/api/script/create':
            self.handle_create_script()
        else:
            self.send_404()

    def do_DELETE(self):
        parsed = urlparse(self.path)

        if parsed.path.startswith('/api/script/'):
            script_id = parsed.path.split('/')[-1]
            self.handle_delete_script(script_id)
        else:
            self.send_404()

    def send_index(self):
        """Send main dashboard HTML"""
        html = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MAKOTO EXECUTOR - Server</title>
    <style>
        :root {
            --makoto-red: #E53935;
            --makoto-dark-red: #B71C1C;
            --makoto-black: #0a0a0a;
            --makoto-dark: #1a1a1a;
            --makoto-light: #2a2a2a;
            --makoto-white: #ffffff;
            --makoto-gray: #888888;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: var(--makoto-black);
            color: var(--makoto-white);
            min-height: 100vh;
        }

        .header {
            background: linear-gradient(135deg, var(--makoto-dark-red), var(--makoto-red));
            padding: 20px;
            text-align: center;
            border-bottom: 3px solid var(--makoto-red);
        }

        .header h1 {
            font-size: 2em;
            text-transform: uppercase;
            letter-spacing: 3px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
        }

        .header p {
            opacity: 0.8;
            margin-top: 5px;
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }

        .card {
            background: var(--makoto-dark);
            border-radius: 10px;
            padding: 20px;
            margin-bottom: 20px;
            border: 1px solid var(--makoto-light);
            transition: border-color 0.3s;
        }

        .card:hover {
            border-color: var(--makoto-red);
        }

        .card h2 {
            color: var(--makoto-red);
            margin-bottom: 15px;
            font-size: 1.2em;
        }

        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }

        .status-online { background: #4CAF50; box-shadow: 0 0 10px #4CAF50; }
        .status-offline { background: #F44336; }
        .status-busy { background: #FF9800; }

        .btn {
            background: var(--makoto-red);
            color: var(--makoto-white);
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s;
            text-transform: uppercase;
            font-weight: bold;
        }

        .btn:hover {
            background: var(--makoto-dark-red);
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(229, 57, 53, 0.3);
        }

        .btn-danger {
            background: #d32f2f;
        }

        .btn-danger:hover {
            background: #b71c1c;
        }

        input[type="text"], textarea {
            width: 100%;
            padding: 12px;
            background: var(--makoto-light);
            border: 1px solid var(--makoto-gray);
            border-radius: 5px;
            color: var(--makoto-white);
            font-family: 'Consolas', monospace;
            margin-bottom: 10px;
        }

        textarea {
            min-height: 150px;
            resize: vertical;
        }

        input:focus, textarea:focus {
            outline: none;
            border-color: var(--makoto-red);
        }

        .script-list {
            list-style: none;
        }

        .script-item {
            background: var(--makoto-light);
            padding: 15px;
            margin-bottom: 10px;
            border-radius: 5px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .script-item .name {
            font-weight: bold;
            color: var(--makoto-red);
        }

        .script-item .actions {
            display: flex;
            gap: 10px;
        }

        .log-entry {
            background: var(--makoto-light);
            padding: 10px;
            margin-bottom: 5px;
            border-radius: 3px;
            font-family: 'Consolas', monospace;
            font-size: 12px;
        }

        .log-entry .time {
            color: var(--makoto-gray);
        }

        .log-entry .success { color: #4CAF50; }
        .log-entry .error { color: #F44336; }

        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }

        @media (max-width: 768px) {
            .grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>MAKOTO EXECUTOR</h1>
        <p>Remote Script Execution Server</p>
    </div>

    <div class="container">
        <div class="grid">
            <div class="card">
                <h2>Server Status</h2>
                <p><span class="status-indicator status-online"></span> Server Online</p>
                <p>Uptime: <span id="uptime">Loading...</span></p>
                <p>Scripts: <span id="script-count">0</span></p>
                <p>Executions: <span id="exec-count">0</span></p>
            </div>

            <div class="card">
                <h2>Quick Execute</h2>
                <input type="text" id="quick-url" placeholder="Paste script URL here...">
                <button class="btn" onclick="quickExecute()">Execute</button>
            </div>
        </div>

        <div class="card">
            <h2>Script Library</h2>
            <div class="grid" style="margin-bottom: 15px;">
                <input type="text" id="script-name" placeholder="Script name...">
                <input type="text" id="script-url" placeholder="Script URL...">
            </div>
            <button class="btn" onclick="addScript()">Add Script</button>
            <hr style="border-color: var(--makoto-light); margin: 15px 0;">
            <ul class="script-list" id="script-list">
                <!-- Scripts loaded dynamically -->
            </ul>
        </div>

        <div class="card">
            <h2>Execution Logs</h2>
            <div id="logs">
                <!-- Logs loaded dynamically -->
            </div>
        </div>
    </div>

        <script>
        const API_URL = window.location.origin;

        async function loadStatus() {
            try {
                const res = await fetch(`${API_URL}/api/status`);
                const data = await res.json();
                document.getElementById('uptime').textContent = data.uptime;
                document.getElementById('script-count').textContent = data.script_count;
                document.getElementById('exec-count').textContent = data.exec_count;
            } catch (e) {
                console.error('Failed to load status:', e);
            }
        }

        async function loadScripts() {
            try {
                const res = await fetch(`${API_URL}/api/scripts`);
                const data = await res.json();
                const list = document.getElementById('script-list');
                list.innerHTML = '';

                data.scripts.forEach(script => {
                    const li = document.createElement('li');
                    li.className = 'script-item';
                    li.innerHTML = `
                        <span class="name">${script.name}</span>
                        <span style="color: var(--makoto-gray); font-size: 12px;">${script.hash.substring(0, 8)}</span>
                        <div class="actions">
                            <button class="btn" onclick="executeScript('${script.id}')">Execute</button>
                            <button class="btn btn-danger" onclick="deleteScript('${script.id}')">Delete</button>
                        </div>
                    `;
                    list.appendChild(li);
                });
            } catch (e) {
                console.error('Failed to load scripts:', e);
            }
        }

        async function loadLogs() {
            try {
                const res = await fetch(`${API_URL}/api/logs`);
                const data = await res.json();
                const logsDiv = document.getElementById('logs');
                logsDiv.innerHTML = '';

                data.logs.forEach(log => {
                    const div = document.createElement('div');
                    div.className = 'log-entry';
                    const statusClass = log.success ? 'success' : 'error';
                    div.innerHTML = `
                        <span class="time">[${log.time}]</span>
                        <span class="${statusClass}">${log.success ? 'SUCCESS' : 'FAILED'}</span>
                        - ${log.message}
                    `;
                    logsDiv.appendChild(div);
                });
            } catch (e) {
                console.error('Failed to load logs:', e);
            }
        }

        async function quickExecute() {
            const url = document.getElementById('quick-url').value;
            if (!url) return alert('Please enter a URL');

            try {
                await fetch(`${API_URL}/api/execute`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ url })
                });
                loadLogs();
            } catch (e) {
                alert('Execution failed');
            }
        }

        async function addScript() {
            const name = document.getElementById('script-name').value;
            const url = document.getElementById('script-url').value;
            if (!name || !url) return alert('Please fill all fields');

            try {
                await fetch(`${API_URL}/api/script/create`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name, url })
                });
                loadScripts();
                loadStatus();
            } catch (e) {
                alert('Failed to add script');
            }
        }

        async function executeScript(id) {
            try {
                await fetch(`${API_URL}/api/execute`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ script_id: id })
                });
                loadLogs();
            } catch (e) {
                alert('Execution failed');
            }
        }

        async function deleteScript(id) {
            if (!confirm('Delete this script?')) return;

            try {
                await fetch(`${API_URL}/api/script/${id}`, { method: 'DELETE' });
                loadScripts();
                loadStatus();
            } catch (e) {
                alert('Failed to delete');
            }
        }

        // Load all data on startup
        loadStatus();
        loadScripts();
        loadLogs();

        // Refresh every 5 seconds
        setInterval(() => {
            loadStatus();
            loadScripts();
            loadLogs();
        }, 5000);
    </script>
</body>
</html>
"""
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(html.encode())

    def send_status(self):
        """Send server status as JSON"""
        status = {
            'status': 'online',
            'uptime': time.strftime('%H:%M:%S', time.gmtime(time.time() - SERVER_START_TIME)),
            'script_count': len(SCRIPT_REGISTRY),
            'exec_count': len(EXECUTION_LOG),
            'server': 'MAKOTO EXECUTOR v2.0'
        }
        self.send_json(status)

    def send_scripts(self):
        """Send script registry as JSON"""
        scripts = []
        for script_id, script_data in SCRIPT_REGISTRY.items():
            scripts.append({
                'id': script_id,
                'name': script_data['name'],
                'hash': script_data['hash'],
                'created': script_data['created']
            })
        self.send_json({'scripts': scripts})

    def send_script(self, script_id):
        """Send a specific script"""
        if script_id in SCRIPT_REGISTRY:
            script_data = SCRIPT_REGISTRY[script_id]
            self.send_json({
                'id': script_id,
                'name': script_data['name'],
                'url': script_data['url'],
                'hash': script_data['hash']
            })
        else:
            self.send_404()

    def send_logs(self):
        """Send execution logs as JSON"""
        self.send_json({'logs': EXECUTION_LOG[-50:]})

    def send_health(self):
        """Send health check response"""
        self.send_json({'status': 'ok', 'server': 'MAKOTO'})

    def handle_execute(self):
        """Handle script execution request"""
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        try:
            data = json.loads(body)
        except json.JSONDecodeError:
            self.send_error(400, 'Invalid JSON')
            return

        script_id = data.get('script_id')
        url = data.get('url')

        if script_id and script_id in SCRIPT_REGISTRY:
            script_data = SCRIPT_REGISTRY[script_id]
            self.log_execution(f"Executing script: {script_data['name']}", True)
            self.send_json({'status': 'success', 'message': f"Executing {script_data['name']}"})
        elif url:
            self.log_execution(f"Executing from URL: {url}", True)
            self.send_json({'status': 'success', 'message': 'Script loaded from URL'})
        else:
            self.send_error(400, 'No script_id or URL provided')

    def handle_upload(self):
        """Handle script upload"""
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        try:
            data = json.loads(body)
            name = data.get('name', 'unnamed')
            content = data.get('content', '')

            script_id = hashlib.md5(f"{name}{time.time()}".encode()).hexdigest()[:12]
            script_hash = hashlib.sha256(content.encode()).hexdigest()

            script_path = SCRIPTS_DIR / f"{script_id}.lua"
            with open(script_path, 'w') as f:
                f.write(content)

            SCRIPT_REGISTRY[script_id] = {
                'name': name,
                'url': str(script_path),
                'hash': script_hash,
                'created': time.strftime('%Y-%m-%d %H:%M:%S')
            }

            self.log_execution(f"Uploaded script: {name}", True)
            self.send_json({'status': 'success', 'id': script_id})
        except Exception as e:
            self.log_execution(f"Upload failed: {str(e)}", False)
            self.send_error(500, str(e))

    def handle_create_script(self):
        """Handle script creation from URL"""
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        try:
            data = json.loads(body)
            name = data.get('name')
            url = data.get('url')

            if not name or not url:
                self.send_error(400, 'Name and URL required')
                return

            script_id = hashlib.md5(f"{name}{url}".encode()).hexdigest()[:12]
            script_hash = hashlib.sha256(url.encode()).hexdigest()

            SCRIPT_REGISTRY[script_id] = {
                'name': name,
                'url': url,
                'hash': script_hash,
                'created': time.strftime('%Y-%m-%d %H:%M:%S')
            }

            self.log_execution(f"Created script: {name}", True)
            self.send_json({'status': 'success', 'id': script_id})
        except Exception as e:
            self.log_execution(f"Create failed: {str(e)}", False)
            self.send_error(500, str(e))

    def handle_delete_script(self, script_id):
        """Handle script deletion"""
        if script_id in SCRIPT_REGISTRY:
            del SCRIPT_REGISTRY[script_id]
            self.log_execution(f"Deleted script: {script_id}", True)
            self.send_json({'status': 'success'})
        else:
            self.send_error(404, 'Script not found')

    def send_json(self, data):
        """Send JSON response"""
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def send_404(self):
        """Send 404 response"""
        self.send_response(404)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({'error': 'Not found'}).encode())

    def log_execution(self, message, success):
        """Log an execution event"""
        EXECUTION_LOG.append({
            'time': time.strftime('%H:%M:%S'),
            'message': message,
            'success': success
        })
        if len(EXECUTION_LOG) > 100:
            EXECUTION_LOG.pop(0)

    def log_message(self, format, *args):
        """Override to use custom logging"""
        logger.info("%s - %s" % (self.address_string(), format % args))


SERVER_START_TIME = time.time()

def run_server():
    """Run the MAKOTO EXECUTOR server"""
    server = HTTPServer(('0.0.0.0', MAKOTO_PORT), MakotoHandler)
    logger.info(f"MAKOTO EXECUTOR server started on port {MAKOTO_PORT}")
    logger.info(f"Access the dashboard at http://localhost:{MAKOTO_PORT}")
    server.serve_forever()


# Modal deployment configuration
def modal_deploy():
    """Deploy to Modal VPS"""
    try:
        import modal

        app = modal.App("makoto-executor")

        image = modal.Image.debian_slim().pip_install("flask")

        @app.function(image=image, cpu=0.5, memory=512)
        def makoto_server():
            run_server()

        logger.info("Deploying to Modal...")
        with app.run():
            run_server()
    except ImportError:
        logger.warning("Modal SDK not installed. Running locally...")
        run_server()


if __name__ == '__main__':
    if '--modal' in sys.argv:
        modal_deploy()
    else:
        run_server()
