/*
* Kyervnitis helper macros and function declarations
*/

#define MAX_ROBOCLAWS 2
#define MAX_SABERTOOTHS 2
#define MAX_SERVOS 5

struct dc_motor {
	const struct gpio_dt_spec input_1;
	const struct gpio_dt_spec input_2;
};
