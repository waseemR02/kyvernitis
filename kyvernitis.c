#include <kyvernitis.h>

// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width)
{
	// wrapper around pwm_set_pulse_dt to ensure that pulse_width 
	// remains under max-min range
	if (pulse_width <= motor->min_pulse)
		pulse_width = motor->min_pulse;
	if (pulse_width >= motor->max_pulse)
		pulse_width = motor->max_pulse;
	
	int ret = pwm_set_pulse_dt(&(motor->dev_spec), pulse_width);
	return ret;
}

/*
 * Wrapper around gpio_pin_set_dt to abstract away stepper motor commands
 * Moves the motor by one step in the commanded direction
 *
 * Ret: 0 on Success.
 * Ret: other then 0 on Failure
 */
int stepper_motor_write(const struct stepper_motor *motor, uint8_t cmd)
{
	int ret = 0;
	switch (cmd) {
	
	case STEPPER_MOTOR_FORWARD:
		// set dir
		ret += gpio_pin_set_dt(&(motor->dir), 1);

		// One step
		ret += gpio_pin_set_dt(&(motor->step), 1);
		k_sleep(K_USEC(500));
		ret += gpio_pin_set_dt(&(motor->step), 0);
		k_sleep(K_USEC(500));
		
		break;

	case STEPPER_MOTOR_BACKWARD:
		// set dir
		ret += gpio_pin_set_dt(&(motor->dir), 0);

		// One step
		ret += gpio_pin_set_dt(&(motor->step), 1);
		k_sleep(K_USEC(500));
		ret += gpio_pin_set_dt(&(motor->step), 0);
		k_sleep(K_USEC(500));
		
		break;
	}

	return ret;
}
