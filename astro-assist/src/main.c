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

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

/* DT spec for limit switches */
const struct gpio_dt_spec switch_1 = GPIO_DT_SPEC_GET_BY_IDX(
						DT_NODELABEL(limitswitches),
                                                gpios,
						0);

const struct gpio_dt_spec switch_2 = GPIO_DT_SPEC_GET_BY_IDX(
						DT_NODELABEL(limitswitches),
                                                gpios,
						1);

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
	printk("Astro-Assist: v%s", APP_VERSION_STRING);

	int err;

	/* Device ready checks */
	
	if (!device_is_ready(can_dev)) {
		LOG_INF("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&switch_1))
	{
		LOG_ERR("Error: Led not ready\n");
		return 0;
	}

	if (!gpio_is_ready_dt(&switch_2))
	{
		LOG_ERR("Error: Led not ready\n");
		return 0;
	};

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

	/* Start up configurations */

	err = gpio_pin_configure_dt(&switch_1, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       err, switch_1.port->name, switch_1.pin);
		return 0;
	}

	err = gpio_pin_configure_dt(&switch_2, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       err, switch_2.port->name, switch_2.pin);
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
	
}
