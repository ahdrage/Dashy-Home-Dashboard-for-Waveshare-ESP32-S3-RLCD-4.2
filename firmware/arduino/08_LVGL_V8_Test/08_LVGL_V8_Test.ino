#include "display_bsp.h"
#include "src/app_bsp/lvgl_bsp.h"
#include "lvgl.h"
#include "secrets.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SensorPCF85063.hpp>
#include <Wire.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

DisplayPort RlcdPort(12, 11, 5, 40, 41, 400, 300);
static lv_obj_t *time_label = nullptr;
static lv_obj_t *date_label = nullptr;
static lv_obj_t *indoor_val_label = nullptr;
static lv_obj_t *outdoor_val_label = nullptr;
static lv_obj_t *co2_val_label = nullptr;

static const char *NETATMO_TOKEN_URL = "https://api.netatmo.com/oauth2/token";
static const char *NETATMO_STATIONS_URL = "https://api.netatmo.com/api/getstationsdata";
static const unsigned long UPDATE_INTERVAL_MS = 3UL * 60UL * 1000UL;
static const unsigned long CLOCK_INTERVAL_MS = 1000UL;
static const unsigned long RTC_RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
static const char *OSLO_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";

static const char *NO_DAYS[] = {
  "S\xC3\xB8ndag", "Mandag", "Tirsdag", "Onsdag",
  "Torsdag", "Fredag", "L\xC3\xB8rdag"
};
static const char *NO_MONTHS[] = {
  "januar", "februar", "mars", "april", "mai", "juni",
  "juli", "august", "september", "oktober", "november", "desember"
};
static String accessToken;
static String refreshToken = NETATMO_REFRESH_TOKEN;
static String lastError;
static SensorPCF85063 rtc;
static bool rtcReady = false;
static bool rtcHasValidTime = false;
static unsigned long lastRtcResyncAtMs = 0;

static void Lvgl_FlushCallback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
  uint16_t *buffer = (uint16_t *)color_map;
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      uint8_t color = (*buffer < 0x7fff) ? ColorBlack : ColorWhite;
      RlcdPort.RLCD_SetPixel(x, y, color);
      buffer++;
    }
  }
  RlcdPort.RLCD_Display();
  lv_disp_flush_ready(drv);
}

static void set_label_text(lv_obj_t *label, const char *text) {
  if (!label) return;
  if (Lvgl_lock(1000)) {
    lv_label_set_text(label, text);
    Lvgl_unlock();
  }
}

static bool wifi_connect() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    vTaskDelay(pdMS_TO_TICKS(250));
  }
  return WiFi.status() == WL_CONNECTED;
}

static bool rtc_setup() {
  if (rtcReady) return true;
  rtcReady = rtc.begin(Wire, 13, 14);
  if (!rtcReady) {
    lastError = "RTC init failed";
    return false;
  }
  rtc.start();
  rtcHasValidTime = rtc.isClockIntegrityGuaranteed();
  return true;
}

static bool sync_ntp_to_rtc() {
  if (!rtc_setup()) return false;
  if (!wifi_connect()) return false;

  configTzTime(OSLO_TZ, "pool.ntp.org", "time.nist.gov");
  time_t now = 0;
  for (int i = 0; i < 40; ++i) {
    now = time(nullptr);
    if (now > 1700000000) break;
    vTaskDelay(pdMS_TO_TICKS(250));
  }
  if (now <= 1700000000) {
    lastError = "NTP sync timeout";
    return false;
  }

  struct tm localTm;
  localtime_r(&now, &localTm);
  rtc.setDateTime(
    (uint16_t)(localTm.tm_year + 1900),
    (uint8_t)(localTm.tm_mon + 1),
    (uint8_t)localTm.tm_mday,
    (uint8_t)localTm.tm_hour,
    (uint8_t)localTm.tm_min,
    (uint8_t)localTm.tm_sec
  );
  rtcHasValidTime = true;
  lastRtcResyncAtMs = millis();
  return true;
}

static bool rtc_get_local_tm(struct tm *out) {
  if (!out) return false;
  if (!rtc_setup()) return false;
  if (!rtcHasValidTime) {
    rtcHasValidTime = rtc.isClockIntegrityGuaranteed();
    if (!rtcHasValidTime) return false;
  }

  RTC_DateTime dt = rtc.getDateTime();
  out->tm_year = (int)dt.getYear() - 1900;
  out->tm_mon = (int)dt.getMonth() - 1;
  out->tm_mday = (int)dt.getDay();
  out->tm_hour = (int)dt.getHour();
  out->tm_min = (int)dt.getMinute();
  out->tm_sec = (int)dt.getSecond();
  out->tm_wday = (int)dt.getWeek();
  return true;
}

