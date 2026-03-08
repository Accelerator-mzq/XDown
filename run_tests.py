import os
import subprocess
import sys
import time

# Get the Qt paths
qt_bin = r"E:\Qt\6.5.3\mingw_64\bin"
qt_plugins = r"E:\Qt\6.5.3\mingw_64\plugins"

# MSYS2 / Git Bash MinGW bin path - must be BEFORE Qt to ensure correct libstdc++ version
# This uses GCC 14.2 compatible libstdc++ instead of Qt's GCC 11.2 version
mingw_bin = r"E:\DevelopmentTool\mingw64\bin"

# Get current PATH and prepend MSYS2 + Qt (MSYS2 first for libstdc++ priority)
env = os.environ.copy()
current_path = env.get("PATH", "")
env["PATH"] = f"{mingw_bin};{current_path};{qt_bin}"
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
