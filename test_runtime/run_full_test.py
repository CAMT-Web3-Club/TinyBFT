#!/usr/bin/env python3
"""Test TinyBFT with SET/GET commands"""

import subprocess
import time

PROJECT_DIR = "/home/phukrit7171/Development/TinyBFT"
CONF = f"{PROJECT_DIR}/test_runtime/test.conf"
KEYS = f"{PROJECT_DIR}/test_runtime/priv"


def main():
    # Kill existing
    subprocess.run("pkill -9 -f simple 2>/dev/null", shell=True)
    time.sleep(1)

    # Start replicas
    for i in range(4):
        port = 5679 + i
        key = f"{KEYS}/r{i}.pem"
        cmd = f"./examples/build/simple_replica {CONF} {key} {port}"
        subprocess.Popen(
            cmd,
            shell=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            cwd=PROJECT_DIR,
        )
        time.sleep(1.5)

    time.sleep(3)

    # Run test_client
    result = subprocess.run(
        f"./examples/build/test_client {CONF} {KEYS}/client0.pem",
        shell=True,
        capture_output=True,
        text=True,
        cwd=PROJECT_DIR,
        timeout=60,
    )

    print("=== Client Output ===")
    print(result.stdout)
    print(result.stderr)

    subprocess.run("pkill -9 -f simple 2>/dev/null", shell=True)


if __name__ == "__main__":
    main()
