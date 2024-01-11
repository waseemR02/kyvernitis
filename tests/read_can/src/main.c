/*
 * Source file for reading can over can-bus and logging
 */


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_read_test , CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/*
 * Can filter to read all frames with extended IDs
 */ 
const struct can_filter ex_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
};

struct can_frame rx_frame;

int main()
{
	int filter_id;
	
	if (!device_is_ready(can_dev)) {
		LOG_INF("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}
	
	if (can_start(can_dev)) {
		LOG_ERR("Error starting CAN controller.\n");
		return 0;
	}

	filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &ex_filter);
	if (filter_id < 0)
	{
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
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

	while (true)
	{
		k_msgq_get(&rx_msgq, &rx_frame, K_FOREVER);
		gpio_pin_toggle_dt(&led);
		printk("Frame recieved\n");
		printk("Frame ID: %d\n", rx_frame.id);
	}
	return 0;
}
