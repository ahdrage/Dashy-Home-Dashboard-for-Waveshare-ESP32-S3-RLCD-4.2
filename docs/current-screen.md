# Current screen output (exact)

This document mirrors what the firmware currently renders on the 400x300 display.

Source of truth: `firmware/arduino/08_LVGL_V8_Test/08_LVGL_V8_Test.ino`.

## What is shown
- Top: time (`HH:MM:SS`)
- Under time: date in Norwegian (`<weekday> <day>. <month> <year>`)
- Bottom row:
  - left: `INNE` + indoor temperature in `°C`
  - center: `CO2` + ppm
  - right: `UTE` + outdoor temperature in `°C`

## Exact label/layout code

```c
/* Clock */
time_label = lv_label_create(screen);
lv_obj_set_width(time_label, 400);
lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 30);
lv_label_set_text(time_label, "--:--:--");

/* Date */
date_label = lv_label_create(screen);
lv_obj_set_width(date_label, 400);
lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 92);
lv_label_set_text(date_label, "--- --, ----");

/* Indoor */
lv_label_set_text(indoor_title, "INNE");
lv_obj_align(indoor_title, LV_ALIGN_TOP_MID, -133, 155);
lv_obj_align(indoor_val_label, LV_ALIGN_TOP_MID, -133, 180);

/* CO2 */
lv_label_set_text(co2_title, "CO2");
lv_obj_align(co2_title, LV_ALIGN_TOP_MID, 0, 155);
lv_obj_align(co2_val_label, LV_ALIGN_TOP_MID, 0, 180);

/* Outdoor */
lv_label_set_text(outdoor_title, "UTE");
lv_obj_align(outdoor_title, LV_ALIGN_TOP_MID, 133, 155);
lv_obj_align(outdoor_val_label, LV_ALIGN_TOP_MID, 133, 180);
```

## Exact value formatting code

```c
snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", indoor);
set_label_text(indoor_val_label, buf);

snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", outdoor);
set_label_text(outdoor_val_label, buf);

if (co2 >= 0) {
  snprintf(buf, sizeof(buf), "%d ppm", co2);
  set_label_text(co2_val_label, buf);
}

strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &t);
snprintf(dbuf, sizeof(dbuf), "%s %d. %s %d",
         NO_DAYS[t.tm_wday], t.tm_mday,
         NO_MONTHS[t.tm_mon], t.tm_year + 1900);
set_label_text(time_label, tbuf);
set_label_text(date_label, dbuf);
```

## Data source mapping
- `INNE`: `devices[0].dashboard_data.Temperature`
- `CO2`: `devices[0].dashboard_data.CO2`
- `UTE`: `modules[type == "NAModule1"].dashboard_data.Temperature` (fallback: first module temp)

See `docs/netatmo-api.md` for OAuth and endpoint setup.


## Clock source (now)
- Primary source: onboard PCF85063 RTC over I2C (SDA 13, SCL 14)
- RTC is synced from NTP using Oslo timezone string: `CET-1CEST,M3.5.0,M10.5.0/3`
- Automatic RTC resync interval: every 6 hours (when Wi-Fi is available)
- If RTC is not yet valid, firmware temporarily falls back to system time from `time(nullptr)`
