#ifndef _TOUCH_H
#define _TOUCH_H

#define USE_ASM_TOUCH
#define FAST_TOUCH_ISR

#ifndef COMP_AS

//From backend.S, or here.  These use the SREG, r24 for return value and r0 as scratch if using backend version.
uint8_t TouchTest5();
uint8_t TouchTest6();
uint8_t TouchTest7();

extern volatile uint8_t touchvals[3];

//Perform next logical touch step.
void TouchNext();

#endif
#endif
