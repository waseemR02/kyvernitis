/*
* Kyervnitis helper macros and function declarations
*/
#include <zephyr/drivers/pwm.h>

#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 3
#define MAX_SERVOS 5

#define PWM_MOTOR_STOP 1520000
#define SERVO_DEFAULT_STATE 1520000

struct pwm_motor {
	const struct pwm_dt_spec dev_spec;
	const uint32_t min_pulse;
	const uint32_t max_pulse;
};

typedef uint32_t servo_state_t;

// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width);

float MQ2_readings(int mv);

float MQ7_readings(int mv);

float MQ136_readings(int mv);

float MQ137_readings(int mv);
