/**
 * Title:   	Humidty and temperature alarm
 * Author:  	Jaap Kanbier
 * 
 * Creation:	23 March 2019
 * Modified:	1 April 2019
 * 
 * Version:		1.0
 * 
 * Changelog:
 * 0.1      Created file, build basic DHT11 functionality
 * 0.2		Added LCD display, reads DHT11 functions
 * 0.3		Added warning/alert functions to display when certain limits are exceeded
 * 0.4		Added speaker/sounds to warning
 * 0.5		Added global explanatory comments/documentation for new users
 * 0.6		Added LED matrix and max7219 led driver basic functionality
 * 0.7		Added LED matrix animation functionality
 * 0.8		Added warning sounds pitch modifier
 * 0.9		Refactored logic away from displaying info to CheckStats()
 * 0.10		Added historic data persistence: min/max for temp & hum 
 * 0.11		Added filtering for unrealistic measurements (max delta 5 per measurement)
 * 0.12		Moved main loop lcd display logic to interupt flow to sync with animation
 * 1.0		Completed final documentation and succesfull test runs
 * 
 */

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>

// Include extra libraries
#include <util/delay.h>
#include <avr/interrupt.h>

// Include custom libraries
#include "hd44780.h"  // Library for Liquid LED Display Screen
#include "max7219/max7219.h"  // Library for LED Driver - LED Matrix
#include "dht.h"  // Library for Temperature and Humidity sensor


/**
 * USER CONFIGURATION
 * 
 * Set global limits for prefered alarms
 * Leave 'NULL' for no limit
 * Outside these boundries alarm rings
 */
#define TEMP_LIMIT_MIN 20
#define TEMP_LIMIT_MAX 30
#define HUM_LIMIT_MIN 20
#define HUM_LIMIT_MAX 50


// Set defines & variables for global code usage
#define SPEAKER_PIN PD5

int8_t temp_exceeded_dir = 0;		// stores if limits are exceeded, either above or below set limit
int8_t hum_exceeded_dir = 0;		
int8_t current_animation = 1;		// stores current animation, in the form of LEDMATRIX_XXX byte arrays
int8_t last_animation_done = 1;		// stores if current animation is done, prevents starting one before finishing
int8_t test_frequency = 0;			// variable to keep track first few measurements are done. Sometimes first are incorrect
int8_t display_step = 0;			// stores current step in different types of information

int8_t temperature_max = 0;			// stores highest known temperature value
int8_t temperature_min = 99;  		// stores lowest known temperature value. starts high to be overwriten by real data
int8_t temperature_previous = 0;  	// stores last known value, to rule out unrealistic measurements by means of delta comparison.
int8_t humidity_max = 0;
int8_t humidity_min = 99;
int8_t humidity_previous = 0;

// DHT library checks for DHT model type
// Reusable system for DHT22 which uses floats
#if DHT_FLOAT == 0
int8_t temperature = 0;
int8_t humidity = 0;
#elif DHT_FLOAT == 1
float temperature = 0;
float humidity = 0;
#endif

// Array of 8 bytes
// Each bit represents a bit in the 8x8LED matrix
uint8_t LEDMATRIX_CHECK[] = {  // Check mark
	0b00000000, 
	0b00000011, 
	0b00000111, 
	0b00001110, 
	0b11011100, 
	0b11111000, 
	0b11110000, 
	0b01100000
};
uint8_t LEDMATRIX_WARNING[] = {  // Exlamation marks
	0b00000000,
	0b01100110,
	0b01100110,
	0b01100110,
	0b01100110,
	0b00000000,
	0b01100110,
	0b00000000
};
uint8_t LEDMATRIX_HEART[] = {  // Heart
	0b00000000,
	0b01100110,
	0b10011001,
	0b10000001,
	0b10000001,
	0b01000010,
	0b00100100,
	0b00011000
};

/**
 * Interupt triggered by the overflow of timer 1A.
 */
