#include <kyvernitis.h>

/*
 * Wrapper around gpio_pin_set_dt to abstract away motor commands
 *
 * Ret: 0 on Success.
 * Ret: 1 on Failure
 */ 
int dc_motor_write(const struct dc_motor *motor, uint8_t motor_cmd)
{
	switch (motor_cmd){
	
	case DC_MOTOR_FORWARD:
		if (gpio_pin_set_dt(&(motor->input_1), 1))
			return 1;
		if (gpio_pin_set_dt(&(motor->input_2), 0))
			return 1;
		break;

	case DC_MOTOR_BACKWARD:
		if (gpio_pin_set_dt(&(motor->input_1), 0))
			return 1;
		if (gpio_pin_set_dt(&(motor->input_2), 1))
			return 1;
		break;

	case DC_MOTOR_STOP:
		if (gpio_pin_set_dt(&(motor->input_1), 0))
			return 1;
		if (gpio_pin_set_dt(&(motor->input_2), 0))
			return 1;
		break;
	}

	return 0;
}
