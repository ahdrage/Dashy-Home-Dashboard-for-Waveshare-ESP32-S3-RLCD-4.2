# Getting started

## 1) Install prerequisites
- Arduino IDE 2.x or `arduino-cli`
- ESP32 core `3.3.6`
- Libraries:
  - `lvgl` `8.4.0`
  - `ArduinoJson` `7.x`

## 2) Configure secrets
1. Copy `firmware/arduino/08_LVGL_V8_Test/secrets.h.example` to `firmware/arduino/08_LVGL_V8_Test/secrets.h`
2. Fill values for:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `NETATMO_CLIENT_ID`
   - `NETATMO_CLIENT_SECRET`
   - `NETATMO_REFRESH_TOKEN`

Netatmo OAuth/token setup guide:
- `docs/netatmo-api.md`
- `docs/current-screen.md`

## 3) Open and build
- Sketch: `firmware/arduino/08_LVGL_V8_Test/08_LVGL_V8_Test.ino`
- Apply settings in `docs/board-settings.md`
- Compile and upload

## 4) CLI workflow
Compile:

```bash
./scripts/compile.sh
```

Upload:

```bash
./scripts/upload.sh /dev/cu.usbmodem2101
```

Probe board:

```bash
./scripts/probe-device.sh /dev/cu.usbmodem2101
```

Inspect app signatures currently flashed:

```bash
./scripts/dump-app-signatures.sh /dev/cu.usbmodem2101
```

## Boot/upload troubleshooting
If upload fails or device does not enumerate:
1. Long-press `PWR` to power off.
2. Hold `BOOT`.
3. Tap `PWR` to power on.
4. Keep holding `BOOT` for ~2 seconds, then release.
5. Re-run upload.

If serial port disappears:
- Unplug USB-C
- Wait 5 seconds
- Replug USB-C
- Tap `PWR` once

## Clock behavior
- Clock display now uses the onboard RTC (PCF85063).
- At boot, firmware tries to sync RTC from NTP in Oslo time.
- It re-syncs every 6 hours when Wi-Fi is connected.
- If your clock is behind, connect Wi-Fi and reboot once to force an initial sync.

## Battery runtime note
- Observed runtime with current configuration is about **2 days per charge**.
- Your runtime will vary with Wi-Fi conditions, data polling cadence, and display update behavior.