ISR(TIMER1_OVF_vect) {
	// Check if temperature values are nominal, otherwise show warning
	if (display_step == 0 && (temp_exceeded_dir == 1 || temp_exceeded_dir == -1)) {
		printWarning(0, temp_exceeded_dir);
	}
	// Check if humidity values are nominal, otherwise show warning
	else if(display_step == 1 && (hum_exceeded_dir == 1 || hum_exceeded_dir == -1)) {
		printWarning(1, hum_exceeded_dir);
	}
	// If no warnings are present, first step is default current values display
	else if(display_step <= 2) {
		printTempHum_Current(temperature, humidity);
	}
	// Show the highest and lowest known temperature values
	else if(display_step == 3) {
		printTemp_History(temperature_max, temperature_min);
	}
	// Show the highest and lowest known humidity values
	else if(display_step == 4) {
		printHum_History(humidity, humidity_min);
	}
	
	display_step++;  // Increase to the next display step

	if (display_step > 4) {  // Reset display stepthrough
		display_step = 0;
	}
			
	// Check if last animation is already done
	if (last_animation_done == 1) {
		last_animation_done = 0;  // Set flag so no new animations are started

		// Check which animation is currently required
		if (current_animation == 0) {
			matrixDisplay_animate(LEDMATRIX_CHECK, 1);
		}
		else if(current_animation == 1) {
			matrixDisplay_animate(LEDMATRIX_WARNING, 1);
		}
		else {
			matrixDisplay_animate(LEDMATRIX_HEART, 1);  // When in doubt, share some love
		}

		last_animation_done = 1;  // After animation, clear flag
	}
}

/**
 * Function: Displays the current temperature & humidity on the LCD screen.
 * Argument: Takes the temperature (in degrees Celcius) & humidity (in percentage) as integers.
 * Returns: None.
 */
void printTempHum_Current(int temperature, int humidity) {
	char str[8];  // Array of chars to hold string values to be printed/displayed on the screen
	sprintf(str, "%d", temperature);  // Converts the given integer argument to a string to fit the LCD display library

	/* DISPLAY REGULAR TEMPERATURE */
	lcd_goto(0);  // Set cursor to the beginning of the display
	lcd_puts("Temperature: ");  // Print string to display
	lcd_puts(str);  // Print string of current temp value to display
	lcd_puts("C  ");  // Append Celcius indicator to value on display

	sprintf(str, "%d", humidity);

	lcd_goto(0x40);  // Set screen cursor to second line
	lcd_puts("Humidity:    "); 
	lcd_puts(str); 
	lcd_puts("%  "); 
}


/**
 * Function: Displays historic (highest & lowest) temperature values
 * Argument: Takes the temperature (in degrees Celcius) & humidity (in percentage) as integers.
 * Returns: None.
 */
void printTemp_History(int temperature_max, int temperature_min) {
	lcd_goto(0);
	lcd_puts("Temp. history:   ");

	char str_max[8];
	char str_min[8];

	sprintf(str_max, "%d", temperature_max);
	sprintf(str_min, "%d", temperature_min); 

	lcd_goto(0x40);  // Set screen cursor to second line
	lcd_puts("Min "); 
	lcd_puts(str_min); 
	lcd_puts("C Max "); 
	lcd_puts(str_max);
	lcd_puts("C   "); 
}

/**
 * Function: Displays historic (highest & lowest) humidity values
 * Argument: Takes the temperature (in degrees Celcius) & humidity (in percentage) as integers.
 * Returns: None.
 */
void printHum_History( humidity_max, humidity_min) {
	lcd_goto(0);
	lcd_puts("Hum. history:    ");

	char str_max[8];
	char str_min[8];

	sprintf(str_max, "%d", humidity_max);
	sprintf(str_min, "%d", humidity_min); 

	lcd_goto(0x40);  // Set screen cursor to second line
	lcd_puts("Min "); 
	lcd_puts(str_min); 
	lcd_puts("% Max "); 
	lcd_puts(str_max);
	lcd_puts("%   "); 
}

/**
 * Function: Plays the warning sound for a certain amount of milliseconds.
 * Argument: Time the sound should play in milliseconds.
 * Returns: None.
 */
void warningSounds(int timeMs) {
	// If so desired, the tuning/pitch of the noise can be tweaked by adjusting sounds_tuning
	// This in turn increases loop size and shortens amplitude distance, as to maintain desired warningSounds length
	uint8_t sounds_tuning = 2;

	for(size_t i = 0; i < timeMs * sounds_tuning; i++)  // timeMs * sounds_tuning creates 1ms, since every loop is devided by the same tuning
	{
		// use PWM to fake a analog signal
		for(uint8_t i = 0; i < 255; i++){
			if (100 > i) {
				PORTD |= (1 << SPEAKER_PIN);
			}
			//clear pin
			else {
				PORTD &= ~(1 << SPEAKER_PIN);
			}
			
		}
		_delay_us(1000 / sounds_tuning);  // Sets a variable delay in amplitudes
	}
}

