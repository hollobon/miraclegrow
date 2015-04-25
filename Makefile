SHELL=/bin/sh

#Macros

GL_LIB  = libMesaGL.a
GLU_LIB = libMesaGLU.a
GLUT_LIB= libglut.a
TK_LIB  = libMesatk.a
AUX_LIB = libMesaaux.a

#CFLAGS  = -O2 -funroll-loops -ffast-math -D_SVID_SOURCE -D_BSD_SOURCE -I/usr/X11R6/include -DSHM

CFLAGS  =  -O3 -funroll-loops -ffast-math -D_SVID_SOURCE -D_BSD_SOURCE -I/usr/X11R6/include -DSHM -Wall

MAKELIB = $(MESA)/mklib.ar-ruv

XLIBS = -L/usr/X11/lib -L/usr/X11R6/lib -lX11 -lXext -lSM -lICE

MESA	= /usr/local

INCDIRS = -I $(MESA)/include

GL_LIBS = -L $(MESA)/lib -lglut -lGLU -lGL -lm $(XLIBS) 

LIB_DEP = $(MESA)/lib/$(GL_LIB) $(MESA)/lib/$(GLU_LIB) $(MESA)/lib/$(GLUT_LIB)

OBJECTS = miraclegrow.o
HEADERS = leaf.h

default: miraclegrow

#Rules
%.o: %.c $(HEADERS)
	/usr/bin/gcc -c $(INCDIRS) $(CFLAGS) $< -o $@

miraclegrow: $(OBJECTS)
	gcc $(OBJECTS) $(GL_LIBS) -o $@
