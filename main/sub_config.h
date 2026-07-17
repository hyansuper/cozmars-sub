#pragma once

#include "hal/gpio_types.h"


/* I2C pin configuration (external 4.7K pullup resistors on board) */
#define I2C_SLAVE_SDA_IO               GPIO_NUM_2
#define I2C_SLAVE_SCL_IO               GPIO_NUM_5

/* reuse i2c_scl pin for deep sleep wakeup. */
#define DSL_WAKE_UP_IO            I2C_SLAVE_SCL_IO

/* WLED pin */
#define WLED_GPIO                      GPIO_NUM_21
#define WLED_FADE_STEP_MS              (50)

/* Motor pins */
#define LEFT_MOTOR_A_GPIO              GPIO_NUM_6
#define LEFT_MOTOR_B_GPIO              GPIO_NUM_7
#define RIGHT_MOTOR_A_GPIO             GPIO_NUM_3
#define RIGHT_MOTOR_B_GPIO             GPIO_NUM_4
#define MOTOR_PWM_TIMER                LEDC_TIMER_0
#define MOTOR_ACCEL_THROTTLE_PER_SEC_DEFAULT    (50) /* %/s */

/* Motor reflective IR sensor — mm traveled per edge */
#define MOTOR_IR_MM_PER_EDGE           (5)

#define MOTOR_UPDATE_STEP_MS           (50)

/* Servo pins */
#define SERVO_PWM_TIMER                LEDC_TIMER_1
#define HEAD_SERVO_GPIO                GPIO_NUM_1
#define LIFT_SERVO_GPIO                GPIO_NUM_20

#define SERVO_UPDATE_STEP_MS           (50)
#define SERVO_HOLD_MS                  (200)/* Servo hold time before turning off */


/* 74HC165 parallel-in shift register */
#define HC165_PL_GPIO                  GPIO_NUM_9
#define HC165_CP_GPIO                  GPIO_NUM_8
#define HC165_Q7_GPIO                  GPIO_NUM_10

/* State reader */
#define STATE_MONITOR_INTERVAL_MS       (20)

/* Battery */
#define BATTERY_ADC_GPIO               GPIO_NUM_0
#define BATTERY_ADC_CHANNEL            0
#define BATTERY_SAMPLE_INTERVAL_MS     (3000)





/* Task priorities (higher number = higher priority) */
#define WLED_TASK_PRIORITY             (1)
#define I2C_RESPONSE_TASK_PRIORITY     (5)
#define MOTOR_TASK_PRIORITY            (10)
#define SERVO_TASK_PRIORITY            (10)
#define STATE_MONITOR_TASK_PRIORITY     (15)


