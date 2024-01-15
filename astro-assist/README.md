# General Description

## CAN frame format

| Frame           |                                                                     |
| :-------------: | :------------------------------------------------------------------ |
| ID              | ASTRO_ASSIST_ID {0x400}                                             |
| flags           | Extended CAN ID                                                     |
| dlc             | 6                                                                   |
| data            | 0/1/2 {4 bytes} + ACTUATOR_COMMAND_ID {10}{1 byte} + 25-28 {1 bytes}|

## Schematics

![Astro-Assist](https://github.com/waseemR02/kyvernitis/assets/98299006/7cc9c33b-c7f7-4b81-9a3e-e73dc3de0b0b)

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
| A0              | INP2           |
| B4              | INP3           |
| A1              | INP4           |
| B10             | INP_1          |
| A3              | INP_2          |
| A2              | INP_3          |
| A10             | INP_4          |
| A8              | LIM_1          |
| A9              | LIM_2          |
