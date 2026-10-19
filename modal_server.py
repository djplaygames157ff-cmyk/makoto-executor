#!/usr/bin/env python3
"""
MAKOTO EXECUTOR - Servidor Web (Flask)
Compatível com gunicorn para deploy no Render
"""

from flask import Flask, jsonify, request, send_file
from pathlib import Path
import time
import hashlib
import json
import os

app = Flask(__name__)

SCRIPTS_DIR = Path('./scripts')
SCRIPTS_DIR.mkdir(exist_ok=True)

SCRIPT_REGISTRY = {}
EXECUTION_LOG = []
SERVER_START = time.time()

DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MAKOTO EXECUTOR</title>
    <style>
        :root {
            --red: #E53935;
            --dark-red: #B71C1C;
            --black: #0a0a0a;
            --dark: #1a1a1a;
            --light: #2a2a2a;
            --white: #ffffff;
            --gray: #888888;
        }
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: var(--black);
            color: var(--white);
            min-height: 100vh;
        }
        .header {
            background: linear-gradient(135deg, var(--dark-red), var(--red));
            padding: 25px;
            text-align: center;
            border-bottom: 3px solid var(--red);
        }
        .header h1 {
            font-size: 2em;
            text-transform: uppercase;
            letter-spacing: 4px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
        }
        .header p { opacity: 0.8; margin-top: 5px; font-size: 0.9em; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .card {
            background: var(--dark);
            border-radius: 10px;
            padding: 20px;
            margin-bottom: 20px;
            border: 1px solid var(--light);
            transition: border-color 0.3s;
        }
        .card:hover { border-color: var(--red); }
        .card h2 {
            color: var(--red);
            margin-bottom: 15px;
            font-size: 1.2em;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }
        .status-online { background: #4CAF50; box-shadow: 0 0 10px #4CAF50; }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .btn {
            background: var(--red);
            color: var(--white);
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s;
            text-transform: uppercase;
            font-weight: bold;
            letter-spacing: 1px;
        }
        .btn:hover {
            background: var(--dark-red);
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(229, 57, 53, 0.4);
        }
        .btn-secondary {
            background: var(--light);
            border: 1px solid var(--gray);
        }
        .btn-secondary:hover {
            background: var(--dark);
            box-shadow: none;
        }
        .btn-danger {
            background: transparent;
            border: 1px solid var(--red);
            color: var(--red);
        }
        .btn-danger:hover {
            background: var(--red);
            color: var(--white);
        }
        input[type="text"], textarea {
            width: 100%;
            padding: 12px;
            background: var(--black);
            border: 1px solid var(--gray);
            border-radius: 5px;
            color: var(--white);
            font-family: 'Consolas', monospace;
            margin-bottom: 10px;
            transition: border-color 0.3s;
        }
        input:focus, textarea:focus {
            outline: none;
            border-color: var(--red);
        }
        textarea { min-height: 150px; resize: vertical; }
        .script-list { list-style: none; }
        .script-item {
            background: var(--light);
            padding: 15px;
            margin-bottom: 10px;
            border-radius: 5px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-left: 3px solid var(--red);
        }
        .script-item .name { font-weight: bold; color: var(--red); }
        .script-item .actions { display: flex; gap: 8px; }
        .script-item .actions .btn { padding: 6px 12px; font-size: 12px; }
        .log-entry {
            background: var(--light);
            padding: 10px;
            margin-bottom: 5px;
            border-radius: 3px;
            font-family: 'Consolas', monospace;
            font-size: 12px;
            border-left: 2px solid var(--gray);
        }
        .log-entry.success { border-left-color: #4CAF50; }
        .log-entry.error { border-left-color: var(--red); }
        .log-entry .time { color: var(--gray); }
        .log-entry .success-text { color: #4CAF50; }
        .log-entry .error-text { color: var(--red); }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        @media (max-width: 768px) { .grid { grid-template-columns: 1fr; } }
        .stats {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-bottom: 20px;
        }
        .stat-box {
            background: var(--light);
            padding: 15px;
            border-radius: 8px;
            text-align: center;
        }
        .stat-box .value {
            font-size: 2em;
            font-weight: bold;
            color: var(--red);
        }
        .stat-box .label {
            font-size: 0.8em;
            color: var(--gray);
            text-transform: uppercase;
        }
        .tabs {
            display: flex;
            gap: 5px;
            margin-bottom: 15px;
            border-bottom: 2px solid var(--light);
        }
        .tab {
            padding: 10px 20px;
            cursor: pointer;
            border-bottom: 2px solid transparent;
            margin-bottom: -2px;
            transition: all 0.3s;
        }
        .tab:hover { color: var(--red); }
        .tab.active {
            border-bottom-color: var(--red);
            color: var(--red);
        }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        .empty-state {
            text-align: center;
            padding: 40px;
            color: var(--gray);
        }
        .empty-state .icon { font-size: 3em; margin-bottom: 10px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>MAKOTO EXECUTOR</h1>
        <p>Servidor de Execução Remota v2.0</p>
    </div>
    <div class="container">
        <div class="stats">
            <div class="stat-box">
                <div class="value" id="stat-scripts">0</div>
                <div class="label">Scripts</div>
            </div>
            <div class="stat-box">
                <div class="value" id="stat-exec">0</div>
                <div class="label">Execuções</div>
            </div>
            <div class="stat-box">
                <div class="value" id="stat-uptime">0s</div>
                <div class="label">Uptime</div>
            </div>
        </div>

        <div class="tabs">
            <div class="tab active" onclick="switchTab('scripts')">Scripts</div>
            <div class="tab" onclick="switchTab('logs')">Logs</div>
        </div>

        <div id="tab-scripts" class="tab-content active">
            <div class="card">
                <h2>Biblioteca de Scripts</h2>
                <div class="grid" style="margin-bottom: 15px;">
                    <input type="text" id="script-name" placeholder="Nome do script...">
                    <input type="text" id="script-url" placeholder="URL do script...">
                </div>
                <button class="btn" onclick="addScript()">Adicionar Script</button>
                <button class="btn btn-secondary" onclick="loadExample()">Carregar Exemplo</button>
                <hr style="border-color: var(--light); margin: 15px 0;">
                <ul class="script-list" id="script-list">
                    <div class="empty-state">
                        <div class="icon">📁</div>
                        <p>Nenhum script carregado</p>
                    </div>
                </ul>
            </div>
        </div>

        <div id="tab-logs" class="tab-content">
            <div class="card">
                <h2>Logs de Execução</h2>
                <div id="logs">
                    <div class="empty-state">
                        <div class="icon">📋</div>
                        <p>Nenhum log ainda</p>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let scripts = [];

        function switchTab(tab) {
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            event.target.classList.add('active');
            document.getElementById('tab-' + tab).classList.add('active');
        }

        async function loadData() {
            try {
                const [statusRes, scriptsRes, logsRes] = await Promise.all([
                    fetch('/api/status'),
                    fetch('/api/scripts'),
                    fetch('/api/logs')
                ]);

                const status = await statusRes.json();
                const scriptsData = await scriptsRes.json();
                const logsData = await logsRes.json();

                document.getElementById('stat-scripts').textContent = status.script_count;
                document.getElementById('stat-exec').textContent = status.exec_count;
                document.getElementById('stat-uptime').textContent = status.uptime;

                scripts = scriptsData.scripts;
                renderScripts();
                renderLogs(logsData.logs);
            } catch (e) {
                console.error('Erro ao carregar:', e);
            }
        }

        function renderScripts() {
            const list = document.getElementById('script-list');
            if (scripts.length === 0) {
                list.innerHTML = '<div class="empty-state"><div class="icon">📁</div><p>Nenhum script carregado</p></div>';
                return;
            }
            list.innerHTML = scripts.map(s => `
                <li class="script-item">
                    <div>
                        <div class="name">${s.name}</div>
                        <div style="font-size: 0.75em; color: var(--gray);">${s.hash.substring(0, 8)}</div>
                    </div>
                    <div class="actions">
                        <button class="btn" onclick="executeScript('${s.id}')">Executar</button>
                        <button class="btn btn-danger" onclick="deleteScript('${s.id}')">Remover</button>
                    </div>
                </li>
            `).join('');
        }

        function renderLogs(logs) {
            const container = document.getElementById('logs');
            if (logs.length === 0) {
                container.innerHTML = '<div class="empty-state"><div class="icon">📋</div><p>Nenhum log ainda</p></div>';
                return;
            }
            container.innerHTML = logs.map(log => `
                <div class="log-entry ${log.success ? 'success' : 'error'}">
                    <span class="time">[${log.time}]</span>
                    <span class="${log.success ? 'success-text' : 'error-text'}">${log.success ? 'SUCCESS' : 'ERROR'}</span>
                    - ${log.message}
                </div>
            `).join('');
        }

        async function addScript() {
            const name = document.getElementById('script-name').value;
            const url = document.getElementById('script-url').value;
            if (!name || !url) return alert('Preencha todos os campos');

            try {
                await fetch('/api/script/create', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name, url })
                });
                document.getElementById('script-name').value = '';
                document.getElementById('script-url').value = '';
                loadData();
            } catch (e) {
                alert('Erro ao adicionar script');
            }
        }

        function loadExample() {
            const examples = [
                { name: 'Hello World', url: 'https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source' },
                { name: 'Infinite Yield', url: 'https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source' },
                { name: 'ESP Library', url: 'https://raw.githubusercontent.com/example/esp/main/esp.lua' }
            ];
            const ex = examples[Math.floor(Math.random() * examples.length)];
            document.getElementById('script-name').value = ex.name;
            document.getElementById('script-url').value = ex.url;
        }

        async function executeScript(id) {
            try {
                await fetch('/api/execute', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ script_id: id })
                });
                setTimeout(loadData, 500);
            } catch (e) {
                alert('Erro ao executar');
            }
        }

        async function deleteScript(id) {
            if (!confirm('Remover este script?')) return;
            try {
                await fetch('/api/script/' + id, { method: 'DELETE' });
                loadData();
            } catch (e) {
                alert('Erro ao remover');
            }
        }

        loadData();
        setInterval(loadData, 5000);
    </script>
