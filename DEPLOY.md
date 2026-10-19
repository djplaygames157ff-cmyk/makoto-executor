# MAKOTO EXECUTOR - Guia de Deploy Gratuito

## Opção 1: Render (RECOMENDADO)
1. Acesse https://render.com e crie conta grátis (GitHub login)
2. Clique "New" → "Web Service"
3. Conecte seu repositório Git ou faça upload dos arquivos
4. Configure:
   - **Runtime:** Python 3
   - **Build Command:** `pip install -r requirements.txt`
   - **Start Command:** `gunicorn modal_server:app --bind 0.0.0.0:$PORT`
5. Clique "Deploy"

URL: `https://makoto-executor.onrender.com`

---

## Opção 2: Railway
1. Acesse https://railway.app (login com GitHub)
2. "New Project" → "Deploy from repo"
3. Upload ou conecte repositório
4. Runtime detecta Python automaticamente

---

## Opção 3: Cloudflare Workers (mais rápido)
1. Acesse https://dash.cloudflare.com
2. Workers & Pages → Create
3. Cole o código de `modal_server.py` adaptado como Worker

---

## Arquivos incluídos:
- `modal_server.py` - Servidor HTTP completo (dashboard + API)
- `requirements.txt` - Dependências Python
- `render.yaml` - Config Render
- `Procfile` - Config Railway/Heroku
