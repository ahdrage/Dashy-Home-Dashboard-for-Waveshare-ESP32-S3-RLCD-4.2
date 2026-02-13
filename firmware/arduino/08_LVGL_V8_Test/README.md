# 08_LVGL_V8_Test (Dashy variant)

## Files
- `08_LVGL_V8_Test.ino`: main app logic (clock + Netatmo data)
- `display_bsp.*` and `src/`: display/LVGL support files
- `secrets.h.example`: template for local credentials

## Setup
1. Copy `secrets.h.example` to `secrets.h`
2. Fill Wi-Fi and Netatmo credentials
3. Build with repo root script:
   - `../../../../scripts/compile.sh`

## Notes
- `secrets.h` is ignored by git.
- Board settings are documented in `docs/board-settings.md`.
