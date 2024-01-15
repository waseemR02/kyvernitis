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
#define TX_THREAD_STACK_SIZE 512
#define TX_THREAD_PRIORITY 2

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);
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

struct can_frame bio_arm_rx_frame;

struct can_frame bio_arm_tx_frame = {
	.flags = CAN_FRAME_IDE,
	.id = LATTEPANDA_ID,
	//.dlc = 4 // TODO: Decide on the sensor format
};


/*
 * Transfer thread
 * 
 */

struct k_thread tx_thread_data;

void tx_thread(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	int err;
	float val;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};


	while (1) {
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read_dt(&adc_channels[i], &sequence);
			if (err < 0) {
				LOG_ERR("Could not read (%d)\n", err);
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
			err = adc_raw_to_millivolts_dt(&adc_channels[i],
						       &val_mv);
			/* conversion to mV may not be supported, skip if not */
			if (err < 0) {
				LOG_ERR(" (value in mV not available)\n");
			} else {
				if (i == 0) {
                                        val = MQ136_readings(val_mv);
                                } else if (i == 1) {
                                        val = MQ2_readings(val_mv);
                                } else if (i == 2) {
                                        continue; // DHT11_function not defined yet !
                                } else if (i == 8) {
                                        val = MQ7_readings(val_mv);
                                } else {
                                        LOG_INF("NOT IN USE\t");
				}
				LOG_INF("Channel:%d = %"PRId32" mV\n", i, (int32_t)val);
				bio_arm_tx_frame.dlc = 6;
				bio_arm_tx_frame.data_32[0] = (uint32_t)val;
				bio_arm_tx_frame.data[5] = (uint8_t)i;
				bio_arm_tx_frame.data[4] = SENSOR_DATA_ID;
			}
			can_send(can_dev, &bio_arm_tx_frame, K_MSEC(100), NULL, NULL);
			LOG_INF("CAN frame sent: ID: %d", bio_arm_tx_frame.id);
			LOG_INF("CAN frame data: %d %d %d", bio_arm_tx_frame.data_32[0],
								bio_arm_tx_frame.data[4],
								bio_arm_tx_frame.data[5]);
			k_sleep(K_SECONDS(1));
			gpio_pin_toggle_dt(&led);
		}
	}

	return;
}




int main(void)
{
	printk("\nBio-arm: v%s\n\n", APP_VERSION_STRING);

	int err;
	k_tid_t tx_tid;
	uint32_t pulse = 15200000;


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
	
#ifdef CONFIG_LOOPBACK_MODE
	if (can_set_mode(can_dev, CAN_MODE_LOOPBACK)) {
		LOG_ERR("Error setting CAN mode");
		return 0;
	}
#endif
	if (can_start(can_dev)) {
		LOG_ERR("Error starting CAN controller.\n");
		return 0;
	}

	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &bio_arm_filter);
	if (filter_id < 0)
	{
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
	}

	tx_tid = k_thread_create(&tx_thread_data, tx_thread_stack,
				 K_THREAD_STACK_SIZEOF(tx_thread_stack),
				 tx_thread, NULL, NULL, NULL,
				 TX_THREAD_PRIORITY, 0, K_NO_WAIT);

	if (!tx_tid) {
		LOG_ERR("ERROR spawning tx thread\n");
	}

	LOG_INF("Initialization completed successfully\n");

	while (true)
	{
		k_msgq_get(&rx_msgq, &bio_arm_rx_frame, K_FOREVER);

		struct can_frame frame = bio_arm_rx_frame;

		LOG_INF("CAN frame sent: ID: %d", frame.id);
		LOG_INF("CAN frame data: %d %d %d", frame.data_32[0],
							frame.data[4],
							frame.data[5]);

		if(frame.dlc != 6)
		{
			//just handling motor commands for now
			LOG_ERR("Unknown Frame Received\n");
			continue;
		}

		switch (frame.data[4]) {
		
		case ACTUATOR_COMMAND_ID:
			if (frame.data[5] < ROBOCLAW_BASE_ID + ROBOCLAWS_COUNT)
			{
				// in the case it will only consider from 10 - 11
				pwm_motor_write(&roboclaw[frame.data[5] - ROBOCLAW_BASE_ID], frame.data_32[0]);
			}
			else if (frame.data[5] < SERVO_BASE_ID + SERVOS_COUNT)
			{
				// it will consider from 15 - 19
				pwm_motor_write(&servo[frame.data[5] - SERVO_BASE_ID], frame.data_32[0]);
			}
			break;
		
		case SENSOR_DATA_ID:
			LOG_INF("Received sensor data\n");
			break;
		}
	}
	return 0;
}