/**
 * Functions: Animates an 8x8 image over led matrix.
 * Arguments:
 * 	1. Array of 8 bytes, each bit representing a LED.
 * 	2. Amount of animation repetitions.
 * Returns: none.
 */
void matrixDisplay_animate(uint8_t row_list[8], uint8_t repetitions) {

	uint8_t previous_value;

	// Loop over animation x times
	for(uint8_t repitition = 0; repitition < repetitions; repitition++)
	{
		// For 8 columns, fill every row
		// Shift backwards, from right to left
		for(int8_t column = 7; column >= 0; column--)
		{
			// Row after row with a small delay to animate
			for(uint8_t row = 0; row < 8; row++)
			{
				max7219_digit(0, row, (row_list[row] >> column));
			}
			_delay_ms(120);
		}

		// Shift image back out of matrix
		for(uint8_t column = 0; column < 8; column++)
		{
			for(uint8_t row = 0; row < 8; row++)
			{
				// grab every column of the image and slide it one left 
				previous_value = max7219_getdigit(0, row);
				max7219_digit(0, row, (previous_value << 1));
			}
			_delay_ms(120);
		}
	}

}

/**
 * Functions: When called, sets the screen to display a warning to user. Can be used for either temp or hum, too high or low.
 * Arguments: 
 * 		1. If the exceeded attribute is temperature or humidity
 *  	2. In which direction this attribute was exceeded.
 * Returns: none.
 */
void printWarning(int tempOrHum, int exceeded_dir) {
	char str[12];  // char array to hold set limit as string

	lcd_goto(0);

	// First print the line describing which attribute it concerns to the user
	if (tempOrHum == 0) {  // 0 == temperature
		// either show too high or too low
		if (exceeded_dir == 1) {
			lcd_puts("HIGH TEMPERATURE");
			lcd_goto(0x40);
			lcd_puts("Over ");
			sprintf(str, "%d", TEMP_LIMIT_MAX);
		}
		else if (exceeded_dir == -1) {
			lcd_puts("LOW TEMPERATURE ");
			lcd_goto(0x40);
			lcd_puts("Under ");
			sprintf(str, "%d", TEMP_LIMIT_MIN);
		}
		lcd_puts(str);  // Display the limit the user has configured
		lcd_puts("C limit! ");

	}
	else if(tempOrHum == 1) {  // 1 == humidity
		if (exceeded_dir == 1) {
			lcd_puts("HIGH HUMIDITY   ");
			lcd_goto(0x40);
			lcd_puts("Over ");
			sprintf(str, "%d", HUM_LIMIT_MAX);
		}
		else if (exceeded_dir == -1) {
			lcd_puts("LOW HUMIDITY    ");
			lcd_goto(0x40);
			lcd_puts("Under ");
			sprintf(str, "%d", HUM_LIMIT_MIN);
		}
		lcd_puts(str);
		lcd_puts("% limit! ");
	}

	warningSounds(2000);  // play a warning sound to hold warning in place for time being
}

/**
 * Function: Uses new measured data and modifies global variables accordingly
 * Arguments: 
 * 		1. New current temperature data
 * 		2. New current humidity data
 * Returns: None.
 */
