all : halftone.kicad_mod halftoneSilk.kicad_mod

halftone : halftone.c
	gcc -o $@ $^ -lm

halftone.kicad_mod : halftone
	./$^ > $@

halftoneSilk.kicad_mod : halftone
	./$^ F.SilkS > $@

clean :
	rm -rf *.o *~ halftone halftone.kicad_mod

