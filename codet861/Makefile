
all : test.hex burn
PART=attiny861
PROGPART=t861

CFLAGS=-g -Os -mmcu=$(PART) -DF_CPU=8000000UL
CC=avr-gcc

test.elf : test.o
	avr-gcc -I $(CFLAGS) -mmcu=$(PART) -Wl,-Map,test.map -o $@ $^ -L/usr/lib/binutils/avr/2.18
	avr-objdump -S test.elf > test.lst

test.o : test.c
	avr-gcc -g $(CFLAGS) -mmcu=$(PART) -c -o test.o test.c

test.hex : test.elf
	avr-objcopy -j .text -j .data -O ihex test.elf test.hex 

burn : test.hex
	avrdude -c usbtiny -p $(PROGPART) -U flash:w:test.hex

burnfuses :
#	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x4E:m -U hfuse:w:0xDF:m #external, high frequency crystal
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m #default


readfuses :
	avrdude -c usbtiny -p $(PROGPART) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

clean : 
	rm -f *~ high.txt low.txt test.hex test.map test.elf test.o







