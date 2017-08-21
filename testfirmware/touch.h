#ifndef _TOUCH_H
#define _TOUCH_H

#define USE_ASM_TOUCH
#define FAST_TOUCH_ISR

#ifndef COMP_AS


#define CAL_BASELINE 0x10  //XXX This is hardware dependent!!! Look at this if you are having h/w cal problems.

//From backend.S, or here.  These use the SREG, r24 for return value and r0 as scratch if using backend version.
uint8_t TouchTest5();
uint8_t TouchTest6();
uint8_t TouchTest7();

extern volatile uint8_t touchvals[3];
extern volatile uint8_t filt_touchvals[3];

extern uint8_t calced_angle;  //In 2* degree increments, 0-180 = one circle.
extern int8_t calced_amplitude;

//Perform next logical touch step.
void TouchNext();

//Don't execute from within interrupt.  Uses touch information to calculate the calced angle and volume.
void CalcTouch();

#endif
#endif
