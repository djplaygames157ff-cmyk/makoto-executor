# MAKOTO EXECUTOR v2.0

Executor dinâmico para Roblox com UI moderna em tema vermelho/preto/branco.

## Estrutura

```
roblox-executor/
├── executor.cpp       # Core executor logic
├── lua_core.cpp      # Lua 5.4 integration
├── injector.cpp      # DLL injection
├── evasion.cpp       # Anti-detection
├── ui.cpp            # ImGui interface
├── host.cpp          # Local injector host
├── server.py         # Modal VPS server
├── modal_deploy.py   # Modal deployment
├── CMakeLists.txt    # Build configuration
└── lua/              # Lua 5.4 source
```

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Output: `MakotoExecutor.dll`

## Uso Local

1. Compile a DLL
2. Execute `host.exe` (injetor local)
3. Abra o Roblox
4. O host detecta e injeta automaticamente
5. Use a UI para executar scripts

## Deploy Modal (VPS)

```bash
pip install modal
modal deploy modal_deploy.py
```

Acesse o dashboard web para gerenciar scripts remotamente.

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/status` | GET | Server status |
| `/api/scripts` | GET | List scripts |
| `/api/execute` | POST | Execute script |
| `/api/script/create` | POST | Create script |
| `/api/script/<id>` | DELETE | Delete script |

## Tema

- Vermelho: `#E53935`
- Preto: `#0A0A0A`
- Branco: `#FFFFFF`
