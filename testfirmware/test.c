/*


Power notes:
 * Enabling PLL on T1 costs a ton of power, ~4mA.
 * Kicking frequency from 8 to 16 MHz takes about 2mA.
 * Using T1 AT ALL takes ~.7mA.
 * Using anything other than idle sleep modes takes juice.
 * Enabling timer output to speaker doesn't add anything.
 * Proc going, even at IDLE takes up 1.2mA.
 * Could have saved more power if we had thought to hook speaker to T0.

TODO:
 * Examine possibility of a manual bridge mode.  Current bridge mode uses a lot of power to keep speaker in center.
   Should be possible to hold one wire high while PWMing the other and vice versa.  Would use MUCH less power when quiet.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "touch.h"

//#define BRIDGE_MODE  //Uses a lot more power but is louder.
//#define USE_SPI
#define USE_GPIOR0   //Uses the General Purpose IO Register for state-base stuff.  This helps with speed and memory usage as well as register overhead in interrupts.

void delay_ms(uint32_t time) {
  uint32_t i;
  for (i = 0; i < time; i++) {
    _delay_ms(1);
  }
}

#define NOOP asm volatile("nop" ::)

volatile unsigned char ThisCharToSend = 0;
unsigned char rcnt;

static void sendchr( char c )
{
	while( ! (USISR & (1<<USIOIF) ) );
	USISR |= 1<<USIOIF;
	USIDR = c;
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
	USICR = (1<<USIWM0) | (1<<USICS1);// | (1<<USICS0) | (1<<USICLK);

	/*PortB:
	  B0: MOSI      (Input)
	  B1: MISO      (Output)
	  B2: SPI Clock (Input)
	  B3: LED	(Output) */
	DDRB |= (1<<1);
	DDRB &= 0xF2;
}


int freq = 1;
int times;
uint8_t inc;
int sfreq = 0;
int sfoverride = 0;

int  cpl;

#ifdef USE_GPIOR0
#define SET_MUTE        {GPIOR0|=_BV(0);}
#define CLEAR_MUTE      {GPIOR0&=~_BV(0);}
#define GET_MUTE        (GPIOR0&_BV(0))
#define SET_WASMUTE     {GPIOR0|=_BV(1);}
#define CLEAR_WASMUTE   {GPIOR0&=~_BV(1);}
#define GET_WASMUTE     (GPIOR0&_BV(1))
#else
volatile uint8_t mute, wasmute;
#define SET_MUTE  { mute = 1; }
#define CLEAR_MUTE { mute = 0; }
#define GET_MUTE  (mute)
#define SET_WASMUTE     {wasmute = 1;}
#define CLEAR_WASMUTE   {wasmute = 0;}
#define GET_WASMUTE     (wasmute)
#endif

uint8_t Sine( uint16_t w ) { w>>=3; if( w & 0x100 ) return 255-(w&0xff); else return (w&0xff); }

uint8_t nextocr1d;
uint8_t ocronread;
uint16_t samples;

ISR( TIMER1_OVF_vect, ISR_NAKED )
{
	asm volatile ( "\n"
"		push r0\n"
"		in r0, 0x3f\n"
"		push r0\n"
"		push r24\n"
"		push r25\n" );

	OCR1D = nextocr1d;
	if( OCR1D & 0x80 )  //Cannot safely operate unless the next is far enough out.
	{
		ocronread = OCR1D;
		TouchNext();
	}

	samples++;

	if( GET_MUTE )
	{
		if( GET_WASMUTE )
		{
			TouchNext();
		}
		else
		{
			nextocr1d = 0;
			SET_WASMUTE
			TCCR1C = 0; //Actually disable PWM
			TCCR1E = 0; 
		}
	}
	else
	{
		if( GET_WASMUTE )
		{
			//Re-enable the PWM
			#ifdef BRIDGE_MODE
				TCCR1C = _BV(PWM1D) | _BV(COM1D0);
				TCCR1E =  _BV(5) | _BV(4); // | _BV(4); No /OC1D.  We don't want to do a full autobridge because we don't have an inductor on the speaker.
			#else
				TCCR1C = _BV(PWM1D) | _BV(COM1D1);
				TCCR1E =  _BV(5); // | _BV(4); No /OC1D.  We don't want to do a full autobridge because we don't have an inductor on the speaker.
			#endif
			CLEAR_WASMUTE
		}

asm volatile( 
"		push r18\n"
"		push r19\n" );

		//XXX Do awesome synth stuff here.
		cpl+=sfreq;  //64 = 4 per, @ 8kHz so SPS is 32kSPS
		nextocr1d = Sine( cpl );

asm volatile( 
"		pop r19\n"
"		pop r18\n" );
	}

cleanup:
	asm volatile( "\n"
"		pop r25\n"
"		pop r24\n"
"		pop r0\n"
"		out 0x3f, r0\n"
"		pop r0\n"
"		reti\n" );

}

