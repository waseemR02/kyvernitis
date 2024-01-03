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

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/*
 * Can filter to read all frames with extended IDs
 */ 
const struct can_filter ex_frame = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = 0,
	.mask = CAN_EXT_ID_MASK
};

struct can_frame rx_frame;

void rx_callback(const struct device *dev, struct can_frame *frame, void *unused)
{
	gpio_pin_toggle_dt(&led);
}

int main()
{
	int filter_id;

	if (!device_is_ready(can_dev)) {
		LOG_INF("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}
	
	filter_id = can_add_rx_filter(can_dev, rx_callback, NULL, &ex_frame);
	if (filter_id < 0) {
		LOG_ERR("Unable to add rx filter [%d]", filter_id);
	}

	return 0;
}
