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

float MQ2_readings(int mv) {
	int ppm = mv * (10000 - 300) / 1024;
	return ppm;
}

float MQ7_readings(int mv) {
	int ppm = mv * (2000 - 20) / 1024;
	return ppm;
}

float MQ136_readings(int mv) {
	float ppm = mv * (100.0 - 1.0) / 1024.0;
	return ppm;
}

float MQ137_readings(int mv) {
	float ppm = mv * (500.0 - 5.0) / 1024.0;
	return ppm;
}