void checkStats(temperature, humidity) {
	// Check if new data is valid, otherwise discard data
	if (isNewResultValid() == 1) {
		/* CHECK CURRENT VALUES EXCEED LIMITS */
		// If temperature limit is set by user AND the current value exceeds this limit
		if (TEMP_LIMIT_MAX != NULL && TEMP_LIMIT_MAX < temperature) {
			temp_exceeded_dir = 1;  // Set global flag to temp OVER set limit
			current_animation = 1;  // Set current animation to LEDMATRIX_WARNING
		}
		else if(TEMP_LIMIT_MIN != NULL && temperature < TEMP_LIMIT_MIN) {
			temp_exceeded_dir = -1;  // Set global flag to temp UNDER set limit
			current_animation = 1;
		}
		else {
			temp_exceeded_dir = 0;  // If no limits are exceeded, return to default
		}

		// If humidity limit is set AND the current value exceeds limits
		if (HUM_LIMIT_MAX != NULL && HUM_LIMIT_MAX < humidity) {
			hum_exceeded_dir = 1;
			current_animation = 1;
		}
		else if(HUM_LIMIT_MIN != NULL && humidity < HUM_LIMIT_MIN) {
			hum_exceeded_dir = -1;
			current_animation = 1;  
		}
		else {
			hum_exceeded_dir = 0;
		}

		// Only if both (temp & hum) limits aren't exceeded the warning animation is returned to CHECK
		if (hum_exceeded_dir == 0 && temp_exceeded_dir == 0) {
			current_animation = 0;
		}
		

		/* ADJUST STORED EXTREMES */
		// If the highest known value is lower then the current value, then the current value is the new highest
		if (temperature_max < temperature) {
			temperature_max = temperature;
		}
		// Idem for lowest known being higher then current value
		if (temperature_min > temperature) {
			temperature_min = temperature;
		}

		if (humidity_max < humidity) {
			humidity_max = humidity;
		}
		
		if (humidity_min > humidity) {
			humidity_min = humidity;
		}
	}
}

/**
 * Function: Check if new measurements are valid by checking checking vs historic delta
 * Argument: None.
 * Returns: integer, either 0 (false) or 1 (true) 
 */
int isNewResultValid() {
	// The historic data first starts as 0 (zero) so overide first input to be valid
	if (test_frequency < 1) {
		temperature_previous = temperature;
		humidity_previous = humidity;
		test_frequency += 1;
		return 1;
	}

	// If delta value is exceeds then 5 return false
	// This means there is a bigger then 5 difference (unrealistic) in measurements
	if (
		(temperature - temperature_previous) < -5 ||
		(temperature - temperature_previous) > 5
	) {
		return 0;
	}

	if (  
		(humidity - humidity_previous) < -5 ||
		(humidity - humidity_previous) > 5
	) {
		return 0;
	}

	// When data is found plausible save it for later reference
	temperature_previous = temperature;
	humidity_previous = humidity;

	return 1;
	
}

int main(void)
{	
	/* SETUP LCD DISPLAY */
	lcd_init();			// Initialize LCD
	lcd_clrscr();		// Clear LCD
	lcd_goto(0);		// Put cursor at start

	/* SETUP LED DRIVER & MATRIX */
	// Initialize LED driver
	max7219_init();  
	// Init led metrix
	max7219_shutdown(0, 1);		//power on
	max7219_test(0, 0); 		//test mode off
	max7219_decode(0, 0); 		//use led matrix
	max7219_intensity(0, 15); 	//intensity
	max7219_scanlimit(0, 7); 	//set number of digit to drive
	// clear led matrix
	for(uint8_t column = 0; column < 8; column++)
	{
		max7219_digit(0, column, 0); // set every column to 0b00000000
	}

	/* SETUP ARDUINO PINS */
	DDRD |= (1 << SPEAKER_PIN);

	/* SETUP TIMER */
	TCCR1B |= (1 << CS12) | (1 << CS10);  // Set prescaler to 1024
	TIMSK1 |= (1 << TOIE2);  // Overflow interrupt enabled when timers overflow
	sei();  // Switch interrupts on

	// Assert correct user config
	// If it doesnt hook on the conditional, the while loop is allowed to run
	// Normally the lower limit can't be over the upper limit
	if (TEMP_LIMIT_MIN > TEMP_LIMIT_MAX || HUM_LIMIT_MIN > HUM_LIMIT_MAX) {
		lcd_puts("Config error:");  // Display error type
		lcd_goto(0x40);
		lcd_puts("Limit MIN > MAX");  // Display specific error cause
		current_animation = 1;
		while(1){}  // Stay here untill problem is resolved		
	} else {
		while(1){
			// Fetch temp & hum from sensor
			if(dht_gettemperaturehumidity(&temperature, &humidity) != -1) {
				checkStats(temperature, humidity);
			} else {  // when fetch failes display corresponding error
				lcd_puts("Input Error:"); 
				lcd_goto(0x40);
				lcd_puts("Bad sensor data.");
				current_animation = 1;
			}
		}
	}	

	return 0;
}