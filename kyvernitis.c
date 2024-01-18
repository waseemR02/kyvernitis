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

/*
 * Writes motor command to dc motors with limit switches
 */
int dc_motor_write_lim(const struct dc_motor *motor, uint8_t motor_cmd, const struct gpio_dt_spec *lim)
{
	int ret = 1;
	
	if(!gpio_pin_get_dt(lim)) {
		ret = dc_motor_write(motor, motor_cmd);
	}
	else {
		ret = dc_motor_write(motor, DC_MOTOR_STOP);
	}
	

	return ret;
}


float MQ2_readings(int adc_reading) {
	int ppm = adc_reading * (10000 - 300) / 4096;
	return ppm;
}

float MQ7_readings(int adc_reading) {
	int ppm = adc_reading * (2000 - 20) / 4096;
	return ppm;
}

float MQ136_readings(int adc_reading) {
	float ppm = adc_reading * (100.0 - 1.0) / 4096.0;
	return ppm;
}

float MQ137_readings(int adc_reading) {
	float ppm = adc_reading * (500.0 - 5.0) / 4096.0;
	return ppm;
}
