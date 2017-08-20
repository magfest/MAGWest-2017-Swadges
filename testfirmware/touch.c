#include <avr/io.h>
#include <avr/interrupt.h>
#include "touch.h"

volatile unsigned char TimerAtTrigger;

//";		push r0 \n"
//";		in	r0, 0x3f\n" //Backup SREG
//";		pop r0 \n"
//";		out 0x3f, r0\n"


#ifdef FAST_TOUCH_ISR

ISR( PCINT_vect, ISR_NAKED )
{
	asm volatile( "\n"
"		push r24 \n"
"		in	r24, %[tcnt1]\n"
"		sts TimerAtTrigger, r24\n"
"		in  r24, %[gimsk]\n"
"		andi r24, ~(1<<5)\n"
"		out %[gimsk], r24\n"
"		pop r24 \n"
"		reti \n"
	: 
	: [tcnt1] "I" (_SFR_IO_ADDR(TCNT1)), [gimsk] "I" (_SFR_IO_ADDR(GIMSK))
	);
}
#else
ISR( PCINT_vect )
{
	TimerAtTrigger = TCNT1; //Interrupts only happen every other cycle anyway.
	//GIFR |= _BV(5);
	GIMSK &= ~_BV(5);     //Turn interrupt back off.
}
#endif



volatile uint8_t touchvals[3];
uint8_t curtouch = 5;

void TouchNext()
{
	if( curtouch == 5 )
	{
		touchvals[0] = TouchTest5();
		curtouch = 6;
	}
	else if( curtouch == 6 )
	{
		touchvals[1] = TouchTest6();
		curtouch = 7;
	}
	else
	{
		touchvals[2] = TouchTest7();
		curtouch = 5;
	}
}




//This is a terrible version of TouchTest, an 8-bit version.

/*
unsigned char TouchTest( unsigned char pin, unsigned char mask )
{
	unsigned char i;
	unsigned char ret = 0;
	unsigned char initialt1;
	PCMSK0 = mask;		//Select interrupt
	GIFR  |= _BV(5);
	DDRA &= ~mask;		//Release the pin to float high.
	GIMSK |= _BV(5);		//Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok.
	PORTA |= mask;
	initialt1 = TCNT1;
	while( !(PINA & mask) );
	ret = TimerAtTrigger - initialt1;
	DDRA |=  mask;     //Lock the pin back low.
	PORTA &= ~mask;
	return ret;
}
*/

#ifndef USE_ASM_TOUCH

#define TTGEN(pin) \
uint8_t TouchTest##pin( ) \
{ \
	unsigned char ret = 0; \
	unsigned char initialt1; \
	PCMSK0 = _BV(pin);		/*Select interrupt*/ \
	GIFR  |= _BV(5);		/*Wipe out any pending flags*/ \
	DDRA &= ~_BV(pin);		/*Release the pin to float high. */ \
	GIMSK |= _BV(5);		/*Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok. */ \
	PORTA |= _BV(pin);		/*Enable weak pullup */ \
	initialt1 = TCNT1; sei(); \
	/*while( !(PINA & mask ) ); *//*XXX Tricky... bug in processor? SOMETIMES?  Can't use _BV(pin)! */ \
	while( !(PINA & _BV(pin)) ); \
	ret = TimerAtTrigger - initialt1; \
	DDRA |=  _BV(pin);     /*Lock the pin back low.*/ \
	PORTA &= ~_BV(pin); \
	return ret; \
}
TTGEN(5);
TTGEN(6);
TTGEN(7);

#endif






