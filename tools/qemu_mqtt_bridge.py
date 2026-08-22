
#!/usr/bin/env python3

import sys
import re
import subprocess


MQTT_HOST = "localhost"
MQTT_PORT = "1883"

PREFIX = "[MQTT-PUBLISH] topic="


def publish(topic, payload):
    command = [
        "mosquitto_pub",
        "-h", MQTT_HOST,
        "-p", MQTT_PORT,
        "-t", topic,
        "-m", payload
    ]

    result = subprocess.run(
        command,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True
    )

    if result.returncode == 0:
        print(
            f"[BRIDGE] Published to {topic}",
            flush=True
        )
    else:
        print(
            f"[BRIDGE] ERROR: {result.stderr.strip()}",
            file=sys.stderr,
            flush=True
        )


def process_line(line):
    line = line.strip()

    if not line.startswith(PREFIX):
        return

    remaining = line[len(PREFIX):]

    marker = " payload="

    if marker not in remaining:
        print(
            "[BRIDGE] ERROR: Invalid MQTT line",
            file=sys.stderr,
            flush=True
        )
        return

    topic, payload = remaining.split(
        marker,
        1
    )

    topic = topic.strip()
    payload = payload.strip()

    if not topic or not payload:
        return

    print(
        f"[BRIDGE] MQTT message detected",
        flush=True
    )

    print(
        f"[BRIDGE] topic={topic}",
        flush=True
    )

    print(
        f"[BRIDGE] payload={payload}",
        flush=True
    )

    publish(topic, payload)


def main():
    print(
        "[BRIDGE] QEMU → Mosquitto bridge started",
        flush=True
    )

    print(
        f"[BRIDGE] Mosquitto: {MQTT_HOST}:{MQTT_PORT}",
        flush=True
    )

    print(
        "[BRIDGE] Waiting for QEMU telemetry...",
        flush=True
    )

    for line in sys.stdin:
        process_line(line)


if __name__ == "__main__":
    main()

