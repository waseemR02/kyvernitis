/*
* Kyervnitis helper macros and function declarations
*/

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>


#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 2
#define MAX_SERVOS 5

#define PWM_MOTOR_STOP 1520000
#define SERVO_DEFAULT_STATE 1520000

#define DC_MOTOR_FORWARD 1
#define DC_MOTOR_BACKWARD 2
#define DC_MOTOR_STOP 0

struct pwm_motor {
	const struct pwm_dt_spec dev_spec;
	const uint32_t min_pulse;
	const uint32_t max_pulse;
};

struct dc_motor {
	const struct gpio_dt_spec input_1;
	const struct gpio_dt_spec input_2;
};

typedef uint32_t servo_state_t;

// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width);

int dc_motor_write(const struct dc_motor *motor, uint8_t motor_cmd);

int dc_motor_write_lim(const struct dc_motor *motor, uint8_t motor_cmd, const struct gpio_dt_spec *lim);

float MQ2_readings(int adc_reading);

float MQ7_readings(int adc_reading);

float MQ136_readings(int adc_reading);

float MQ137_readings(int adc_reading);