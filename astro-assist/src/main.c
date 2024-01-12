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


