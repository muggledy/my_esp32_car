#include "global.h"
#include "motor.h"
#ifdef ENABLE_LCD1602
#include "LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27, 16, 2);
#endif
#include <WebSocketsServer.h>

/*
ESP32 DEVKIT V1
SMART CAR
*/

extern bool connect_wifi();
extern void on_web_socket_event(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
extern String handle_command(String cmd);

uint32_t global_status_word = 0;
// WebSocket 服务器实例，监听端口 81
WebSocketsServer* web_socket = nullptr;

void setup() {
#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
#endif
  pinMode(ONBOARD_LED_PIN, OUTPUT);
#ifdef ENABLE_LCD1602
  lcd.init();
  lcd.backlight();
  lcd.print("I'm smart car.");
#endif

  init_car_motor();
  connect_wifi();

  //只有联网成功，才启动WebSocket服务器
  if (TST_FLAG(global_status_word, G_STATE_WIFI_CONNECTED)) {
    web_socket = new WebSocketsServer(81);
    web_socket->begin();
    web_socket->onEvent(on_web_socket_event);
#ifdef ENABLE_SERIAL_DEBUG
    Serial.println("[WebSocket] Server started on port 81");
#endif
#ifdef ENABLE_LCD1602
    lcd.setCursor(0, 1);
    lcd.print("server ready");
#endif
  }
}

void loop() {
  // delay(1000);
  // MOTOR_MOVE_FULL_SPEED(&car);
  // delay(1000);
  // MOTOR_MOVE_STOP(&car);
  if (TST_FLAG(global_status_word, G_STATE_WIFI_CONNECTED)) {
    // 处理WebSocket事件（接收消息、发送响应、管理连接）
    web_socket->loop();
    // delay(10);
  }
}

// WebSocket 事件处理函数
void on_web_socket_event(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
#ifdef ENABLE_LCD1602
  char buffer[32] = {0};
#endif
  switch (type) {
    case WStype_DISCONNECTED: // 客户端断开连接
      {
#ifdef ENABLE_SERIAL_DEBUG
        Serial.printf("[WebSocket] Client #%u disconnected\n", num);
#endif
        CLR_FLAG(global_status_word, G_STATE_WS_CONNECTED);
        // 断开连接时自动停车（安全考虑）
        MOTOR_MOVE_STOP(&car);
      }
      break;
    case WStype_CONNECTED: // 新客户端连接
      {
          IPAddress ip = web_socket->remoteIP(num);
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] Client #%u connected from %d.%d.%d.%d\n",
              num, ip[0], ip[1], ip[2], ip[3]);
#endif
          SET_FLAG(global_status_word, G_STATE_WS_CONNECTED);
#ifdef ENABLE_LCD1602
          snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d#%u connected", ip[0], ip[1], ip[2], ip[3], num);
          lcd.clear();
          lcd.print(buffer);
#endif
      }
      break;
    case WStype_TEXT: // 收到文本消息
      {
          String cmd = String((char*)payload);
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] Received cmd: %s\n", cmd.c_str());
#endif
#ifdef ENABLE_LCD1602
          snprintf(buffer, sizeof(buffer), "[cmd]%s", cmd.c_str());
          lcd.clear();
          lcd.print(buffer);
#endif
          // 解析并执行指令
          String response = handle_command(cmd);
          // 发送响应
          web_socket->sendTXT(num, response);
      }
      break;
    case WStype_BIN: // 收到二进制消息（本项目不使用）
#ifdef ENABLE_SERIAL_DEBUG
      Serial.println("[WebSocket] Binary message received(ignored)");
#endif
      break;
    case WStype_ERROR: // 发生错误
#ifdef ENABLE_SERIAL_DEBUG
      Serial.printf("[WebSocket] Error from client #%u\n", num);
#endif
      break;
    case WStype_PING:
    case WStype_PONG: // 心跳检测（库自动处理）
      break;
    default:
      break;
  }
}

// 指令处理函数
String handle_command(String cmd) {
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "forward") {
        if (car.gear != MOTOR_DRIVE) {
          MOTOR_SET_DRIVE_GEAR(&car);
        }
        if (CURRENT_SPEED(&car) == 0) {
          MOTOR_MOVE_WITH_SPEED(&car, MOTOR_STANDARD_SPEED);
        }
        return "ok: moving forward";
    }
    else if (cmd == "backward") {
        // 后退：IN1=HIGH, IN2=LOW, PWM=max
        if (car.gear != MOTOR_REVERSE) {
          MOTOR_SET_REVERSE_GEAR(&car);
        }
        if (CURRENT_SPEED(&car) == 0) {
          MOTOR_MOVE_WITH_SPEED(&car, MOTOR_STANDARD_SPEED);
        }
        return "ok: moving backward";
    }
    else if (cmd == "left") {
        return "ok: turning left";
    }
    else if (cmd == "right") {
        return "ok: turning right";
    }
    else if (cmd == "stop") {
        MOTOR_MOVE_STOP(&car);
        return "ok: stopped";
    }
    else if (cmd == "status") {
        // 返回当前状态
        return "status: xxx";
    }
    else {
        return "error: unknown command '" + cmd + "'";
    }
}