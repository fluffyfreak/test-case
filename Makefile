
linux:
	gcc -I. -I/usr/include GLSLnoise.c -lglfw -lGLU -lGL -lm -o GLSLnoise

clean:
	rm -f GLSLnoise.o

distclean:
	rm -rf GLSLnoise.o GLSLnoise
