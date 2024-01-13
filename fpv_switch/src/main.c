#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <app_version.h>

#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);


const struct gpio_dt_spec gpio_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 0);

const struct gpio_dt_spec gpio_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 1);

const struct gpio_dt_spec gpio_3 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 2);

const struct gpio_dt_spec gpio_4 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 3);



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

	return 0;
}