int main()
{
	unsigned char i;
	int8_t s1, s2, s3, s4, s5;
	
	DDRB = 3;  //LEDs output

	cli();

	//Setup clock
	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/
	OSCCAL = 0x80;  //0xFF;

	
#ifdef USE_SPI
	setup_spi();
#endif

	sei();


	DDRB |= 3;  //LEDs output

	//Timer 1 was mostly used for the speaker.
	DDRB |= _BV(4) | _BV(5); //speaker.  
	TCCR1A = 0;
	TCCR1B = _BV(CS10); //_BV(CS11) | _BV(CS12); // | _BV(CS13);
#ifdef BRIDGE_MODE
	TCCR1C = _BV(PWM1D) | _BV(COM1D0);
	TCCR1E =  _BV(5) | _BV(4); // | _BV(4); No /OC1D.  We don't want to do a full autobridge because we don't have an inductor on the speaker.
#else
	TCCR1C = _BV(PWM1D) | _BV(COM1D1);
	TCCR1E =  _BV(5); // | _BV(4); No /OC1D.  We don't want to do a full autobridge because we don't have an inductor on the speaker.
#endif
	TCCR1D = 0;
	OCR1C = 0xff;  //Set top
	OCR1D = 0x00;


//	PLLCSR = _BV(PCKE) | _BV(PLLE); //Enable PLL - makes max speed 263.7kHz.  WARNING: Adds ~4mA.
	PLLCSR = 0; //Don't use PLL. - max speed 33.35kHz
	TIMSK |= _BV(TOIE1);// | _BV(OCIE1D); //Enable overflow interrupt. as well as the D-match interrupt.

//	sfreq = 150;

	set_sleep_mode(SLEEP_MODE_IDLE);
	//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//set_sleep_mode(SLEEP_MODE_ADC);
	cli();
	sleep_enable();
	sei();


	
	//Pullup bits for inputs.
	DDRB &= ~( _BV(2) | _BV(3) | _BV(6) );
	DDRA &= ~( _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) );
	PORTB |= _BV(2) | _BV(3) | _BV(6);
	PORTA |= _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4);



	uint8_t buttons;
	while(1)
	{
		//buttons++;
		buttons = ~( (PINA&0x1f) | ((PINB & _BV(2)) << 3) | ((PINB & _BV(3))<<3) | ((PINB&_BV(6))<<1));
#ifdef USE_SPI
		buttons &= 0x0f;
#endif
		if( buttons & 0x02 ) { PORTB |= _BV(1); } else { PORTB &= ~_BV(1); }
		if( buttons & 0x01 ) { PORTB |= _BV(0); } else { PORTB &= ~_BV(0); }

		sleep_cpu();

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

		if( sfreq == 0 )
			SET_MUTE
		else
			CLEAR_MUTE

		sleep_cpu();

		CalcTouch();  //Calculates amplitude and angle of button press.

		sleep_cpu();

		if( ( calced_amplitude > 3 && sfoverride ) || ( calced_amplitude > 7 ) ) //This provides a little debouncing.  >5 to start, if already started, needs to maintain >3
		{
			sfoverride = 100 + calced_angle;
		}
		else
		{
			sfoverride = 0;
		}

		sleep_cpu();

		//Ring check
		{
			static uint8_t ringstate;
			static uint8_t last_ring_place;
			static uint8_t ringaccum;
			uint8_t trigger_ring = 0;

			if( buttons == 0 )
			{
				if( calced_amplitude > 3 )
				{
					if( ringstate == 0 )
					{
						last_ring_place = calced_angle;
						ringaccum = 0;
						ringstate = 1;
					}
					else
					{
						int16_t delta = (int16_t)calced_angle - (int16_t)last_ring_place;
						if( delta > 90 ) delta -= 180;
						if( delta <-90 ) delta += 180; //Handle wrap-around.
						ringaccum += delta;
						if( ringaccum > 230 ) ringaccum = 0;
						if( ringaccum > 180 ) { trigger_ring = 1; ringaccum -= 180; }
						last_ring_place = calced_angle;
					}
				}
				else
				{
					ringstate = 0;
				}
				
			}
			else
			{
				ringstate = 0;
			}


			static uint8_t play_ring_state = 0;
			static uint16_t ring_start_time = 0;
			if( trigger_ring )
			{
				play_ring_state = 1;
				ring_start_time = samples;
			}
			if( play_ring_state )
			{
				int16_t difftime = ( samples - ring_start_time );
				if( difftime < 15000 )
				{
					//XXX Do a coin sound here.
					sfoverride = ((difftime>>7)&0xf0) + 300;
				}
				else
				{
					play_ring_state = 0;
				}
			}
		}
		
	}
}


