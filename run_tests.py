import os
import subprocess
import sys
import time

# Get the Qt paths
qt_bin = r"E:\Qt\6.5.3\mingw_64\bin"
qt_plugins = r"E:\Qt\6.5.3\mingw_64\plugins"

# Get current PATH and prepend Qt bin
env = os.environ.copy()
current_path = env.get("PATH", "")
env["PATH"] = f"{current_path};{qt_bin}"
env["QT_PLUGIN_PATH"] = qt_plugins
env["PYTHONIOENCODING"] = "utf-8"
env["PYTHONUTF8"] = "1"

print("Starting mock HTTP server...")
server_process = subprocess.Popen([sys.executable, "tests/src/mock_http_server.py"])
time.sleep(2)  # Wait for server to start

try:
    # Run ctest with the modified environment
    cmd = ["ctest", "--test-dir", "build", "-C", "Debug", "-V"]
    print(f"Running: {' '.join(cmd)} with PATH injected")
    result = subprocess.run(cmd, env=env, capture_output=False)
finally:
    print("Terminating mock HTTP server...")
    server_process.terminate()
    server_process.wait()

sys.exit(result.returncode)
