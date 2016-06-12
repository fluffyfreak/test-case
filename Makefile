#Makefile for Windows mingw32, Linux and MacOSX (gcc environments)

gcc -I. -I/usr/include GLSLnoise.c -lglfw -lGLU -lGL -lm -o GLSLnoise

clean:
	rm -f GLSLnoise.o

distclean:
	rm -rf GLSLnoise.o GLSLnoise
