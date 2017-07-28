DEVICE     = attiny2313a
PDEVICE    = t2313
CLOCK      = 8000000
PROGRAMMER = usbtiny
PORT	   = /dev/tty.usbmodem1421
SPEED      = 2
FILENAME   = main
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
LOWFUSE    = 0xE4
HIGHFUSE   = 0xDF
EXTFUSE    = 0xFF

all: clean build upload

build:
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf $(FILENAME).o
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

upload:
	avrdude -v -B $(SPEED) -p $(PDEVICE) -c $(PROGRAMMER) -U flash:w:$(FILENAME).hex:i 

clean:
	rm main.o
	rm main.elf
	rm main.hex

readfuses:
	avrdude -v -p $(PDEVICE) -c $(PROGRAMMER) -U lfuse:r:low_fuses.hex:h -U hfuse:r:high_fuses.hex:h -U efuse:r:ext_fuses.hex:h
	cat low_fuses.hex high_fuses.hex ext_fuses.hex

writefuses:
	avrdude -v -p $(PDEVICE) -c $(PROGRAMMER) -U lfuse:w:$(LOWFUSE):m -U hfuse:w:$(HIGHFUSE):m -U efuse:w:$(EXTFUSE):m
