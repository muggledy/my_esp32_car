#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>

//项目上线后需注释掉ENABLE_SERIAL_DEBUG宏
#define ENABLE_SERIAL_DEBUG

#define ENABLE_LCD1602

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

#endif