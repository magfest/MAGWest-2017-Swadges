//Copyright 2011 Charles Lohr (CC3.0 BY)
//Based roughly off of http://www.youtube.com/watch?v=b71Vkq8dyf8 / http://www.artem.ru/cgi-bin/news?c=v&id=738

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

//#define USE_SPI
#define TIMER1RUNNING
void delay_ms(uint32_t time) {
  uint32_t i;
  for (i = 0; i < time; i++) {
    _delay_ms(1);
  }
}

#define NOOP asm volatile("nop" ::)

volatile unsigned char ThisCharToSend = 0;
unsigned char rcnt;

ISR( USI_OVF_vect )
{
	USIDR = ThisCharToSend;
	ThisCharToSend = 0;
	USISR |= (1<<USIOIF); 
}

static void sendchr( char c )
{
	ThisCharToSend = c;
	while( ThisCharToSend );
}

#define sendstr( s ) {\
	for( rcnt = 0; s[rcnt] != 0; rcnt++ ) \
		sendchr( s[rcnt] ); \
	sendchr( 0x00 ); }\

static void sendhex1( unsigned char i )
{
	sendchr( (i<10)?(i+'0'):(i+'A'-10) );
}
static void sendhex2( unsigned char i )
{
	sendhex1( i>>4 );
	sendhex1( i&0x0f );
}
static void sendhex4( unsigned int i )
{
	sendhex2( i>>8 );
	sendhex2( i&0xFF);
}


static void setup_spi( void )
{
	/* Slave mode */
	USICR = (1<<USIWM0) | (1<<USICS1) | (1<<USIOIE);// | (1<<USICS0) | (1<<USICLK);

	/*PortB:
	  B0: MOSI      (Input)
	  B1: MISO      (Output)
	  B2: SPI Clock (Input)
	  B3: LED	(Output) */
	DDRB |= (1<<1);
	DDRB &= 0xF2;
}

void SetupHSTimer()
{
	TCCR0A = _BV(TCW0);
	TCCR0B = (1<<CS10); //no prescaling

//	TCCR1B |= (1<<CS12); //If uncommented, is /1024.
}


volatile unsigned short TimerAtTrigger;

//";		push r0 \n"
//";		in	r0, 0x3f\n" //Backup SREG
//";		pop r0 \n"
//";		out 0x3f, r0\n"

//#define FAST_ISR

#ifdef FAST_ISR
ISR( PCINT_vect, ISR_NAKED )
{
	asm volatile( "\n"
"       push r0\n"
"       in	r0, 0x3f\n"
"		push r24 \n"
"		in	r24, %[tcnt0l]\n"
"		sts TimerAtTrigger, r24\n"
"		in	r24, %[tcnt0h]\n"
"		sts TimerAtTrigger+1, r24\n"
"		in	r24, %[gimsk]\n"
"		andi r24, 0xEF\n"
"		out	%[gimsk], r24\n"
"		pop r24 \n"
"       out	0x3f, r0\n"
"       pop r0\n"
"		reti \n"
	: 
	: [tcnt0l] "I" (_SFR_IO_ADDR(TCNT0L)), [tcnt0h] "I" (_SFR_IO_ADDR(TCNT0H)), [gimsk] "I" (_SFR_IO_ADDR(GIMSK))
	);
}
#else
ISR( PCINT_vect )
{
	uint16_t i = TCNT0L;
	i |= ((unsigned int)TCNT0H << 8);

	GIFR |= _BV(5);
//	GIMSK &= ~_BV(4);     //Turn interrupt back off.
	GIMSK &= ~_BV(5);     //Turn interrupt back off.
	TimerAtTrigger = i; //Interrupts only happen every other cycle anyway.
}
#endif



