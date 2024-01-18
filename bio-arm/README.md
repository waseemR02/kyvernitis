# General Description

## CAN frame format

| Frame           |                                                                     |
| :-------------: | :------------------------------------------------------------------ |
| ID              | BIO_ARM_ID {0x300}                                                  |
| flags           | Extended CAN ID                                                     |
| dlc             | 6                                                                   |
| data            | pwm {4 bytes} + ACTUATOR_COMMAND_ID {10}{1 byte} + motor_id {1 byte}|

## Schematics

![Bio-arm](https://github.com/waseemR02/kyvernitis/assets/98299006/af5c61b9-da94-4aba-8280-41c0a0881bcd)


## Pin Configuration

| PIN             |                |
| :-------------: | :-------------:|
| B6              | TX             |
| B12             | CS             |
| A6              | MISO           |
| A5              | SCK            |
| A7              | MOSI           |
| B9              | INT            |
| B8              | INP1           |
| A0              | mq136          |
| A1              | mq2            |
| A2              | dht11          |
| B0              | mq137          |
| B1              | mq7            |
| A9              | PWM1           |
| A15             | PWM2           |
| B5              | PWM3           |
| B8              | PWM4           |
