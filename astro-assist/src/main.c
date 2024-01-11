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


