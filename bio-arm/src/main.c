/*
 * Source file for Bio Arm Application
 */


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <app_version.h>

#include <kycan.h>
#include <kyvernitis.h>

#define ROBOCLAWS_COUNT 2
#define SERVOS_COUNT 2

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* DT spec for adc channels */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* DT spec for roboclaws*/
struct pwm_motor roboclaw[ROBOCLAWS_COUNT] = {
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor1)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_motor1), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_motor1), max_pulse)
	},
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor2)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_motor2), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_motor2), max_pulse)
	}
};

/* DT spec for servos*/
struct pwm_motor servo[SERVOS_COUNT] = {
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo1)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_servo1), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_servo1), max_pulse)
	},
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo2)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_servo2), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_servo2), max_pulse)
	}
};

/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/*
 * CAN frames and filter for Bio Arm
 */
const struct can_filter bio_arm_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = BIO_ARM_ID,
	.mask = CAN_EXT_ID_MASK
};

struct can_frame bio_arm_rx_frame = {
	.flags = CAN_FRAME_IDE,
	.id = BIO_ARM_ID,
	/* dlc = 4(one uint32 to account for largest pwm pulse width) 
			+ 1(one uint8 to address different motor) */
};

struct can_frame bio_arm_tx_frame = {
	.flags = CAN_FRAME_IDE,
	.id = BIO_ARM_ID,
	//.dlc = 4 // TODO: Decide on the sensor format
};


/*
 * Transfer thread
 * 
 */
void tx_thread()
{
	return;
}

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


int main(void)
{
	printk("Bio-arm: v%s", APP_VERSION_STRING);
	
	int err;
	uint32_t count = 0;
	uint16_t buf;
	uint32_t pulse = 15200000;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	ARG_UNUSED(count);
	ARG_UNUSED(sequence);
	ARG_UNUSED(pulse);
	/* Device ready checks*/

	if (!device_is_ready(can_dev)) {
		LOG_INF("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			LOG_ERR("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}
	
	// check for pwm motor readiness
	for (size_t i = 0U; i < ARRAY_SIZE(roboclaw); i++) {
		if (!pwm_is_ready_dt(&(roboclaw[i].dev_spec))) {
			LOG_ERR("PWM: Roboclaw %s is not ready\n", roboclaw[i].dev_spec.dev->name);
			return 0;
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(servo); i++) {
		if (!pwm_is_ready_dt(&(servo[i].dev_spec))) {
			LOG_ERR("PWM: Servo %s is not ready\n", servo[i].dev_spec.dev->name);
			return 0;
		}
	}

	if (!gpio_is_ready_dt(&led))
	{
		LOG_ERR("Error: Led not ready\n");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0)
	{
		LOG_ERR("Error: Led not configured\n");
		return 0;
	}

	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &bio_arm_filter);
	if (filter_id < 0)
	{
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
	}

	while (true)
	{
		k_msgq_get(&rx_msgq, &bio_arm_rx_frame, K_FOREVER);

		struct can_frame frame = bio_arm_rx_frame;

		if(frame.dlc != 5)
		{
			//just handling motor commands for now
			LOG_ERR("Unknown Frame Received\n");
			continue;
		}
		if (frame.data[4] < ROBOCLAW_BASE_ID + ROBOCLAWS_COUNT)
		{
			// in the case it will only consider from 10 - 11
			pwm_motor_write(&roboclaw[frame.data[4] - ROBOCLAW_BASE_ID], frame.data_32[0]);
		}
		else if (frame.data[4] < SERVO_BASE_ID + SERVOS_COUNT)
		{
			// it will consider from 15 - 19
			pwm_motor_write(&servo[frame.data[4] - SERVO_BASE_ID], frame.data_32[0]);
		}
		else continue;
	}
	
	return 0;
}
