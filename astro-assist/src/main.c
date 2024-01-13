/*
 * Source file for Astro Assist Application
 */


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app_version.h>

#include <kycan.h>
#include <kyvernitis.h>

#define L293D_COUNT 4
#define L293D_LIM_COUNT 2

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* DT spec for limit switches */
const struct gpio_dt_spec switches[L293D_LIM_COUNT] = {
		GPIO_DT_SPEC_GET_BY_IDX(
					DT_NODELABEL(limitswitches),
                                        gpios,
					0),
		GPIO_DT_SPEC_GET_BY_IDX(
					DT_NODELABEL(limitswitches),
                                        gpios,
					1)
};


/* DT spec for all instances of motors controlled by l293d*/
const struct dc_motor l293d[L293D_COUNT] = {
	{
		.input_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_1), gpios, 0),
		.input_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_1), gpios, 1)
	},
	{
		.input_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_2), gpios, 0),
		.input_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_2), gpios, 1),
	},
	{
		.input_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_3), gpios, 0),
		.input_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_3), gpios, 1)
	},
	{
		.input_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_4), gpios, 0),
		.input_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(motor_4), gpios, 1)
	}
};

/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* Can filter for Astro Assist*/
const struct can_filter astro_assist_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = ASTRO_ASSIST_ID,
	.mask = CAN_EXT_ID_MASK
};

struct can_frame astro_assist_rx_frame;



int main()
{
	printk("\nAstro-Assist: v%s\n\n", APP_VERSION_STRING);

	int err;

	/* Device ready checks */
	
	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}
	
	for (size_t i = 0U; i < ARRAY_SIZE(switches); i++) {
		if (!gpio_is_ready_dt(&(switches[i]))) {
			LOG_ERR("Limit Switch: %d is not ready\n", i + L293D_BASE_ID);
			return 0;
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(l293d); i++) {
		if (!gpio_is_ready_dt(&(l293d[i].input_1))) {
			LOG_ERR("DC-motor %d: input %d is not ready\n", i,
							l293d[i].input_1.pin);
			return 0;
		}
		if (!gpio_is_ready_dt(&(l293d[i].input_2))) {
			LOG_ERR("DC-motor %d: input %d is not ready\n", i,
							l293d[i].input_2.pin);
			return 0;
		}
	}

	if (!gpio_is_ready_dt(&led)){
		LOG_ERR("Error: Led not ready\n");
		return 0;
	}

	/* Start up configurations */
	
	for (size_t i = 0U; i < ARRAY_SIZE(switches); i++) {
		err = gpio_pin_configure_dt(&switches[i], GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("Error %d: failed to configure %s pin %d\n",
					err, switches[i].port->name, switches[i].pin);
			return 0;
		}
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
	
	
	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &astro_assist_filter);
	if (filter_id < 0) {
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
	}


	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0)
	{
		LOG_ERR("Error: Led not configured\n");
		return 0;
	}	

	LOG_INF("Initialization completed successfully\n");

	while (true)
	{
		err = k_msgq_get(&rx_msgq, &astro_assist_rx_frame, K_MSEC(100));
		if(k_msgq_get(&rx_msgq, &astro_assist_rx_frame, K_MSEC(100))) {
			LOG_ERR("Message Recieve Timeout!!");
			for(size_t i = 0U; i < ARRAY_SIZE(l293d); i++) {
				err = dc_motor_write(&l293d[i], DC_MOTOR_STOP);
				if (err) {
					return 0;
				}

			}
			LOG_INF("Stopped all motors");
			continue;
		}
		struct can_frame frame = astro_assist_rx_frame;

		if(frame.dlc != 6) {
			//just handling motor commands for now
			LOG_ERR("Unknown Frame Received\n");
			continue;
		}

		switch (frame.data[4]) {
		
		case ACTUATOR_COMMAND_ID:
			if (frame.data[5] < L293D_BASE_ID + L293D_LIM_COUNT) {
				// Consider from 25 to 26
				int motor_no = frame.data[5] - L293D_BASE_ID;
				err = dc_motor_write_lim(&l293d[motor_no],
							frame.data_32[0],
							&switches[motor_no]);
				if (err) {
					return 0;
				}
			}	
			else if (frame.data[5] < L293D_BASE_ID + L293D_COUNT) {
				// consider from 25 to 28	
				err = dc_motor_write(&l293d[frame.data[5] - L293D_BASE_ID],
							frame.data_32[0]);
				
				if (err) {
					return 0;
				}
			}
			break;
		}
		gpio_pin_toggle_dt(&led);
	}
}
