#include <avr/io.h>

int main() {
  DDRD |= 1<<1;
  while(1==1) {
    PORTD |= 1<<1;
    PORTD &= ~(1<<1);
  }
}
