# Device verification

This verification was run on **February 13, 2026** against a connected board at `/dev/cu.usbmodem2101`.

## Hardware probe results
Using Arduino-bundled `esptool`:

- Chip: `ESP32-S3 (revision v0.2)`
- Embedded PSRAM: `8MB`
- Flash: `16MB`
- USB mode: `USB-Serial/JTAG`
- MAC: `20:6e:f1:a9:a3:8c`

## On-device firmware check
A 2MB app-region dump was read from flash (`0x10000` offset) and inspected with `strings`.
Equivalent repo command:

```bash
./scripts/dump-app-signatures.sh /dev/cu.usbmodem2101
```

The dump includes source path markers pointing to:
- `/Users/alf/Downloads/ESP32-S3-RLCD-4.2-main/Example/Arduino/08_LVGL_V8_Test/...`

This indicates the currently flashed firmware was compiled from that local sketch lineage.

## Notes
- Exact source reconstruction from flash binaries is not lossless.
- Path markers and runtime strings are a strong indicator, not a byte-for-byte source proof.
