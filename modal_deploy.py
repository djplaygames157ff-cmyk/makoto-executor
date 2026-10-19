import modal

app = modal.App("makoto-executor-server")

image = (
    modal.Image.debian_slim(python_version="3.11")
    .pip_install("flask", "gunicorn")
    .copy_local_file("server.py", "/root/server.py")
)

@app.function(image=image, cpu=1.0, memory=1024)
def run_server():
    import subprocess
    import sys
    subprocess.run([
        sys.executable, "-m", "flask", "run",
        "--host=0.0.0.0", "--port=8080"
    ], cwd="/root")

@app.local_entrypoint()
def main():
    print("MAKOTO EXECUTOR - Modal Deployment")
    print("Run with: modal deploy modal_deploy.py")
