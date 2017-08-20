#define COMP_AS
#include "touch.h"

#ifdef USE_ASM_TOUCH


/**** Touch functions ****/


//The idea is an interrupt should happen somewhere in this sled here.  It will update TimerAtTrigger with the value in TCNT1.
//XXX TODO: Compare with/without nopsled.  It could save some code space.
#define NOPSLED
#ifdef NOPSLED
#define WAIT_CODE(bit) \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $ \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $ \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $ \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $ \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $ \
		nop $ nop $ nop $ nop $ nop $ nop $ nop $ nop $
#else
#define WAIT_CODE(bit) \
		in  r24, 0x19 $ \
		sbrs r24, bit $ \
		rjmp	.-6 $
#endif


#define GEN_TOUCHTEST(bit) \
		ldi	r24, (1<<bit) $\
		out	0x23, r24 $ \
		in	r24, 0x3a $ \
		ori	r24, 0x20 $ \
		out	0x3a, r24 $ \
		cbi	0x1a, bit $ \
		in	r24, 0x3b $ \
		ori	r24, 0x20 $ \
		out	0x3b, r24 $ \
		sbi	0x1b, bit $ \
		in	r0, 0x2e /*TCNT1, prserve the value so we can subtract it out later.*/ $ \
		sei $ \
		WAIT_CODE(bit) \
		lds	r24, TimerAtTrigger $ \
		sbi	0x1a, bit $ \
		cbi	0x1b, bit $ \
		sub	r24, r0 $ \
		ret


// The below code ordinarily would go below WAIT_CODE to make sure the PCINT is turned off, in case 
// our nopsled fails.
//		in	r24, 0x3b	  $  /* (read in GIMSK) */ \
//		andi	r24, 0xDF $  /* and out the bit for pin change */ \
//		out	0x3b, r24	  $  /* Write back to register */ \


.global TouchTest5
TouchTest5:
	GEN_TOUCHTEST(5)

.global TouchTest6
TouchTest6:
	GEN_TOUCHTEST(6)

.global TouchTest7
TouchTest7:
	GEN_TOUCHTEST(7)

#endif




/*

00000154 <TouchTest6>:
 154:	80 e4       	ldi	r24, 0x40	; 64
 156:	83 bd       	out	0x23, r24	; 35
 158:	8a b7       	in	r24, 0x3a	; 58
 15a:	80 64       	ori	r24, 0x40	; 64
 15c:	8a bf       	out	0x3a, r24	; 58
 15e:	d6 98       	cbi	0x1a, 6	; 26

 160:	8b b7       	in	r24, 0x3b	; 59
 162:	80 64       	ori	r24, 0x40	; 64
 164:	8b bf       	out	0x3b, r24	; 59
 166:	de 9a       	sbi	0x1b, 6	; 27
 168:	0e b4       	in	r0, 0x2e	; 46
 16a:	ce 9b       	sbis	0x19, 6	; 25
 16c:	fe cf       	rjmp	.-4      	; 0x16a <TouchTest6+0x16>
 16e:	80 91 77 00 	lds	r24, 0x0077
 172:	d6 9a       	sbi	0x1a, 6	; 26
 174:	de 98       	cbi	0x1b, 6	; 27
 176:	80 19       	sub	r24, r0
 178:	08 95       	ret

00000160 <TouchTest6>:
 160:	80 e4       	ldi	r24, 0x40	; 64
 162:	83 bd       	out	0x23, r24	; 35
 164:	8a b7       	in	r24, 0x3a	; 58
 166:	80 62       	ori	r24, 0x20	; 32
 168:	8a bf       	out	0x3a, r24	ori	r24, 0x20; 58
 16a:	d6 98       	cbi	0x1a, 6	; 26

 16c:	8b b7       	in	r24, 0x3b	; 59
 16e:	80 62       	ori	r24, 0x20	; 32
 170:	8b bf       	out	0x3b, r24	; 59
 172:	de 9a       	sbi	0x1b, 6	; 27
 174:	9e b5       	in	r25, 0x2e	; 46
 176:	ce 9b       	sbis	0x19, 6	; 25
 178:	fe cf       	rjmp	.-4      	; 0x176 <TouchTest6+0x16>
 17a:	80 91 77 00 	lds	r24, 0x0077
 17e:	d6 9a       	sbi	0x1a, 6	; 26
 180:	de 98       	cbi	0x1b, 6	; 27
 182:	89 1b       	sub	r24, r25
 184:	08 95       	ret


 */
