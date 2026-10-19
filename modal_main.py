import modal

app = modal.App("makoto-executor-server")

image = (
    modal.Image.debian_slim(python_version="3.11")
    .pip_install("flask", "gunicorn")
    .add_local_file("modal_server.py", "/root/modal_server.py")
)

@app.function(image=image, cpu=1.0, memory=1024, min_containers=1)
@modal.asgi_app()
def makoto_server():
    from modal_server import HTTPServer, MakotoHandler
    import threading
    server = HTTPServer(('0.0.0.0', 8080), MakotoHandler)
    thread = threading.Thread(target=server.serve_forever)
    thread.start()
    return app

@app.local_entrypoint()
def main():
    print("Deployando MAKOTO EXECUTOR para Modal...")
    print("Após deploy, acesse: https://djplaygames157ff-makoto-executor-server.modal.run")
