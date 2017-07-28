#include <avr/io.h>

int main() {
  DDRD |= 1<<1;
  DDRD |= 1<<2;
  PORTD &= ~(1<<1);
  PORTD &= ~(1<<2);

  DDRB |= 1<<3;
  PORTB &= ~(1<<3);
  PORTA &= ~(1);

  while(1==1) {
    DDRA |= 1;
    delay();
    DDRA &= ~(1);
    int i = 0;
    while(!(PINA & (1<<0))) {
      i++;
    }
    if (i > 28) {
      beep(i);
    }
  }
  return 0;
}

void beep(int freq)
{
  PORTD |= 1<<1;
  PORTD |= 1<<2;
  volatile unsigned int del = 2;
  while(del--) {
    for (int i = 0; i < freq*4; i++) {
      PORTB |= 1<<3;
    }
    for (int i = 0; i < freq*4; i++) {
      PORTB &= ~(1<<3);
    }
  }
  PORTD &= ~(1<<1);
  PORTD &= ~(1<<2);
}

void delay(void)
{
  volatile unsigned int del = 50;
  while(del--) {
  }
}
