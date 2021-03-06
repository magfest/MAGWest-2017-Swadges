//Copyright 2011 Charles Lohr (CC3.0 BY)
//Based roughly off of http://www.youtube.com/watch?v=b71Vkq8dyf8 / http://www.artem.ru/cgi-bin/news?c=v&id=738

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

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


static void setup_clock( void )
{
	/*Examine Page 33*/

	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/

	//OSCCAL = 0xff;//0xFF;
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


void SetupSwitches()
{
//	DDRA  |= 0x8F;
//	PORTA &= 0x70;
	PORTA = 0x0;
}

volatile unsigned short TimerAtTrigger;

//";		push r0 \n"
//";		in	r0, 0x3f\n" //Backup SREG
//";		pop r0 \n"
//";		out 0x3f, r0\n"

#define FAST_ISR

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

	GIMSK &= ~_BV(PCIE0);     //Turn interrupt back off.
	TimerAtTrigger = i; //Interrupts only happen every other cycle anyway.
}
#endif


unsigned short TimeTest( unsigned char pin )
{
	unsigned char i;
	unsigned short ret = 0;


	cli();
	PCMSK0 = _BV(pin);		//Select interrupt
	GIMSK |= _BV(PCIE0);		//Turn on PCINT0
	TimerAtTrigger = 0xFFFF;
	sei();

	for( i = 0; i < 2; i++ )
	{
		USICR &= ~_BV(USIOIE);		//Temporarily disable serial comms.
		NOOP; NOOP; NOOP;

		cli();
		PCMSK0 = _BV(pin);		//Select interrupt
		GIFR |= _BV(PCIF);
		GIMSK |= _BV(PCIF);		//Turn on PCINT0  ok, actually this is a bug in their documentation. BUT that's ok.
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

		USICR |= _BV(USIOIE);
		PCMSK0 = 0;
		DDRA |=  _BV(pin);
		PORTA &= ~_BV(pin);


		if( TimerAtTrigger == 0xFFFF )
		{
			GIMSK &= ~_BV(PCIE0);     //Turn interrupt back off.
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
ISR( TIMER1_OVF_vect )
{
//	inc++;
//	if( inc == 0 )
//	{
//		freq++;
//	}

	OCR1D+=1;
//	OCR1D+=8;
}

int main()
{
	unsigned char i;
	unsigned short s1, s2, s3, s4, s5;
	
	cli();
	setup_clock();
	SetupSwitches();
	SetupHSTimer();   //TIMER0
	setup_spi();
	sei();

#if 1
/*
MCUCR &= ~_BV(PUD);
PORTA = 0x1f;
DDRA = ~0x1F;
DDRB = 0x01;
PORTB = _BV(2) | _BV(3) | _BV(6);
*/
while(1)
{
/*
	uint8_t buttons = PINA | ((PINB & _BV(2)) << 3) | ((PINB & _BV(3))<<3) | ((PINB&_BV(6))<<1);
	if ( buttons & 0x20 )
	{
		PORTB |= 3;
	}
	else
	{
		PORTB &=~3;
	}
	*/
	uint8_t buttons = 0xaa;
	sendhex2( buttons );
	sendstr( "\n" );
}

#endif


#if 0
	//TCCR1A = _BV(TCW0);
	//TCCR1B = (1<<CS10); //no prescaling

	DDRB |= _BV(4) | _BV(5); //speaker.

	TCCR1A = 0;
	TCCR1B = _BV(CS10);
	TCCR1C = _BV(PWM1D) | _BV(COM1D1);
	TCCR1D = 0;
	TCCR1E = 0;
	OCR1C = 0xff;  //Set top
	PLLCSR = _BV(PCKE) | _BV(PLLE); //Don't use PLL.
	TIMSK |= _BV(TOIE1); //Enable overflow interrupt.
while(1);
while(1)
{
	_delay_us(600);
	PORTB = 0x10;
	_delay_us(600);
	PORTB = 0x20;
}
while(1);
#endif

//Below here: Touch sensing.

	//MCUCR |= _BV(PUD);

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

/*
	while(1)
	{
		int mark;
		PORTA = 0;
		DDRA &= ~_BV(7);
		mark = 0;
		while( !( PINA & _BV(7) ) ) mark++;
		DDRA |= _BV(7);
		sendhex4( mark );
		sendstr( "\n: ");
		_delay_ms(1);
	}
*/
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
}


