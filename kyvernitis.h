/*
* Kyervnitis helper macros and function declarations
*/
#include <zephyr/drivers/pwm.h>

#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 3
#define MAX_SERVOS 5

struct pwm_motor {
	const struct pwm_dt_spec dev_spec;
	const uint32_t min_pulse;
	const uint32_t max_pulse;
};

// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width);

float MQ2_readings(int mv);

float MQ7_readings(int mv);

float MQ136_readings(int mv);

float MQ137_readings(int mv);