</body>
</html>"""


@app.route('/')
def index():
    return DASHBOARD_HTML

@app.route('/api/status')
def api_status():
    uptime = int(time.time() - SERVER_START)
    hours = uptime // 3600
    minutes = (uptime % 3600) // 60
    seconds = uptime % 60
    return jsonify({
        'status': 'online',
        'server': 'MAKOTO EXECUTOR v2.0',
        'script_count': len(SCRIPT_REGISTRY),
        'exec_count': len(EXECUTION_LOG),
        'uptime': f'{hours}h {minutes}m {seconds}s'
    })

@app.route('/api/scripts')
def api_scripts():
    scripts = []
    for sid, data in SCRIPT_REGISTRY.items():
        scripts.append({
            'id': sid,
            'name': data['name'],
            'url': data.get('url', ''),
            'hash': data.get('hash', ''),
            'created': data.get('created', '')
        })
    return jsonify({'scripts': scripts})

@app.route('/api/script/<script_id>')
def api_get_script(script_id):
    if script_id in SCRIPT_REGISTRY:
        return jsonify(SCRIPT_REGISTRY[script_id])
    return jsonify({'error': 'Script não encontrado'}), 404

@app.route('/api/script/create', methods=['POST'])
def api_create_script():
    data = request.get_json()
    name = data.get('name', '')
    url = data.get('url', '')

    if not name:
        return jsonify({'error': 'Nome é obrigatório'}), 400

    script_id = hashlib.md5(f"{name}{time.time()}".encode()).hexdigest()[:12]
    script_hash = hashlib.sha256(url.encode()).hexdigest() if url else ''

    SCRIPT_REGISTRY[script_id] = {
        'name': name,
        'url': url,
        'hash': script_hash,
        'created': time.strftime('%Y-%m-%d %H:%M:%S')
    }

    log_execution(f"Script criado: {name}", True)
    return jsonify({'status': 'success', 'id': script_id})

@app.route('/api/script/<script_id>', methods=['DELETE'])
def api_delete_script(script_id):
    if script_id in SCRIPT_REGISTRY:
        name = SCRIPT_REGISTRY[script_id]['name']
        del SCRIPT_REGISTRY[script_id]
        log_execution(f"Script removido: {name}", True)
        return jsonify({'status': 'success'})
    return jsonify({'error': 'Script não encontrado'}), 404

@app.route('/api/execute', methods=['POST'])
def api_execute():
    data = request.get_json()
    script_id = data.get('script_id')
    url = data.get('url')

    if script_id and script_id in SCRIPT_REGISTRY:
        script_data = SCRIPT_REGISTRY[script_id]
        log_execution(f"Executando: {script_data['name']}", True)
        return jsonify({
            'status': 'success',
            'message': f"Script '{script_data['name']}' enviado para execução",
            'script_id': script_id
        })
    elif url:
        log_execution(f"Carregando de URL: {url[:50]}...", True)
        return jsonify({
            'status': 'success',
            'message': 'Script carregado da URL',
            'url': url
        })

    return jsonify({'error': 'script_id ou url necessário'}), 400

@app.route('/api/logs')
def api_logs():
    return jsonify({'logs': list(reversed(EXECUTION_LOG[-50:]))})

@app.route('/health')
def health():
    return jsonify({'status': 'ok', 'server': 'MAKOTO'})

def log_execution(message, success):
    EXECUTION_LOG.append({
        'time': time.strftime('%H:%M:%S'),
        'message': message,
        'success': success
    })
    if len(EXECUTION_LOG) > 100:
        EXECUTION_LOG.pop(0)

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    app.run(host='0.0.0.0', port=port, debug=False)
