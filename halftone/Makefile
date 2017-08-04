all : halftone.kicad_mod

halftone : halftone.c
	gcc -o $@ $^ -lm

halftone.kicad_mod : halftone
	./$^ > $@

clean :
	rm -rf *.o *~ halftone halftone.kicad_mod

