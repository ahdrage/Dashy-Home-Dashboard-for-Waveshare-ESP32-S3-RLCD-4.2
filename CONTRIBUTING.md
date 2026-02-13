# Contributing

## Development setup
1. Install Arduino IDE 2.x or `arduino-cli`.
2. Install `esp32:esp32` core `3.3.6`.
3. Install libraries `lvgl` `8.4.0` and `ArduinoJson`.
4. Create `firmware/arduino/08_LVGL_V8_Test/secrets.h` from `secrets.h.example`.

## Typical workflow
1. Branch from `main`.
2. Make changes.
3. Run `./scripts/compile.sh`.
4. If hardware is connected, run `./scripts/upload.sh /dev/cu.usbmodem2101`.
5. Open a pull request with:
   - what changed
   - why
   - hardware/firmware test notes

## Scope
- Keep board-specific behavior for Waveshare ESP32-S3-RLCD-4.2 stable.
- Do not commit secrets (`secrets.h`, tokens, SSIDs, passwords).
