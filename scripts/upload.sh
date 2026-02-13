#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKETCH_DIR="$ROOT_DIR/firmware/arduino/08_LVGL_V8_Test"
PORT="${1:-/dev/cu.usbmodem2101}"

FQBN='esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi'

"$ROOT_DIR/scripts/compile.sh"
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH_DIR"
