# Netatmo API integration

This project reads weather data from Netatmo using OAuth2 and `GET /getstationsdata`.

## Official references
- Create app: https://dev.netatmo.com/apps/createanapp#form
- OAuth docs: https://dev.netatmo.com/apidocumentation/oauth
- Weather docs: https://dev.netatmo.com/apidocumentation/weather#getstationsdata

## Required scope
Use `read_station`.

Netatmo docs list `read_station` as the scope for weather station endpoints such as `getstationsdata`.

## Secrets file
Create `firmware/arduino/08_LVGL_V8_Test/secrets.h` from `secrets.h.example` and set:

```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define NETATMO_CLIENT_ID "YOUR_NETATMO_CLIENT_ID"
#define NETATMO_CLIENT_SECRET "YOUR_NETATMO_CLIENT_SECRET"
#define NETATMO_REFRESH_TOKEN "YOUR_NETATMO_REFRESH_TOKEN"
```

## How to get the initial refresh token
This project uses refresh-token flow on-device, so you need one initial refresh token.

1. Create a Netatmo app.
2. Send user to authorize URL (scope `read_station`):

```text
https://api.netatmo.com/oauth2/authorize?client_id=YOUR_APP_ID&redirect_uri=YOUR_REDIRECT_URI&scope=read_station&state=YOUR_RANDOM_STATE
```

3. Netatmo redirects back with `code=...`.
4. Exchange code for access + refresh token:

```bash
curl -X POST 'https://api.netatmo.com/oauth2/token' \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data-urlencode 'grant_type=authorization_code' \
  --data-urlencode 'client_id=YOUR_APP_ID' \
  --data-urlencode 'client_secret=YOUR_CLIENT_SECRET' \
  --data-urlencode 'code=CODE_FROM_REDIRECT' \
  --data-urlencode 'redirect_uri=YOUR_REDIRECT_URI' \
  --data-urlencode 'scope=read_station'
```

5. Put the returned `refresh_token` into `secrets.h` as `NETATMO_REFRESH_TOKEN`.

## Exact runtime flow used in this repo
Source: `firmware/arduino/08_LVGL_V8_Test/08_LVGL_V8_Test.ino`.

```c
static const char *NETATMO_TOKEN_URL = "https://api.netatmo.com/oauth2/token";
static const char *NETATMO_STATIONS_URL = "https://api.netatmo.com/api/getstationsdata";
```

### Refresh access token

```c
String body = "grant_type=refresh_token";
body += "&client_id=" + String(NETATMO_CLIENT_ID);
body += "&client_secret=" + String(NETATMO_CLIENT_SECRET);
body += "&refresh_token=" + refreshToken;

int code = http.POST(body);
```

### Fetch station data

```c
http.begin(client, NETATMO_STATIONS_URL);
http.addHeader("Authorization", "Bearer " + accessToken);
int code = http.GET();
```

On `401/403`, firmware refreshes token and retries `GET` once.

### JSON fields mapped to screen

```c
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
```

If `NAModule1` is not found, code falls back to the first module's `dashboard_data.Temperature`.

## Refresh/update cadence
- Netatmo fetch interval in firmware: every 3 minutes (`UPDATE_INTERVAL_MS = 3 * 60 * 1000`).
- Clock on screen updates every second (`CLOCK_INTERVAL_MS = 1000`).

