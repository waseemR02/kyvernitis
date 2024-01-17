/*
* Kyervnitis helper macros and function declarations
*/
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 3
#define MAX_SERVOS 5

#define PWM_MOTOR_STOP 1520000
#define SERVO_DEFAULT_STATE 1520000

#define STEPPER_MOTOR_FORWARD 1
#define STEPPER_MOTOR_BACKWARD 2

struct pwm_motor {
	const struct pwm_dt_spec dev_spec;
	const uint32_t min_pulse;
	const uint32_t max_pulse;
};

struct stepper_motor {
	const struct gpio_dt_spec dir;
	const struct gpio_dt_spec step;
};

typedef uint32_t servo_state_t;

// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width);

int stepper_motor_write(const struct stepper_motor *motor, uint8_t cmd);

float MQ2_readings(int mv);

float MQ7_readings(int mv);

float MQ136_readings(int mv);

float MQ137_readings(int mv);
