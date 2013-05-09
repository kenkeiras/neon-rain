all:
	gcc -o neon-rain neon-rain.c -L/usr/X11R6/lib -lX11 -lGL -lGLU -lglut -lm -Wall

install:
	install neon-rain /usr/lib/xscreensaver/
