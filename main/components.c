#include "components.h"
#include "sub_config.h"
#include "wled.h"
#include "motor.h"
#include "servo.h"
#include "sub_state.h"

wled_t wled;
motor_pair_t motors;
servo_t head_servo, lift_servo;

void components_init() {

	servo_init(&head_servo, &(servo_init_arg_t){
		.gpio = HEAD_SERVO_GPIO,
		.chan = LEDC_CHANNEL_4,
		.value_start = HEAD_ANGLE_MIN,
		.value_end = HEAD_ANGLE_MAX,
		.cfg = { // choose a safe default range
			.pwm_start = 410,
			.pwm_end = 2048,
		},
		.idle_mask = IDLE_HEAD_MASK
	});

	servo_init(&lift_servo, &(servo_init_arg_t){
		.gpio = LIFT_SERVO_GPIO,
		.chan = LEDC_CHANNEL_5,
		.value_start = 0,
		.value_end = 100,
		.cfg = { // choose a safe default range
			.pwm_start = 410,
			.pwm_end = 2048,
		},
		.idle_mask = IDLE_LIFT_MASK
	});

	motor_pair_init_arg_t motor_args = {
		.left = {
			.gpio_a = LEFT_MOTOR_A_GPIO,
			.gpio_b = LEFT_MOTOR_B_GPIO,
			.ch_a = LEDC_CHANNEL_0,
			.ch_b = LEDC_CHANNEL_1,
		},
		.right = {
			.gpio_a = RIGHT_MOTOR_A_GPIO,
			.gpio_b = RIGHT_MOTOR_B_GPIO,
			.ch_a = LEDC_CHANNEL_2,
			.ch_b = LEDC_CHANNEL_3,
		},
	};
	motors_init(&motors, &motor_args);

	wled_init(&wled, &(wled_init_arg_t){
		.gpio = WLED_GPIO
	});

	
}