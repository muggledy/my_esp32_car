#include "motor.h"

uint16_t motor_max_resolution_val = pow(2, MOTOR_PWM_RESOLUTION) - 1; // [0, max_resolution_val] is valid

smart_car_t car = {
    .direction = 0,
    .speed_left = 255,
    .speed_right = 255,
    .gear = 255
};

//L298N直流电机驱动模块初始化
void init_motor_pin_mode() {
    pinMode(MOTOR_A_IN1_PIN, OUTPUT);
    pinMode(MOTOR_A_IN2_PIN, OUTPUT);
    pinMode(MOTOR_B_IN3_PIN, OUTPUT);
    pinMode(MOTOR_B_IN4_PIN, OUTPUT);

    // 配置PWM
    ledcSetup(MOTOR_A_PWM_CHANNEL, MOTOR_PWM_FREQUENCE, MOTOR_PWM_RESOLUTION);
    ledcAttachPin(MOTOR_A_PWM_PIN, MOTOR_A_PWM_CHANNEL);
    ledcSetup(MOTOR_B_PWM_CHANNEL, MOTOR_PWM_FREQUENCE, MOTOR_PWM_RESOLUTION);
    ledcAttachPin(MOTOR_B_PWM_PIN, MOTOR_B_PWM_CHANNEL);

    // 初始化档位：前进挡
    MOTOR_SET_DRIVE_GEAR(&car);
    // 初始化速度：0（停止状态）
    MOTOR_MOVE_STOP(&car);
}

void motor_move_with_speed(smart_car_t *car, uint16_t speed) {
    if (!car) {
        return;
    }
    if (speed > MOTOR_MAX_SPEED) {
        speed = MOTOR_MAX_SPEED;
    }
    if ((speed != car->speed_left) || speed != car->speed_right) {
        if (speed == 0) {
            MOTOR_SET_PARK_GEAR(car);
        } else {
            if (car->gear == MOTOR_D_PARK) {
                MOTOR_SET_DRIVE_GEAR(car);
            } else if (car->gear == MOTOR_R_PARK) {
                MOTOR_SET_REVERSE_GEAR(car);
            }
        }
        ledcWrite(MOTOR_A_PWM_CHANNEL, speed);
        ledcWrite(MOTOR_B_PWM_CHANNEL, speed);
        car->speed_left = car->speed_right = speed;
    }
}