static int netatmo_refresh_token() {
  if (!wifi_connect()) return -100;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  http.begin(client, NETATMO_TOKEN_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "grant_type=refresh_token";
  body += "&client_id=" + String(NETATMO_CLIENT_ID);
  body += "&client_secret=" + String(NETATMO_CLIENT_SECRET);
  body += "&refresh_token=" + refreshToken;

  int code = http.POST(body);
  if (code != 200) {
    lastError = http.errorToString(code);
    http.end();
    return code;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, payload)) return -101;

  const char *new_access = doc["access_token"];
  const char *new_refresh = doc["refresh_token"];
  if (new_access) accessToken = new_access;
  if (new_refresh && strlen(new_refresh) > 0) refreshToken = new_refresh;
  lastError = "";
  return accessToken.length() > 0 ? 200 : -102;
}

static int fetch_netatmo(float &indoor_temp, float &outdoor_temp, int &co2_ppm) {
  indoor_temp = NAN;
  outdoor_temp = NAN;
  co2_ppm = -1;
  if (accessToken.length() == 0) {
    int rc = netatmo_refresh_token();
    if (rc != 200) return rc;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  http.begin(client, NETATMO_STATIONS_URL);
  http.addHeader("Authorization", "Bearer " + accessToken);

  int code = http.GET();
  if (code == 401 || code == 403) {
    http.end();
    int rc = netatmo_refresh_token();
    if (rc != 200) return rc;
    http.begin(client, NETATMO_STATIONS_URL);
    http.addHeader("Authorization", "Bearer " + accessToken);
    code = http.GET();
  }

  if (code != 200) {
    lastError = http.errorToString(code);
    http.end();
    return code;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(48 * 1024);
  if (deserializeJson(doc, payload)) {
    lastError = "JSON parse error";
    return -201;
  }

  JsonObject device0 = doc["body"]["devices"][0];
  indoor_temp = device0["dashboard_data"]["Temperature"] | NAN;
  co2_ppm = device0["dashboard_data"]["CO2"] | -1;

  JsonArray modules = device0["modules"];
  for (JsonObject m : modules) {
    const char *type = m["type"] | "";
    if (strcmp(type, "NAModule1") == 0) {
      outdoor_temp = m["dashboard_data"]["Temperature"] | NAN;
      break;
    }
  }

  if (isnan(outdoor_temp) && modules.size() > 0) {
    outdoor_temp = modules[0]["dashboard_data"]["Temperature"] | NAN;
  }

  if (isnan(outdoor_temp) && isnan(indoor_temp)) {
    lastError = "No temperature data";
    return -202;
  }
  lastError = "";
  return 200;
}

void setup() {
  RlcdPort.RLCD_Init();
  Lvgl_PortInit(400, 300, Lvgl_FlushCallback);
  rtc_setup();
  sync_ntp_to_rtc();
  if (Lvgl_lock(-1)) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    /* ── Clock: large centered time (HH:MM:SS) ── */
    time_label = lv_label_create(screen);
    lv_obj_set_width(time_label, 400);
    lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_letter_space(time_label, 2, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 30);
    lv_label_set_text(time_label, "--:--:--");

    /* ── Date beneath the clock (bolder, Norwegian) ── */
    date_label = lv_label_create(screen);
    lv_obj_set_width(date_label, 400);
    lv_obj_set_style_text_color(date_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 92);
    lv_label_set_text(date_label, "--- --, ----");

    /* ── Separator line ── */
    lv_obj_t *line = lv_obj_create(screen);
    lv_obj_set_size(line, 340, 1);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 135);

    /* ── Three data columns: Inne | CO2 | Ute ── */
    /* Column positions: 67 (left third), 200 (center), 333 (right third) */

    /* Indoor temperature */
    lv_obj_t *indoor_title = lv_label_create(screen);
    lv_obj_set_style_text_color(indoor_title, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(indoor_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(indoor_title, 2, 0);
    lv_obj_align(indoor_title, LV_ALIGN_TOP_MID, -133, 155);
    lv_label_set_text(indoor_title, "INNE");

    indoor_val_label = lv_label_create(screen);
    lv_obj_set_style_text_color(indoor_val_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(indoor_val_label, &lv_font_montserrat_28, 0);
    lv_obj_align(indoor_val_label, LV_ALIGN_TOP_MID, -133, 180);
    lv_label_set_text(indoor_val_label, "--.-\xC2\xB0""C");

    /* CO2 level */
    lv_obj_t *co2_title = lv_label_create(screen);
    lv_obj_set_style_text_color(co2_title, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(co2_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(co2_title, 2, 0);
    lv_obj_align(co2_title, LV_ALIGN_TOP_MID, 0, 155);
    lv_label_set_text(co2_title, "CO2");

    co2_val_label = lv_label_create(screen);
    lv_obj_set_style_text_color(co2_val_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(co2_val_label, &lv_font_montserrat_28, 0);
    lv_obj_align(co2_val_label, LV_ALIGN_TOP_MID, 0, 180);
    lv_label_set_text(co2_val_label, "---- ppm");

    /* Outdoor temperature */
    lv_obj_t *outdoor_title = lv_label_create(screen);
    lv_obj_set_style_text_color(outdoor_title, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(outdoor_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(outdoor_title, 2, 0);
    lv_obj_align(outdoor_title, LV_ALIGN_TOP_MID, 133, 155);
    lv_label_set_text(outdoor_title, "UTE");

    outdoor_val_label = lv_label_create(screen);
    lv_obj_set_style_text_color(outdoor_val_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(outdoor_val_label, &lv_font_montserrat_28, 0);
    lv_obj_align(outdoor_val_label, LV_ALIGN_TOP_MID, 133, 180);
    lv_label_set_text(outdoor_val_label, "--.-\xC2\xB0""C");

    /* ── Vertical dividers between columns ── */
    lv_obj_t *div1 = lv_obj_create(screen);
    lv_obj_set_size(div1, 1, 65);
    lv_obj_set_style_bg_color(div1, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(div1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_set_style_radius(div1, 0, 0);
    lv_obj_set_style_pad_all(div1, 0, 0);
    lv_obj_align(div1, LV_ALIGN_TOP_MID, -67, 158);

    lv_obj_t *div2 = lv_obj_create(screen);
    lv_obj_set_size(div2, 1, 65);
    lv_obj_set_style_bg_color(div2, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(div2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div2, 0, 0);
    lv_obj_set_style_radius(div2, 0, 0);
    lv_obj_set_style_pad_all(div2, 0, 0);
    lv_obj_align(div2, LV_ALIGN_TOP_MID, 67, 158);

    /* ── Bottom accent line ── */
    lv_obj_t *bottom_line = lv_obj_create(screen);
    lv_obj_set_size(bottom_line, 60, 3);
    lv_obj_set_style_bg_color(bottom_line, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bottom_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_line, 0, 0);
    lv_obj_set_style_radius(bottom_line, 1, 0);
    lv_obj_set_style_pad_all(bottom_line, 0, 0);
    lv_obj_align(bottom_line, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_scr_load(screen);
    Lvgl_unlock();
  }

  if (wifi_connect()) {
    float indoor = NAN;
    float outdoor = NAN;
    int co2 = -1;
    int code = fetch_netatmo(indoor, outdoor, co2);
    if (code == 200) {
      char buf[24];
      snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", indoor);
      set_label_text(indoor_val_label, buf);
      snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", outdoor);
      set_label_text(outdoor_val_label, buf);
      if (co2 >= 0) {
        snprintf(buf, sizeof(buf), "%d ppm", co2);
        set_label_text(co2_val_label, buf);
      }
    }
  }
}

void loop() {
  static unsigned long lastFetch = 0;
  static unsigned long lastClock = 0;

  if (millis() - lastFetch >= UPDATE_INTERVAL_MS || lastFetch == 0) {
    if (!wifi_connect()) {
      lastFetch = millis();
      vTaskDelay(pdMS_TO_TICKS(500));
      return;
    }

    float indoor = NAN;
    float outdoor = NAN;
    int co2 = -1;
    int code = fetch_netatmo(indoor, outdoor, co2);
    if (code == 200) {
      char buf[24];
      snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", indoor);
      set_label_text(indoor_val_label, buf);
      snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", outdoor);
      set_label_text(outdoor_val_label, buf);
      if (co2 >= 0) {
        snprintf(buf, sizeof(buf), "%d ppm", co2);
        set_label_text(co2_val_label, buf);
      }
    }

    lastFetch = millis();
  }

  if (millis() - lastClock >= CLOCK_INTERVAL_MS) {
    if (millis() - lastRtcResyncAtMs >= RTC_RESYNC_INTERVAL_MS) {
      sync_ntp_to_rtc();
    }

    struct tm t = {};
    bool hasTime = rtc_get_local_tm(&t);
    if (!hasTime) {
      time_t now = time(nullptr);
      if (now > 100000) {
        localtime_r(&now, &t);
        hasTime = true;
      }
    }
    if (hasTime) {
      char tbuf[12];
      char dbuf[48];
      strftime(tbuf, sizeof(tbuf), "%H:%M:%S", &t);
      snprintf(dbuf, sizeof(dbuf), "%s %d. %s %d",
               NO_DAYS[t.tm_wday], t.tm_mday,
               NO_MONTHS[t.tm_mon], t.tm_year + 1900);
      set_label_text(time_label, tbuf);
      set_label_text(date_label, dbuf);
    }
    lastClock = millis();
  }

  vTaskDelay(pdMS_TO_TICKS(500));
}
