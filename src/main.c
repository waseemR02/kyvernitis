
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const uint8_t pwm_motor_count = 4;

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

struct pwm_motor {
	const struct pwm_dt_spec dev_spec;
	const uint32_t min_pulse;
	const uint32_t max_pulse;
};

static const struct pwm_motor roboclaw_1 = {
	
	.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor1)),
	.min_pulse = DT_PROP(DT_ALIAS(pwm_motor1), min_pulse),
	.max_pulse = DT_PROP(DT_ALIAS(pwm_motor1), max_pulse)
};

static const struct pwm_motor roboclaw_2 = {
	.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor2)),
	.min_pulse = DT_PROP(DT_ALIAS(pwm_motor2), min_pulse),
	.max_pulse = DT_PROP(DT_ALIAS(pwm_motor2), max_pulse)
};

static const struct pwm_motor servo_1 = {
	.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo1)),
	.min_pulse = DT_PROP(DT_ALIAS(pwm_servo1), min_pulse),
	.max_pulse = DT_PROP(DT_ALIAS(pwm_servo1), max_pulse)
};

static const struct pwm_motor servo_2 = {
	.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo2)),
	.min_pulse = DT_PROP(DT_ALIAS(pwm_servo2), min_pulse),
	.max_pulse = DT_PROP(DT_ALIAS(pwm_servo2), max_pulse)
};


// Wrapper around pwm_set_pulse_dt to ensure that pulse_width
// remains under max-min range
static inline int pwm_motor_write(const struct pwm_motor *motor, uint32_t pulse_width)
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

// returns the number of ready pwm motors
static inline int pwm_motors_ready()
{
	uint8_t device_count = 0;
	if (!pwm_is_ready_dt(&(roboclaw_1.dev_spec))){
		printk("Error: PWM device %s is not ready\n", roboclaw_1.dev_spec.dev->name);
		device_count += 1;
	}

	if (!pwm_is_ready_dt(&(roboclaw_2.dev_spec))){
		printk("Error: PWM device %s is not ready\n", roboclaw_2.dev_spec.dev->name);
		device_count += 1;
	}

	if (!pwm_is_ready_dt(&(servo_1.dev_spec))){
		printk("Error: PWM device %s is not ready\n", servo_1.dev_spec.dev->name);
		device_count += 1;
	}

	if (!pwm_is_ready_dt(&(servo_2.dev_spec))){
		printk("Error: PWM device %s is not ready\n", servo_2.dev_spec.dev->name);
		device_count += 1;
	}

	return device_count;
}

enum direction {
	DOWN,
	UP,
};

int main(void)
{
	int err;
	uint32_t count = 0;
	uint16_t buf;
	uint32_t pulse = 1500;
	enum direction dir = UP;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	
	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}
	
	if (pwm_motors_ready() == pwm_motor_count)
	{
		printk("Error: All instances of PWM motors are not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&led))
	{
		printk("Error: Led not ready\n");
		return 0;
	}
	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0)
	{
		printk("Error: Led not configured\n");
		return 0;
	}
	while (1) {
		printk("ADC reading[%u]:\n", count++);
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			printk("- %s, channel %d: ",
			       adc_channels[i].dev->name,
			       adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read_dt(&adc_channels[i], &sequence);
			if (err < 0) {
				printk("Could not read (%d)\n", err);
				continue;
			}

			/*
			 * If using differential mode, the 16 bit value
			 * in the ADC sample buffer should be a signed 2's
			 * complement value.
			 */
			if (adc_channels[i].channel_cfg.differential) {
				val_mv = (int32_t)((int16_t)buf);
			} else {
				val_mv = (int32_t)buf;
			}
			printk("%"PRId32, val_mv);
			err = adc_raw_to_millivolts_dt(&adc_channels[i],
						       &val_mv);
			/* conversion to mV may not be supported, skip if not */
			if (err < 0) {
				printk(" (value in mV not available)\n");
			} else {
				printk(" = %"PRId32" mV\n", val_mv);
			}
		}
		while (dir == UP)
		{
			err = pwm_motor_write(&roboclaw_1, pulse);
			err += pwm_motor_write(&roboclaw_2, pulse);
			err += pwm_motor_write(&servo_1, pulse);
			err += pwm_motor_write(&servo_2, pulse);
			if (err < 0)
			{
				printk("Error failed to set pulse width\n");
				return 0;
			}

			pulse += 50;

			if (pulse >= 2000)
				dir = DOWN;
			gpio_pin_toggle_dt(&led);
			k_sleep(K_SECONDS(1));
		}
		while (dir == DOWN)
		{
			err = pwm_motor_write(&roboclaw_1, pulse);
			err += pwm_motor_write(&roboclaw_2, pulse);
			err += pwm_motor_write(&servo_1, pulse);
			err += pwm_motor_write(&servo_2, pulse);

			if (err < 0)
			{
				printk("Error failed to set pulse width\n");
				return 0;
			}

			pulse -= 50;

			if (pulse <= 1000)
				dir = UP;
			gpio_pin_toggle_dt(&led);
			k_sleep(K_SECONDS(1));
		}
	}
	return 0;
}
