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
1. 小车引脚连接（或参见“接线示意图.drawio”）：
  1）motor.h中标注了“L298N控制线引脚编号”设置
  2）lcd1602的SDA接单片机21引脚、SCL接单片机22引脚
  3）HC-SR04的Trigger/Echo引脚设置见global.h
2. WiFi账号设置：见wifi.cpp
*/

extern bool connect_wifi();
extern void on_web_socket_event(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
extern String handle_command(String cmd);
#ifdef ENABLE_LCD1602
extern void lcd_print_long_str(const char *str);
#endif
#ifdef ENABLE_HCSR04
extern void IRAM_ATTR echoISR();
extern void sendTrigger();
extern void initHCSR04();
extern void handleCurrentDistance();
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

//for HC-SR04
#ifdef ENABLE_HCSR04
volatile unsigned long echoStart = 0; // 中断服务程序中的相关变量需要加volatile修饰
volatile unsigned long echoDuration = 0;
volatile bool newDistanceAvailable = false;
float distanceCm = 0;
#endif

// uint8_t lcd_buffer_len = 0;

void setup() {
  init_car_motor();
#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
#endif
  pinMode(ONBOARD_LED_PIN, OUTPUT);
#ifdef ENABLE_LCD1602
  lcd.init();
  lcd.backlight();
  lcd.print("I'm smart car.");
#endif
  connect_wifi();
#ifdef ENABLE_HCSR04
  initHCSR04();
#endif

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
  static unsigned long lastTriggerTime = 0;

  if (TST_FLAG(global_status_word, G_STATE_WIFI_CONNECTED)) {
    web_socket->loop(); // 处理WebSocket事件（接收消息、发送响应、管理连接）

#ifdef ENABLE_HCSR04
    // 触发超声波距离探测动作
    if (millis() - lastTriggerTime >= HCSR04_TRIGGER_INTERVAL) {
      lastTriggerTime = millis();
      sendTrigger();
    }
    handleCurrentDistance();
#endif
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
          snprintf(buffer, sizeof(buffer), "cmd:%s", cmd.c_str());
          lcd.clear();
          lcd.print(buffer);
#endif
          // 解析并执行指令
          String response = handle_command(cmd);
          // 发送响应
          web_socket->sendTXT(num, response);
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[websocket] %s\n", response);
#endif
#ifdef ENABLE_LCD1602
          /*lcd_buffer_len = */snprintf(buffer, sizeof(buffer), "%s:%u/%u/%u", GET_CAR_GEAR_STR(&car), 
              car.speed_left, car.speed_right, global_car_speed);
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
      MOTOR_SET_DRIVE_GEAR(&car, 0);
    }
    if ((GET_CURRENT_SPEED(&car) == 0) || (GET_CURRENT_SPEED(&car) != global_car_speed) 
        || IS_TURNING(&car)) {
      MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
    }
    if (distanceCm < HCSR04_SAFE_DISTANCE) { //检查前方是否还有安全距离可以行驶，没有则强制停止
      MOTOR_MOVE_STOP(&car);
    }
    return "ok: moving forward";
  }
  else if (cmd == "backward") {
    if (car.gear != MOTOR_REVERSE) {
      MOTOR_SET_REVERSE_GEAR(&car, 0);
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
  } else if (cmd.startsWith("chgspeed:")) { // 修改小车速度
    // 解析速度值
    int newSpeed = cmd.substring(9).toInt();
    if (newSpeed >= MOTOR_MIN_SPEED && newSpeed <= MOTOR_MAX_SPEED) {
      global_car_speed = newSpeed;
      // 如果小车正在运动，立即应用新速度
      if ((GET_CURRENT_SPEED(&car) > 0) && (GET_CURRENT_SPEED(&car) != global_car_speed)) {
        if (IS_TURNING_LEFT(&car)) { //正在左转
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] change speed from %u to %u when turning left\n", 
              GET_CURRENT_SPEED(&car), global_car_speed);
#endif
          MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
          MOTOR_MOVE_LEFT(&car);
        } else if (IS_TURNING_RIGHT(&car)) { //正在右转
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] change speed from %u to %u when turning right\n", 
              GET_CURRENT_SPEED(&car), global_car_speed);
#endif
          MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
          MOTOR_MOVE_RIGHT(&car);
        } else { //正在前进或倒退
#ifdef ENABLE_SERIAL_DEBUG
          Serial.printf("[WebSocket] change speed from %u to %u when %s\n", 
              GET_CURRENT_SPEED(&car), global_car_speed, GET_CAR_GEAR_STR(&car));
#endif
          MOTOR_MOVE_WITH_SPEED(&car, global_car_speed);
        }
      }
      snprintf(buffer, sizeof(buffer), "ok: speed changed to %d", global_car_speed);
      return buffer;
    } else {
      return "error: speed must be 0-" + String(MOTOR_MAX_SPEED);
    }
  } else if (cmd == "stop") {
    MOTOR_MOVE_STOP(&car);
    return "ok: stopped";
  } else if (cmd == "status") { // 返回当前小车状态
    snprintf(buffer, sizeof(buffer), "status: %s:%u/%u/%u", GET_CAR_GEAR_STR(&car), 
        /*GET_CURRENT_SPEED(&car)*/car.speed_left, car.speed_right, global_car_speed);
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

#ifdef ENABLE_HCSR04
void initHCSR04() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT_PULLUP);

  // 给 Echo 绑定中断
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), echoISR, CHANGE);

  // 先触发一次超声波探测动作
  sendTrigger();
}

void IRAM_ATTR echoISR() {
  if (digitalRead(ECHO_PIN) == HIGH) {
    // 上升沿：开始计时
    echoStart = micros();
  } else {
    // 下降沿：结束计时
    echoDuration = micros() - echoStart;
    newDistanceAvailable = true;
  }
}

void sendTrigger() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

void handleCurrentDistance() {
  // 有新探测到的距离时才处理
  if (newDistanceAvailable) {
    newDistanceAvailable = false;

    distanceCm = echoDuration / 58.0;

    if (distanceCm >= HCSR04_MIN_DISTANCE && distanceCm <= HCSR04_MAX_DISTANCE) { // 有效测距
      if (distanceCm > HCSR04_DISTANCE_BIAS) {
        distanceCm -= HCSR04_DISTANCE_BIAS;
      }
      if ((car.gear == MOTOR_DRIVE) && (distanceCm < HCSR04_SAFE_DISTANCE)) { //自动停止，防止撞击前方阻碍物体
        MOTOR_MOVE_STOP(&car);
      }
#ifdef ENABLE_SERIAL_DEBUG
      Serial.print("Distance: ");
      Serial.print(distanceCm);
      Serial.println(" cm");
#endif
#ifdef ENABLE_LCD1602
      if (TST_FLAG(global_status_word, G_STATE_WS_CONNECTED)) {
        lcd.setCursor(0, 0);
        lcd.print("   ");
        lcd.setCursor(0, 0);
        lcd.print(int(distanceCm));
      }
#endif
    } else {
      if (distanceCm > HCSR04_MAX_DISTANCE) {
        distanceCm = 0;
      }
#ifdef ENABLE_SERIAL_DEBUG
      Serial.println("Distance: Out of range(2~400)");
#endif
    }
  }
}
#endif