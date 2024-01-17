#include <zephyr/drivers/pwm.h>
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
