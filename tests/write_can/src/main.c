/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/can.h>
#include <string.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 100

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* DT spec for can module*/
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\r') && rx_buf_pos > 0) {
			uart_fifo_read(uart_dev, &c, 1);
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void parse_message(char *buffer, uint32_t *msgid, uint8_t *cmd_type, uint32_t *cmd, uint8_t *num) {
    char *start = buffer;
    char *end = buffer;

    // Parse msgid
    while (*end != ',' && *end != '\0') {
        end++;
    }
    if (*end == ',') {
        *end = '\0';
        *msgid = 0;
        while (*start >= '0' && *start <= '9') {
            *msgid = (*msgid * 10) + (*start - '0');
            start++;
        }
        start = end + 1;
        end = start;
    }

    // Parse cmd_type
    while (*end != ',' && *end != '\0') {
        end++;
    }
    if (*end == ',') {
        *end = '\0';
        *cmd_type = 0;
        while (*start >= '0' && *start <= '9') {
            *cmd_type = (*cmd_type * 10) + (*start - '0');
            start++;
        }
        start = end + 1;
        end = start;
    }

    // Parse cmd
    while (*end != ',' && *end != '\0') {
        end++;
    }
    if (*end == ',') {
        *end = '\0';
        *cmd = 0;
        while (*start >= '0' && *start <= '9') {
            *cmd = (*cmd * 10) + (*start - '0');
            start++;
        }
        start = end + 1;
        end = start;
    }

    // Parse num
    *num = 0;
    while (*start >= '0' && *start <= '9') {
        *num = (*num * 10) + (*start - '0');
        start++;
    }
}



int main(void)
{
	char rx_msg[MSG_SIZE];
	char clone[MSG_SIZE];
	uint32_t msg_id;
	uint8_t cmd_type;
	uint32_t cmd;
	uint8_t num;
	
	struct can_frame tx_frame;

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	
	if (can_start(can_dev)) {
		return 0;
	}

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(uart_dev);

	print_uart("Hello! I'm your echo bot.\r\n");
	print_uart("Tell me something and press enter:\r\n");

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, &rx_msg, K_FOREVER) == 0) {
		strcpy(clone, rx_msg);
		parse_message(rx_msg, &msg_id, &cmd_type, &cmd, &num);
	// 	
	// 	tx_frame.id = msg_id;
	// 	tx_frame.dlc = 6;
	// 	tx_frame.data_32[0] = cmd;
	// 	tx_frame.data[4] = cmd_type;
	// 	tx_frame.data[5] = num;
	// 
	// 	can_send(can_dev, &tx_frame, K_MSEC(100), NULL, NULL);
		
		print_uart("Sent: ");
		print_uart(clone);
		print_uart("\r\n");
	}
	return 0;
}