unsigned short TimeTest( unsigned char pin )
{
	unsigned char i;
	unsigned short ret = 0;

	for( i = 0; i < 2; i++ )
	{
#ifdef USE_SPI
		USICR &= ~_BV(USIOIE);		//Temporarily disable serial comms.
#endif
		NOOP; NOOP; NOOP;

#ifdef TIMER1RUNNING
		TIMSK &= ~_BV(TOIE1); //Enable overflow interrupt.
#endif

		cli();
		PCMSK0 = _BV(pin);		//Select interrupt
		GIFR  |= _BV(5);
		GIMSK |= _BV(5);		//Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok.
		TimerAtTrigger = 0xFFFF;
		TCNT0H = 0;			//Start timer.
		TCNT0L = 0;			//Start timer.
		sei();


		DDRA &= ~_BV(pin);		//Set the pin
#if 1
		PORTA |= _BV(pin);
		while( !(PINA & _BV(pin)) );
#else
		set_sleep_mode(SLEEP_MODE_IDLE); // sleep mode is set here
		sleep_enable();
		PORTA |= _BV(pin);
		sleep_mode();                        // System actually sleeps here
		sleep_disable();                     // System continues execution here when watchdog timed out 
#endif

#ifdef TIMER1RUNNING
		TIMSK |= _BV(TOIE1); //Enable overflow interrupt.
#endif

#ifdef USE_SPI
		USICR |= _BV(USIOIE);
#endif

		PCMSK0 = 0;
		GIMSK &= ~_BV(5);		//Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok.

		DDRA |=  _BV(pin);
		PORTA &= ~_BV(pin);


		if( TimerAtTrigger == 0xFFFF )
		{
			return 0xFFFF;
		}
		ret += TimerAtTrigger;

		NOOP; NOOP; NOOP;
	}

	return ret;
}


unsigned short Biases[8];

unsigned short GetTime( unsigned char bit )
{
	unsigned short rr = TimeTest( bit );
	if( rr == 0xffff ) return 0xff;
	rr -= Biases[bit];
	if( rr >= 0x8000 ) return 0;
	if( rr > 0xff ) return 0xff;
	return rr;
/*
	if( rr == 0xFFFF ) return 0xFFFF;
	if( rr > 0x8000 ) return 0;
	return rr;
*/
}

int freq = 1;
int times;
uint8_t inc;
int sfreq = 0;
int sfoverride = 0;

int  cpl;
int mute;
uint8_t Sine( uint16_t w ) { w>>=3; if( w & 0x100 ) return 255-(w&0xff); else return (w&0xff); }

ISR( TIMER1_OVF_vect )
{
	static int ticktime = 0;
	ticktime = 0;
	cpl+=sfreq;  //64 = 4 per, @ 8kHz so SPS is 32kSPS
	OCR1D = Sine( cpl );
}

int main()
{
	unsigned char i;
	unsigned short s1, s2, s3, s4, s5;
	
	DDRB = 3;  //LEDs output

	cli();

	//Setup clock
	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/
	//OSCCAL = 0xff;//0xFF;

	SetupHSTimer();   //TIMER0  (For touch)

#ifdef USE_SPI
	setup_spi();
#endif

	sei();

	//Initialize all touch buttons
	Biases[5] = TimeTest(5); _delay_us(100);
	Biases[6] = TimeTest(6); _delay_us(100);
	Biases[7] = TimeTest(7); _delay_us(100);
	_delay_ms(10);

	//Get baselines.
	Biases[5] = TimeTest(5); _delay_us(100);
	Biases[6] = TimeTest(6); _delay_us(100);
	Biases[7] = TimeTest(7); _delay_us(100);

#if 0
#ifdef USE_SPI

	Biases[5] = TimeTest(5);
		_delay_us(100);
	Biases[6] = TimeTest(6);
		_delay_us(100);
	Biases[7] = TimeTest(7);
		_delay_us(100);
	_delay_ms(10);
	Biases[5] = TimeTest(5);
		_delay_us(100);
	Biases[6] = TimeTest(6);
		_delay_us(100);
	Biases[7] = TimeTest(7);
		_delay_us(100);

	while(1)
	{
		_delay_us(100);
		s1 = GetTime( 5 );
		_delay_us(100);
		s2 = GetTime( 6 );
		_delay_us(100);
		s3 = GetTime( 7 );
		_delay_us(100);
		sendhex2( s1 );
		sendchr( ',' );
		sendhex2( s2 );
		sendchr( ',' );
		sendhex2( s3 );
		sendstr( "\n" );
	}
#endif
#endif

	DDRB |= 3;  //LEDs output
	
	//Pullup bits for inputs.
	DDRB &= ~( _BV(2) | _BV(3) | _BV(6) );
	DDRA &= ~( _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) );
	PORTB |= _BV(2) | _BV(3) | _BV(6);
	PORTA |= _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4);

	DDRB |= _BV(4) | _BV(5); //speaker.
	TCCR1A = 0;
	TCCR1B = _BV(CS11) | _BV(CS10);
	TCCR1C = _BV(PWM1D) | _BV(COM1D1);
	TCCR1D = 0;
	TCCR1E = 0;
	OCR1C = 0xff;  //Set top
	PLLCSR = _BV(PCKE) | _BV(PLLE); //Don't use PLL.
	TIMSK |= _BV(TOIE1); //Enable overflow interrupt.

