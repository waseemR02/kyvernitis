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
