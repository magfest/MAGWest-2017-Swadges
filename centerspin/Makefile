all : centerspin.kicad_mod

centerspin : centerspin.c
	gcc -o $@ $^ -lm

centerspin.kicad_mod : centerspin
	./$^ > $@

clean :
	rm -rf *.o *~ centerspin centerspin.kicad_mod