#if 0
	while(1)
	{
		s1 = GetTime( 5 );
		_delay_us(1000);
		s2 = GetTime( 6 );
		_delay_us(100);

		sendhex2(s1);
		sendchr(32);
		sendhex2(s2);
		sendchr(10);

	}
#endif


	uint8_t buttons;
	while(1)
	{
		//buttons++;
		buttons = ~( (PINA&0x1f) | ((PINB & _BV(2)) << 3) | ((PINB & _BV(3))<<3) | ((PINB&_BV(6))<<1));
		if( buttons & 0x02 ) { PORTB |= _BV(1); } else { PORTB &= ~_BV(1); }
		if( buttons & 0x01 ) { PORTB |= _BV(0); } else { PORTB &= ~_BV(0); }

		if( sfoverride ) sfreq = sfoverride;
		else if( buttons & 1 ) sfreq = 159;
		else if( buttons & 2 ) sfreq = 178;
		else if( buttons & 4 ) sfreq = 212;
		else if( buttons & 8 ) sfreq = 238;
		else if( buttons & 16 ) sfreq = 283;
		else if( buttons & 32 ) sfreq = 317;
		else if( buttons & 64 ) sfreq = 357;
		else if( buttons & 128 ) sfreq = 424;
		else sfreq = 0;

		s1 = GetTime( 5 );
		_delay_us(1000);
		s2 = GetTime( 6 );
		_delay_us(1000);
		s3 = GetTime( 7 );
		_delay_us(1000);

		if( s1 > 5)
			sfoverride = 150;
		else if( s2 > 5 )
			sfoverride = 160;
		else if( s3 > 5 )
			sfoverride = 170;
		else
			sfoverride = 0;
//		sendhex2(s1);
//		sendchr(10);

/*		s2 = GetTime( 6 );
		_delay_us(1000);
		s3 = GetTime( 7 );
		_delay_us(1000);
/*

		int maximumtouch = 0;
		int angle = 0;

		if( s1 < s2 && s1 < s3 )
		{
			//S1 is minimum (baseline)
			s2 -= s1;
			s3 -= s1;

			//Compare S2/S3
			if( s2 > s3 )
			{
				//S2 is max.
				maximumtouch = s2;
				angle = 0 + (30 * (int16_t)s3) / s2;
			}
			else
			{
				//S3 is max.
				maximumtouch = s3;
				angle = 30 + (30 * (int16_t)s2) / s3;
			}
		}
		else if( s2 < s3 )
		{
			s1 -= s2;
			s3 -= s2;
			if( s1 > s3 )
			{
				maximumtouch = s1;
				angle = 60 + (30 * (int16_t)s3) / s1;
			}
			else
			{
				maximumtouch = s3;
				angle = 90 + (30 * (int16_t)s1) / s3;
			}

		}
		else
		{
			s2 -= s3;
			s1 -= s3;
			if( s1 > s2 )
			{
				maximumtouch = s1;
				angle = 60 + (30 * (int16_t)s2) / s1;
			}
			else
			{
				maximumtouch = s2;
				angle = 90 + (30 * (int16_t)s1) / s2;
			}
		}

		if( maximumtouch > 4 )
		{
			sfoverride = 180 + angle;
		}
		else
		{
			sfoverride = 0;
		}
*/

	}
}


