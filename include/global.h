#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>

//可注释掉ENABLE_SERIAL_DEBUG宏
#define ENABLE_SERIAL_DEBUG

#define ENABLE_LCD1602

#define ENABLE_HCSR04

//板载LED引脚（可设置值）
#define ONBOARD_LED_PIN 2
//END

//入参为LED阳极引脚，高电平点亮、低电平熄灭
#define turn_on_led(pin)  digitalWrite(pin, HIGH)
#define turn_off_led(pin) digitalWrite(pin, LOW)

//FLAGs for global_status_word
#define G_STATE_WIFI_CONNECTED 0x00000001
#define G_STATE_WS_CONNECTED   0x00000002 //客户端（如手机控制端）已连接

#define SET_FLAG(field, flag) (field) |= flag
#define CLR_FLAG(field, flag) (field) &= (~flag)
#define TST_FLAG(field, flag) ((field) & flag)

#define SYSTEM_UP_SECONDS ((unsigned long)(millis()/1000))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

//HC-SR04超声波模块引脚（可设置值）
#define TRIG_PIN  27
#define ECHO_PIN  26
//END
#define HCSR04_TRIGGER_INTERVAL 200 //超声波间隔200ms周期探测一次
#define HCSR04_DISTANCE_BIAS 5
#define HCSR04_SAFE_DISTANCE 12 //安全距离
#define HCSR04_MIN_DISTANCE 2
#define HCSR04_MAX_DISTANCE 360

#endif