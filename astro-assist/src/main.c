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

#define L298N_COUNT 4
#define L298N_LIM_COUNT 2

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 10);

#ifdef CONFIG_LOOPBACK_MODE

#define TX_THREAD_STACK_SIZE 512
#define TX_THREAD_PRIORITY 2

K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);

#endif

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* DT spec for limit switches */
const struct gpio_dt_spec switches[L298N_LIM_COUNT] = {
		GPIO_DT_SPEC_GET_BY_IDX(
					DT_NODELABEL(limitswitches),
                                        gpios,
					0),
		GPIO_DT_SPEC_GET_BY_IDX(
					DT_NODELABEL(limitswitches),
                                        gpios,
					1)
};


/* DT spec for all instances of motors controlled by l298n*/
const struct dc_motor l298n[L298N_COUNT] = {
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


#ifdef CONFIG_LOOPBACK_MODE
	
struct can_frame astro_assist_tx_frame = {
	.flags = CAN_FRAME_IDE,
	.id = ASTRO_ASSIST_ID,
	.dlc = 6,
	.data[4] = ACTUATOR_COMMAND_ID,
};

struct k_thread tx_thread_data;

void tx_thread(void *unused1, void *unused2, void *unused3)
{
	while (1) {
		for(size_t i = 0u; i < ARRAY_SIZE(l298n); i++) {
			astro_assist_tx_frame.data[5] = i + L298N_BASE_ID;
			astro_assist_tx_frame.data_32[0] = DC_MOTOR_FORWARD;
			can_send(can_dev, &astro_assist_tx_frame, K_MSEC(100), NULL, NULL);
		}

		k_sleep(K_SECONDS(1));

		for(size_t i = 0u; i < ARRAY_SIZE(l298n); i++) {
			astro_assist_tx_frame.data[5] = i + L298N_BASE_ID;
			astro_assist_tx_frame.data_32[0] = DC_MOTOR_STOP;
			can_send(can_dev, &astro_assist_tx_frame, K_MSEC(100), NULL, NULL);
		}

		k_sleep(K_SECONDS(1));

		for(size_t i = 0u; i < ARRAY_SIZE(l298n); i++) {
			astro_assist_tx_frame.data[5] = i + L298N_BASE_ID;
			astro_assist_tx_frame.data_32[0] = DC_MOTOR_BACKWARD;
			can_send(can_dev, &astro_assist_tx_frame, K_MSEC(100), NULL, NULL);
		}

		k_sleep(K_SECONDS(1));

		for(size_t i = 0u; i < ARRAY_SIZE(l298n); i++) {
			astro_assist_tx_frame.data[5] = i + L298N_BASE_ID;
			astro_assist_tx_frame.data_32[0] = DC_MOTOR_STOP;
			can_send(can_dev, &astro_assist_tx_frame, K_MSEC(100), NULL, NULL);
		}

		k_sleep(K_SECONDS(1));
	}
	
	return;
}

#endif

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
			LOG_ERR("Limit Switch: %d is not ready\n", i + L298N_BASE_ID);
			return 0;
		}
	}

	for (size_t i = 0U; i < ARRAY_SIZE(l298n); i++) {
		if (!gpio_is_ready_dt(&(l298n[i].input_1))) {
			LOG_ERR("DC-motor %d: input %d is not ready\n", i,
							l298n[i].input_1.pin);
			return 0;
		}
		if (!gpio_is_ready_dt(&(l298n[i].input_2))) {
			LOG_ERR("DC-motor %d: input %d is not ready\n", i,
							l298n[i].input_2.pin);
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
	
	for (size_t i = 0U; i < ARRAY_SIZE(l298n); i++) {
		if (gpio_pin_configure_dt(&(l298n[i].input_1), GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Error : LED not configured\n");
			return 0;
		}
		if (gpio_pin_configure_dt(&(l298n[i].input_2), GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Error : LED not configured\n");
			return 0;
		}
	}	

#ifdef CONFIG_LOOPBACK_MODE

	k_tid_t tx_tid;

	if (can_set_mode(can_dev, CAN_MODE_LOOPBACK)) {
		LOG_ERR("Error setting CAN mode");
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
#ifndef CONFIG_LOOPBACK_MODE
		if(k_msgq_get(&rx_msgq, &astro_assist_rx_frame,  K_MSEC(1000))) {
#else
		if(k_msgq_get(&rx_msgq, &astro_assist_rx_frame, K_FOREVER)) {
#endif
			LOG_ERR("Message Recieve Timeout!!");
			for(size_t i = 0U; i < ARRAY_SIZE(l298n); i++) {
				err = dc_motor_write(&l298n[i], DC_MOTOR_STOP);
				if (err) {
					return 0;
				}

			}
			LOG_INF("Stopped all motors");
			continue;
		}
		struct can_frame frame = astro_assist_rx_frame;

		printk("CAN frame ID: %x  Data: %d %d %d\n", frame.id, frame.data_32[0], frame.data[4], frame.data[5]);

		if(frame.dlc != 6) {
			//just handling motor commands for now
			LOG_ERR("Unknown Frame Received\n");
			continue;
		}

		switch (frame.data[4]) {
		
		case ACTUATOR_COMMAND_ID:
			if (frame.data[5] < L298N_BASE_ID + L298N_LIM_COUNT) {
				// Consider from 25 to 26
				int motor_no = frame.data[5] - L298N_BASE_ID;
				err = dc_motor_write_lim(&l298n[motor_no],
							frame.data_32[0],
							&switches[motor_no]);
				if (err) {
					return 0;
				}
			}	
			else if (frame.data[5] < L298N_BASE_ID + L298N_COUNT) {
				// consider from 25 to 28	
				err = dc_motor_write(&l298n[frame.data[5] - L298N_BASE_ID],
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
