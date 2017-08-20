

#if 1


/**** Touch functions ****/


//	unsigned char ret = 0; \
	unsigned char initialt1; \
	PCMSK0 = _BV(pin);		/*Select interrupt*/ \
	GIFR  |= _BV(5);		/*Wipe out any pending flags*/ \
	DDRA &= ~_BV(pin);		/*Release the pin to float high. */ \
	GIMSK |= _BV(5);		/*Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok. */ \
	PORTA |= _BV(pin);		/*Enable weak pullup */ \
	initialt1 = TCNT1; \
	/*while( !(PINA & mask ) ); *//*XXX Tricky... bug in processor? SOMETIMES?  Can't use _BV(pin)! */ \
	while( !(PINA & _BV(pin)) ); \
	ret = TimerAtTrigger - initialt1; \
	DDRA |=  _BV(pin);     /*Lock the pin back low.*/ \
	PORTA &= ~_BV(pin); \
	return ret; \


#define GEN_TOUCHTEST(bit) \
		ldi	r24, (1<<bit) $\
		out	0x23, r24 $ \
		in	r24, 0x3a $ \
		ori	r24, (1<<bit) $ \
		out	0x3a, r24 $ \
		cbi	0x1a, bit $ \
		in	r24, 0x3b $ \
		ori	r24, (1<<bit) $ \
		out	0x3b, r24 $ \
		sbi	0x1b, bit $ \
		in	r25, 0x2e /*TCNT1*/ $ \
		sbis	0x19, bit $ \
		rjmp	.-4 $ \
		lds	r24, TimerAtTrigger $ \
		sbi	0x1a, bit $ \
		cbi	0x1b, bit $ \
		sub	r24, r25 $ \
		ret

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

