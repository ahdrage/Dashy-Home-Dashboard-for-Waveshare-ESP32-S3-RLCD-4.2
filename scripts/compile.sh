#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKETCH_DIR="$ROOT_DIR/firmware/arduino/08_LVGL_V8_Test"
SECRETS_FILE="$SKETCH_DIR/secrets.h"

FQBN='esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi'

if [[ ! -f "$SECRETS_FILE" ]]; then
  echo "Missing $SECRETS_FILE"
  echo "Create it from $SKETCH_DIR/secrets.h.example"
  exit 1
fi

arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"
