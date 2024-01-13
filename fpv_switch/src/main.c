#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <app_version.h>

#include <kycan.h>

#ifdef CONFIG_LOOPBACK_MODE
	#define TX_THREAD_STACK_SIZE 512
	#define TX_THREAD_PRIORITY 2

	K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);

#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

/* DT spec for gpio pins */
const struct gpio_dt_spec gpio_1 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 0);
const struct gpio_dt_spec gpio_2 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 1);
const struct gpio_dt_spec gpio_3 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 2);
const struct gpio_dt_spec gpio_4 = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(io_pins), gpios, 3);

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* DT spec for can module */
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* can filter for fpv switch */
struct can_filter fpv_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = FPV_SWITCH_ID,
	.mask = CAN_EXT_ID_MASK,
};

struct can_frame fpv_switch_rx_frame;


#ifdef CONFIG_LOOPBACK_MODE
	
	struct can_frame fpv_switch_tx_frame = {
		.flags = CAN_FRAME_IDE,
		.id = FPV_SWITCH_ID,
	};

	struct k_thread tx_thread_data;

	void tx_thread(void *unused1, void *unused2, void *unused3)
	{
		int count = 0;
		while (1) {	
			fpv_switch_tx_frame.dlc = 6;
			fpv_switch_tx_frame.data[5] = (uint8_t)count;
			count++;
		
			can_send(can_dev, &fpv_switch_tx_frame, K_MSEC(100), NULL, NULL);
			LOG_INF("CAN frame sent: ID: %d", fpv_switch_tx_frame.id);
			LOG_INF("CAN frame data: %d", fpv_switch_tx_frame.data[5]);
			k_sleep(K_SECONDS(1));
		}

		return;
	}
#endif


int main(void)
{
	printk("\nFPV-Switch: v%s\n\n", APP_VERSION_STRING);
	
	int err;

	/* Device ready checks */

	if (!gpio_is_ready_dt(&gpio_1)) {
		LOG_ERR("GPIO 1 : Pin not ready\n");
		return 0;
    };

	if (!gpio_is_ready_dt(&gpio_2)) {
		LOG_ERR("GPIO 2 : Pin not ready\n");
		return 0;
    };
	
	if (!gpio_is_ready_dt(&gpio_3)) {
		LOG_ERR("GPIO 3 : Pin not ready\n");
		return 0;
    };

	if (!gpio_is_ready_dt(&gpio_4)) {
		LOG_ERR("GPIO 4 : Pin not ready\n");
		return 0;
    };

	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}

	/* Start up configurations */

	err = gpio_pin_configure_dt(&gpio_1, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
				err, gpio_1.port->name, gpio_1.pin);
		return 0;
	}

	err = gpio_pin_configure_dt(&gpio_2, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
				err, gpio_2.port->name, gpio_2.pin);
		return 0;
	}

	err = gpio_pin_configure_dt(&gpio_3, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
				err, gpio_3.port->name, gpio_3.pin);
		return 0;
	}

	err = gpio_pin_configure_dt(&gpio_4, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
				err, gpio_4.port->name, gpio_4.pin);
		return 0;
	}


#ifdef CONFIG_LOOPBACK_MODE
	k_tid_t tx_tid;

	if (can_set_mode(can_dev, CAN_MODE_LOOPBACK)) {
		LOG_ERR("Error setting CAN mode");
		return 0;
	}
	
	if (can_start(can_dev)) {
		LOG_ERR("Error starting CAN controller.\n");
		return 0;
	}

	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &fpv_filter);
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

#endif

	if (can_start(can_dev)) {
		LOG_ERR("Error starting CAN controller.\n");
		return 0;
	}

	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &fpv_filter);
	if (filter_id < 0)
	{
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
	}

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (err != 0) {
		LOG_ERR("Error : LED not configured\n");
		return 0;
	}

	LOG_INF("Initialization completed successfully\n");


	while(1) {

		k_msgq_get(&rx_msgq, &fpv_switch_rx_frame, K_FOREVER);
		
		struct can_frame frame = fpv_switch_rx_frame;

		switch (frame.data[5]) {	
			
		case 0:
			gpio_pin_set_dt(&gpio_1, 1);
			gpio_pin_set_dt(&gpio_2, 0);
			gpio_pin_set_dt(&gpio_3, 0);
			gpio_pin_set_dt(&gpio_4, 0);
			break;
		case 1:
            gpio_pin_set_dt(&gpio_1, 0);
            gpio_pin_set_dt(&gpio_2, 1);
            gpio_pin_set_dt(&gpio_3, 0);
            gpio_pin_set_dt(&gpio_4, 0);
			break;
		case 2:
            gpio_pin_set_dt(&gpio_1, 0);
            gpio_pin_set_dt(&gpio_2, 0);
            gpio_pin_set_dt(&gpio_3, 1);
            gpio_pin_set_dt(&gpio_4, 0);
            break;
		case 3:
            gpio_pin_set_dt(&gpio_1, 0);
            gpio_pin_set_dt(&gpio_2, 0);
            gpio_pin_set_dt(&gpio_3, 0);
            gpio_pin_set_dt(&gpio_4, 1);
            break;
		}

		gpio_pin_toggle_dt(&led);
	}
	return 0;
}
