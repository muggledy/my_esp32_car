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
#ifdef ENABLE_LCD1602
extern void lcd_print_long_str(const char *str);
#endif

uint32_t global_status_word = 0;
// WebSocket 服务器实例，监听端口 81
WebSocketsServer* web_socket = nullptr;
static char buffer[32] = {0};
uint16_t global_car_speed = MOTOR_STANDARD_SPEED; //用于记录当前设定车速
int16_t valid_client_id = -1;
#if defined(ENABLE_LCD1602) || defined(ENABLE_SERIAL_DEBUG)
IPAddress remote_ip;
#endif

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

  if (TST_FLAG(global_status_word, G_STATE_WIFI_CONNECTED)) { //只有联网成功，才启动WebSocket服务器
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
  if (TST_FLAG(global_status_word, G_STATE_WIFI_CONNECTED)) {
    web_socket->loop(); // 处理WebSocket事件（接收消息、发送响应、管理连接）
    // delay(1);
  }
}

// WebSocket 事件处理函数
void on_web_socket_event(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // 客户端断开连接
      {
        if (TST_FLAG(global_status_word, G_STATE_WS_CONNECTED) && (valid_client_id == num)) {
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] Client #%u disconnected\n", num);
#endif
          CLR_FLAG(global_status_word, G_STATE_WS_CONNECTED);
          valid_client_id = -1;
#ifdef ENABLE_LCD1602
          snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d#%u disconnected", 
              remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], num);
          lcd_print_long_str(buffer);
          remote_ip = INADDR_NONE;
#endif
          // 断开连接时自动停车（安全考虑）
          MOTOR_MOVE_STOP(&car);
        }
      }
      break;
    case WStype_CONNECTED: // 新客户端连接
      {
#if defined(ENABLE_LCD1602) || defined(ENABLE_SERIAL_DEBUG)
          remote_ip = web_socket->remoteIP(num);
#endif
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] Client #%u connected from %d.%d.%d.%d(%lus)\n",
              num, remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], SYSTEM_UP_SECONDS);
#endif
          if (TST_FLAG(global_status_word, G_STATE_WS_CONNECTED)) {
            web_socket->disconnect(num);  // 只允许一个client接入，其余连接立刻踢掉
#ifdef ENABLE_SERIAL_DEBUG
            Serial.printf("[WebSocket] Reject new client #%u (already connected)\n", num);
#endif
            return;
          }
          SET_FLAG(global_status_word, G_STATE_WS_CONNECTED);
          valid_client_id = num;
#ifdef ENABLE_LCD1602
          snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d#%u connected", 
              remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], num);
          lcd_print_long_str(buffer);
#endif
      }
      break;
    case WStype_TEXT: // 收到文本消息
      {
          String cmd = String((char*)payload);
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] Received cmd(%lus): %s\n", SYSTEM_UP_SECONDS, cmd.c_str());
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
#ifdef ENABLE_LCD1602
          snprintf(buffer, sizeof(buffer), "%s:%u/%u", GET_CAR_GEAR_STR(&car), car.speed_left, car.speed_right);
          lcd.setCursor(0, 1);
          lcd.print(buffer);
#endif
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
    if ((GET_CURRENT_SPEED(&car) == 0) || (GET_CURRENT_SPEED(&car) != global_car_speed) 
        || IS_TURNING(&car)) {
      MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
    }
    return "ok: moving forward";
  }
  else if (cmd == "backward") {
    if (car.gear != MOTOR_REVERSE) {
      MOTOR_SET_REVERSE_GEAR(&car);
    }
    if ((GET_CURRENT_SPEED(&car) == 0) || (GET_CURRENT_SPEED(&car) != global_car_speed) 
        || IS_TURNING(&car)) {
      MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
    }
    return "ok: moving backward";
  } else if (cmd == "left") {
    if (!IS_TURNING_LEFT(&car)) {
      MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
      MOTOR_MOVE_LEFT(&car);
    }
    return "ok: turning left";
  } else if (cmd == "right") {
    if (!IS_TURNING_RIGHT(&car)) {
      MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
      MOTOR_MOVE_RIGHT(&car);
    }
    return "ok: turning right";
  } else if (cmd == "stop") {
    MOTOR_MOVE_STOP(&car);
    return "ok: stopped";
  } else if (cmd == "status") { // 返回当前小车状态
    snprintf(buffer, sizeof(buffer), "status: %s:%u/%u", GET_CAR_GEAR_STR(&car), 
        /*GET_CURRENT_SPEED(&car)*/car.speed_left, car.speed_right);
    return buffer;
  } else {
    return "error: unknown command '" + cmd + "'";
  }
}

#ifdef ENABLE_LCD1602
void lcd_print_long_str(const char *str) {
  lcd.clear();
  // 第1行：最多 16 个字符
  lcd.setCursor(0, 0);
  for(int i=0; i<16 && str[i]!='\0'; i++) {
    lcd.write(str[i]);
  }
  // 第2行：从第16个字符开始
  lcd.setCursor(0, 1);
  for(int i=16; i<32 && str[i]!='\0'; i++) {
    lcd.write(str[i]);
  }
}
#endif