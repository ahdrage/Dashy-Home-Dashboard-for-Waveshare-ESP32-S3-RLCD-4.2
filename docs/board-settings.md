# Board settings (Waveshare ESP32-S3-RLCD-4.2)

These values are based on:
- Waveshare official wiki: https://docs.waveshare.com/ESP32-S3-RLCD-4.2
- Waveshare official repo screenshot (`Tools Configuration.png`):
  https://github.com/waveshareteam/ESP32-S3-RLCD-4.2
- Local known-good config used on this project.

## Arduino IDE settings
- Board: `ESP32S3 Dev Module`
- USB CDC On Boot: `Enabled`
- CPU Frequency: `240MHz (WiFi)`
- Flash Mode: `QIO 80MHz`
- Flash Size: `16MB (128Mb)`
- JTAG Adapter: `Disabled`
- Partition Scheme: `16M Flash (3MB APP/9.9MB FATFS)`
- PSRAM: `OPI PSRAM`
- Upload Mode: `UART0 / Hardware CDC`
- Upload Speed: `921600`
- USB Mode: `Hardware CDC and JTAG`

## Arduino CLI FQBN
Use this FQBN string in scripts/commands:

```text
esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
```

## LVGL font requirements
The current sketch uses these LVGL fonts:
- `lv_font_montserrat_14`
- `lv_font_montserrat_20`
- `lv_font_montserrat_28`
- `lv_font_montserrat_48`

If fonts are disabled in `lv_conf.h`, compilation will fail.

Set these to `1` in your `lv_conf.h`:

```c
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
```

Common location on macOS: `~/Documents/Arduino/libraries/lvgl/lv_conf.h`.
