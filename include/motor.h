#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "global.h"

//L298N控制线引脚编号（可设置值：A：用于控制小车右轮，B：左轮）
#define MOTOR_A_IN1_PIN 4
#define MOTOR_A_IN2_PIN 2
#define MOTOR_A_PWM_PIN 12
#define MOTOR_B_IN3_PIN 5
#define MOTOR_B_IN4_PIN 18
#define MOTOR_B_PWM_PIN 14
//END

// PWM参数
#define MOTOR_A_PWM_CHANNEL 0
#define MOTOR_B_PWM_CHANNEL 1
#define MOTOR_PWM_FREQUENCE 1000 // 1kHz
#define MOTOR_PWM_RESOLUTION 8   // 8位 (0~255)

extern uint16_t motor_max_resolution_val;

#define MOTOR_MIN_SPEED 0
#define MOTOR_MAX_SPEED motor_max_resolution_val //将MOTOR_MAX_SPEED/2=127定为基准普通速度
#define MOTOR_STANDARD_SPEED (MOTOR_MAX_SPEED/2)
#define MOTOR_SAFE_PWM_SPEED 90 //PWM 给小了、电机只是嗡嗡响但不转。低于该值，很可能无法带动电机，导致电机堵转
//即便是基准速度下，转弯时由于一侧PWM置为0，所以驱动力减半，遇到地面不平也会出现堵转情况
//或者干脆不允许client设置低于该值的PWM速度，并返回错误
/*如果一定要以较低速行驶，应在启动时加以助推：
譬如设置速度 = 40
 ↓
低于 MOTOR_SAFE_PWM_SPEED → 自动进入助推
 ↓
0 ~ 120ms：输出 200 PWM（轮子立刻转起来）
 ↓
120ms 后：自动变回 40 PWM（保持低速走）
*/

typedef struct smart_car_ {
    uint16_t speed_left; //速度：0~255
    uint16_t speed_right;
#define MOTOR_DRIVE   0
#define MOTOR_REVERSE 1
#define MOTOR_D_PARK  2 //前进档切换到的驻车档
#define MOTOR_R_PARK  3 //倒车档切换到的驻车档
    uint8_t gear;       //档位：前进挡、倒车档、驻车档
} smart_car_t;

#define GET_CAR_GEAR_STR(car) ((car)->gear == MOTOR_DRIVE) ? "DRIVE" : \
    (((car)->gear == MOTOR_REVERSE) ? "REVERSE" : \
    (((car)->gear == MOTOR_D_PARK) ? "D_PARK" : "R_PARK"))

#define GET_CURRENT_SPEED(car) (((car)->speed_left == (car)->speed_right) ? \
    (car)->speed_left : (((car)->speed_left < (car)->speed_right) ? \
    (car)->speed_right : (car)->speed_left))

#define IS_TURNING(car) ((car)->speed_left != (car)->speed_right)
#define IS_TURNING_LEFT(car) (((car)->speed_left != (car)->speed_right) \
    && ((car)->speed_left < (car)->speed_right))
#define IS_TURNING_RIGHT(car) (((car)->speed_left != (car)->speed_right) \
    && ((car)->speed_left > (car)->speed_right))

extern smart_car_t car;

#define MOTOR_SET_DRIVE_GEAR(car, force) \
do { \
    if (((car)->gear != MOTOR_DRIVE) || force) { \
        digitalWrite(MOTOR_A_IN1_PIN, LOW); \
        digitalWrite(MOTOR_A_IN2_PIN, HIGH); \
        digitalWrite(MOTOR_B_IN3_PIN, LOW); \
        digitalWrite(MOTOR_B_IN4_PIN, HIGH); \
        (car)->gear = MOTOR_DRIVE; \
    } \
} while(0)

#define MOTOR_SET_REVERSE_GEAR(car, force) \
do { \
    if (((car)->gear != MOTOR_REVERSE) || force) { \
        digitalWrite(MOTOR_A_IN1_PIN, HIGH); \
        digitalWrite(MOTOR_A_IN2_PIN, LOW); \
        digitalWrite(MOTOR_B_IN3_PIN, HIGH); \
        digitalWrite(MOTOR_B_IN4_PIN, LOW); \
        (car)->gear = MOTOR_REVERSE; \
    } \
} while(0)

#define MOTOR_SET_PARK_GEAR(car) \
do { \
    if (((car)->gear != MOTOR_D_PARK) && ((car)->gear != MOTOR_R_PARK)) { \
        digitalWrite(MOTOR_A_IN1_PIN, LOW); \
        digitalWrite(MOTOR_A_IN2_PIN, LOW); \
        digitalWrite(MOTOR_B_IN3_PIN, LOW); \
        digitalWrite(MOTOR_B_IN4_PIN, LOW); \
        if ((car)->gear == MOTOR_DRIVE) { \
            (car)->gear = MOTOR_D_PARK; \
        } else { \
            (car)->gear = MOTOR_R_PARK; \
        } \
    } \
} while(0)

extern void motor_move_with_speed(smart_car_t *car, uint16_t speed);
#define MOTOR_MOVE_WITH_SPEED motor_move_with_speed

#define MOTOR_MOVE_STOP(car) MOTOR_MOVE_WITH_SPEED(car, MOTOR_MIN_SPEED)
#define MOTOR_MOVE_FULL_SPEED(car) MOTOR_MOVE_WITH_SPEED(car, MOTOR_MAX_SPEED)

#define MOTOR_MOVE_LEFT(car) \
do { \
    if ((car)->speed_right > 0) { \
        if ((car)->speed_left > 0) { \
            (car)->speed_left = 0; \
        } \
        ledcWrite(MOTOR_B_PWM_CHANNEL, (car)->speed_left); \
    } \
} while(0)

#define MOTOR_MOVE_RIGHT(car) \
do { \
    if ((car)->speed_left > 0) { \
        if ((car)->speed_right > 0) { \
            (car)->speed_right = 0; \
        } \
        ledcWrite(MOTOR_A_PWM_CHANNEL, (car)->speed_right); \
    } \
} while(0)

extern void init_car_motor();

#endif