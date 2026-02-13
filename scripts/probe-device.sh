#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/cu.usbmodem2101}"
ESPTOOL="$HOME/Library/Arduino15/packages/esp32/tools/esptool_py/5.1.0/esptool"

if [[ ! -x "$ESPTOOL" ]]; then
  echo "Could not find esptool at: $ESPTOOL"
  echo "Install/refresh esp32 core with arduino-cli first."
  exit 1
fi

echo "== board list =="
arduino-cli board list

echo
echo "== chip-id =="
"$ESPTOOL" --port "$PORT" chip-id

echo
echo "== flash-id =="
"$ESPTOOL" --port "$PORT" flash-id

echo
echo "== read-mac =="
"$ESPTOOL" --port "$PORT" read-mac
