#ifndef _TOUCH_H
#define _TOUCH_H

//From backend.S.  These use the SREG, r24 and r25.
uint8_t TouchTest5();
uint8_t TouchTest6();
uint8_t TouchTest7();

extern volatile uint8_t touchvals[3];

//Perform next logical touch step.
void TouchNext();

#endif
