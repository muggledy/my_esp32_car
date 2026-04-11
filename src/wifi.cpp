#include <WiFi.h>
#include "global.h"
#ifdef ENABLE_LCD1602
#include "LiquidCrystal_I2C.h"
extern LiquidCrystal_I2C lcd;
#endif

#define CONNECT_WIFI_WAIT_TIME_ONCE_MS   500
#define CONNECT_WIFI_WAIT_TIME_MAX_COUNT 30 //等待wifi连接时长≈15s

extern uint32_t global_status_word;

static const char* ssid     = "TP-LINK_07D2"; // Change this to your WiFi SSID
static const char* password = "2016SSJL"; // Change this to your WiFi password

//return: 1 means success, 0 failed
bool connect_wifi() {
    uint8_t n = 0;

#ifdef ENABLE_SERIAL_DEBUG
    Serial.print("Connecting to WiFi ");
    Serial.print(ssid);
#endif

    WiFi.begin(ssid, password);

    while ((WiFi.status() != WL_CONNECTED) && (n++ < CONNECT_WIFI_WAIT_TIME_MAX_COUNT)) {
        delay(CONNECT_WIFI_WAIT_TIME_ONCE_MS);
#ifdef ENABLE_SERIAL_DEBUG
        Serial.print(".");
#endif
    }

    if (WiFi.status() == WL_CONNECTED) {
#ifdef ENABLE_SERIAL_DEBUG
        Serial.println("");
        Serial.printf("[INFO] WiFi connected(%lus)\n", SYSTEM_UP_SECONDS);
        Serial.print("Allocated IP address: ");
        Serial.println(WiFi.localIP());
#endif
        //wifi连接成功，板载led小蓝灯连续闪烁三下
#define WIFI_BLINK_LED_COUNT 3
        for (n=0; n<WIFI_BLINK_LED_COUNT; n++) {
            turn_on_led(ONBOARD_LED_PIN);
            delay(200);
            turn_off_led(ONBOARD_LED_PIN);
            if (n<(WIFI_BLINK_LED_COUNT-1)) {
                delay(200);
            }
        }
        SET_FLAG(global_status_word, G_STATE_WIFI_CONNECTED);
#ifdef ENABLE_LCD1602
        lcd.clear();
        lcd.print(WiFi.localIP());
#endif
        return 1;
    }
#ifdef ENABLE_SERIAL_DEBUG
    Serial.println("");
    Serial.println("[WARNING] WiFi connect failed");
#endif
#ifdef ENABLE_LCD1602
    lcd.clear();
    lcd.print("WiFi connect failed");
#endif
    return 0;
}