/*
max7219 lib 0x02

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef MAX7219_H_
#define MAX7219_H_

#include <avr/io.h>


//setup ports
#define MAX7219_DINDDR DDRB
#define MAX7219_DINPORT PORTB
#define MAX7219_DININPUT PB3
#define MAX7219_CLKDDR DDRB
#define MAX7219_CLKPORT PORTB
#define MAX7219_CLKINPUT PB5
#define MAX7219_LOADDDR DDRB
#define MAX7219_LOADPORT PORTB
#define MAX7219_LOADINPUT PB2

//setup number of chip attached to the board
#define MAX7219_ICNUMBER 1

//define registers
#define MAX7219_REGNOOP 0x00
#define MAX7219_REGDIGIT0 0x01
#define MAX7219_REGDIGIT1 0x02
#define MAX7219_REGDIGIT2 0x03
#define MAX7219_REGDIGIT3 0x04
#define MAX7219_REGDIGIT4 0x05
#define MAX7219_REGDIGIT5 0x06
#define MAX7219_REGDIGIT6 0x07
#define MAX7219_REGDIGIT7 0x08
#define MAX7219_REGDECODE 0x09
#define MAX7219_REGINTENSITY 0x0A
#define MAX7219_REGSCANLIMIT 0x0B
#define MAX7219_REGSHUTDOWN 0x0C
#define MAX7219_REGTEST 0x0F


//functions
extern void max7219_send(uint8_t icnum, uint8_t reg, uint8_t data);
extern void max7219_shutdown(uint8_t icnum, uint8_t value);
extern void max7219_intensity(uint8_t icnum, uint8_t value);
extern void max7219_test(uint8_t icnum, uint8_t value);
extern void max7219_scanlimit(uint8_t icnum, uint8_t value);
extern void max7219_decode(uint8_t icnum, uint8_t value);
extern void max7219_digit0(uint8_t icnum, uint8_t value);
extern void max7219_digit1(uint8_t icnum, uint8_t value);
extern void max7219_digit2(uint8_t icnum, uint8_t value);
extern void max7219_digit3(uint8_t icnum, uint8_t value);
extern void max7219_digit4(uint8_t icnum, uint8_t value);
extern void max7219_digit5(uint8_t icnum, uint8_t value);
extern void max7219_digit6(uint8_t icnum, uint8_t value);
extern void max7219_digit7(uint8_t icnum, uint8_t value);
extern void max7219_digit(uint8_t icnum, uint8_t digit, uint8_t value);
extern uint8_t max7219_getdigit0(uint8_t icnum);
extern uint8_t max7219_getdigit1(uint8_t icnum);
extern uint8_t max7219_getdigit2(uint8_t icnum);
extern uint8_t max7219_getdigit3(uint8_t icnum);
extern uint8_t max7219_getdigit4(uint8_t icnum);
extern uint8_t max7219_getdigit5(uint8_t icnum);
extern uint8_t max7219_getdigit6(uint8_t icnum);
extern uint8_t max7219_getdigit7(uint8_t icnum);
extern uint8_t max7219_getdigit(uint8_t icnum, uint8_t digit);
extern void max7219_init();

#endif
