#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <app_version.h>

#include <kycan.h>

#include <zephyr/logging/log.h>



CAN_MSGQ_DEFINE(rx_msgq, 10);
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* DT spec for gpio pins */
const struct gpio_dt_spec gpio_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 0);
const struct gpio_dt_spec gpio_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 1);
const struct gpio_dt_spec gpio_3 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 2);
const struct gpio_dt_spec gpio_4 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 3);

/* DT spec for can module */
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* can filter for fpv switch */
struct can_filter fpv_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = FPV_SWITCH_ID,
	.mask = CAN_EXT_ID_MASK,
};

struct can_frame fpv_switch_rx_frame;

int main(void)
{
	if (!gpio_is_ready_dt(&gpio_1)) {
        	printf("GPIO 1 : Pin not ready\n");
        } else {
                printf("GPIO 1 : Pin ready\n");
        };


        if (!gpio_is_ready_dt(&gpio_2)) {
                printf("GPIO 2 : Pin not ready\n");
        } else {
                printf("GPIO 2 : Pin ready\n");
        };


        if (!gpio_is_ready_dt(&gpio_3)) {
                printf("GPIO 3 : Pin not ready\n");
        } else {
                printf("GPIO 3 : Pin ready\n");
        };


        if (!gpio_is_ready_dt(&gpio_4)) {
                printf("GPIO 4 : Pin not ready\n");
        } else {
                printf("GPIO 4 : Pin ready\n");
        };
	k_msleep(3000);
	while(1) {

		k_msgq_get(&rx_msgq, &fpv_switch_rx_frame, K_FOREVER);
		
		struct can_frame frame = fpv_switch_rx_frame;

		switch (frame.data[5]) {	
			
			case '0':
				gpio_pin_set_dt(&gpio_1, 1);
				gpio_pin_set_dt(&gpio_2, 0);
				gpio_pin_set_dt(&gpio_3, 0);
				gpio_pin_set_dt(&gpio_4, 0);
				break;
			case '1':
                                gpio_pin_set_dt(&gpio_1, 0);
                                gpio_pin_set_dt(&gpio_2, 1);
                                gpio_pin_set_dt(&gpio_3, 0);
                                gpio_pin_set_dt(&gpio_4, 0);
				break;
			case '2':
                                gpio_pin_set_dt(&gpio_1, 0);
                                gpio_pin_set_dt(&gpio_2, 0);
                                gpio_pin_set_dt(&gpio_3, 1);
                                gpio_pin_set_dt(&gpio_4, 0);
                                break;
			case '3':
                                gpio_pin_set_dt(&gpio_1, 0);
                                gpio_pin_set_dt(&gpio_2, 0);
                                gpio_pin_set_dt(&gpio_3, 0);
                                gpio_pin_set_dt(&gpio_4, 1);
                                break;
	
		}
	}
	return 0;
}
