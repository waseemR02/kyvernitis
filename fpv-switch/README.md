# General Description

## CAN frame format

| Frame           |                                                                |
| :-------------: | :------------------------------------------------------------- |
| ID              | FPV_SWITCH_ID {0x500}                                          |
| flags           | Extended CAN ID                                                |
| dlc             | 6                                                              |
| data            | 0 {4 bytes}+ FPV_SWITCH_CMD_ID {40}{1 byte} + 0/1/2/3 {1 bytes}|

## Pin Configuration

| PIN             |                |
| :-------------: | :-------------:|
| A9              | TX             |
| A1              | INP_1          |
| A2              | INP_2          |
| A3              | INP_3          |
| A4              | INP_4          |
| B12             | CS             |
| A6              | MISO           |
| A5              | SCK            |
| A7              | MOSI           |
| B9              | INT            |
