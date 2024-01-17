/*
 * Source file for Gripper Arm Application
 */


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app_version.h>

#include <kycan.h>
#include <kyvernitis.h>

#define ROBOCLAWS_COUNT 2
#define STEPPER_MOTOR_COUNT 2

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
CAN_MSGQ_DEFINE(rx_msgq, 25);

/* Testing tx_thread macros */
#ifdef CONFIG_LOOPBACK_MODE

#define TX_THREAD_STACK_SIZE 512
#define TX_THREAD_PRIORITY 2

K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);

#endif

/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* DT spec for led*/
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* DT spec for roboclaws*/
struct pwm_motor roboclaw[ROBOCLAWS_COUNT] = {
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor1)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_motor1), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_motor1), max_pulse)
	},
	{
		.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_motor2)),
		.min_pulse = DT_PROP(DT_ALIAS(pwm_motor2), min_pulse),
		.max_pulse = DT_PROP(DT_ALIAS(pwm_motor2), max_pulse)
	}
};

struct pwm_motor servo = {
	.dev_spec = PWM_DT_SPEC_GET(DT_ALIAS(pwm_servo1)),
	.min_pulse = DT_PROP(DT_ALIAS(pwm_servo1), min_pulse),
	.max_pulse = DT_PROP(DT_ALIAS(pwm_servo1), max_pulse)
};

servo_state_t servo_state = SERVO_DEFAULT_STATE;

/* DT spec for stepper motors */
struct stepper_motor tb6600[STEPPER_MOTOR_COUNT] = {
	{
		.dir = GPIO_DT_SPEC_GET(DT_ALIAS(stepper_motor1), dir_gpios),
		.step = GPIO_DT_SPEC_GET(DT_ALIAS(stepper_motor1), step_gpios)
	},
	{
		.dir = GPIO_DT_SPEC_GET(DT_ALIAS(stepper_motor2), dir_gpios),
		.step = GPIO_DT_SPEC_GET(DT_ALIAS(stepper_motor2), step_gpios)
	}
};

/* Can filter for Astro Assist*/
const struct can_filter gripper_arm_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
	.id = GRIPPER_ARM_ID,
	.mask = CAN_EXT_ID_MASK
};

struct can_frame gripper_arm_rx_frame;


/* Testing tx_thread */
#ifdef CONFIG_LOOPBACK_MODE
	
struct can_frame gripper_arm_tx_frame = {
	.flags = CAN_FRAME_IDE,
	.id = GRIPPER_ARM_ID,
	.dlc = 6,
	.data[4] = ACTUATOR_COMMAND_ID,
};

struct k_thread tx_thread_data;

void tx_thread(void *unused1, void *unused2, void *unused3)
{
	while (1) {
		k_sleep(K_SECONDS(1));
		gripper_arm_tx_frame.data_32[0] = 0;
		gripper_arm_tx_frame.data[5] = 10;
		can_send(can_dev, &gripper_arm_tx_frame, K_MSEC(100), NULL, NULL);
	}
	
	return;
}

#endif

int main()
{
	printk("\nGripper-Arm: v%s\n\n", APP_VERSION_STRING);

	int err;
	ARG_UNUSED(err);

	/* Device ready checks */

	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN: Device %s not ready.\n", can_dev->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&led)){
		LOG_ERR("Error: Led not ready\n");
		return 0;
	}

	/* Start up configurations */

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
	
	
	int filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &gripper_arm_filter);
	if (filter_id < 0) {
		LOG_ERR("Unable to add rx msgq [%d]", filter_id);
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0)
	{
		LOG_ERR("Error: Led not configured\n");
		return 0;
	}	
	
	while (true)
	{
#ifndef CONFIG_LOOPBACK_MODE
		if(k_msgq_get(&rx_msgq, &gripper_arm_rx_frame,  K_MSEC(1000))) {
#else
		if(k_msgq_get(&rx_msgq, &gripper_arm_rx_frame, K_FOREVER)) {
#endif
			LOG_ERR("Message Recieve Timeout!!");
			// send stop command to roboclaw motors
			for(size_t i = 0U; i < ARRAY_SIZE(roboclaw); i++) {
				if(pwm_motor_write(&roboclaw[i], PWM_MOTOR_STOP)) {
					LOG_ERR("Unable to write pwm pulse to Roboclaw: %d", i + ROBOCLAW_BASE_ID);
					return 0;
				}
			}
			LOG_INF("Stopped all Roboclaw motors");

			if(pwm_motor_write(&servo, servo_state)) {
				LOG_ERR("Unable to write pwm pulse to servo");
				return 0;
			}
			LOG_INF("Set all servo motors to previous state");

			continue;
		}
				
		struct can_frame frame = gripper_arm_rx_frame;

		printk("CAN frame ID: %x  Data: %d %d %d\n", frame.id, frame.data_32[0], frame.data[4], frame.data[5]);

		if(frame.dlc != 6) {
			//just handling motor commands for now
			LOG_ERR("Unknown Frame Received\n");
			continue;
		}

		switch (frame.data[4]) {
		
		case ACTUATOR_COMMAND_ID:
			if (frame.data[5] < ROBOCLAW_BASE_ID + ROBOCLAWS_COUNT)
			{
				// in the case it will only consider from 10 - 11
				if (pwm_motor_write(&roboclaw[frame.data[5] - ROBOCLAW_BASE_ID], frame.data_32[0])) {
					LOG_ERR("Unable to write pwm pulse to PWM motor: %d", frame.data[5]);
					return 0;
				}
			}
			else if (frame.data[5] < SERVO_BASE_ID)
			{
				// it will consider from 15 - 19
				if(pwm_motor_write(&servo, servo_state)) {
					LOG_ERR("Unable to write pwm pulse to Servo Motor: %d", frame.data[5]);
					return 0;
				}
				else {
					servo_state = frame.data_32[0];
				}
			}
			break;
		}
	}
	return 0;
}
