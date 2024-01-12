/*
* Kyervnitis helper macros and function declarations
*/
#include <zephyr/drivers/gpio.h>

#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 2
#define MAX_SERVOS 5

#define DC_MOTOR_FORWARD 1
#define DC_MOTOR_BACKWARD 2
#define DC_MOTOR_STOP 0

struct dc_motor {
	const struct gpio_dt_spec input_1;
	const struct gpio_dt_spec input_2;
};

int dc_motor_write(const struct dc_motor *motor, uint8_t motor_cmd);
