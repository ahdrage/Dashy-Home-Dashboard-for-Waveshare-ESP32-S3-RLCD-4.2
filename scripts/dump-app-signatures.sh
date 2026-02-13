#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/cu.usbmodem2101}"
OFFSET="${2:-0x10000}"
SIZE="${3:-0x200000}"
OUT="${4:-/tmp/esp32s3_app_dump.bin}"
ESPTOOL="$HOME/Library/Arduino15/packages/esp32/tools/esptool_py/5.1.0/esptool"

if [[ ! -x "$ESPTOOL" ]]; then
  echo "Could not find esptool at: $ESPTOOL"
  exit 1
fi

"$ESPTOOL" --port "$PORT" read-flash "$OFFSET" "$SIZE" "$OUT"

echo
echo "Dump written to: $OUT"
echo "Possible source and app signatures:"
strings "$OUT" | rg -i '08_LVGL_V8_Test|display_bsp|lvgl_bsp|netatmo|worldtimeapi|timezone/Etc/UTC' | head -n 200 || true
