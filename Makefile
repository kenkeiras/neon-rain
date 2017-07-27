all: neon-rain

empty_shader.frag.c: empty_shader.frag.glsl
	(echo 'const char* EMPTY_FRAG_SHADER = ""\n'; \
	 cat $< | sed -r 's/^(.*)$$/"\1\\n"/g'; \
	 echo ';' \
	) > $@

neon-rain: neon-rain.c empty_shader.frag.c
	gcc -o $@ $< -L/usr/X11R6/lib -lX11 -lGL -lGLU -lm -Wall

clean:
	rm -f empty_shader.frag.c neon-rain

install: neon-rain
	install $< /usr/lib/xscreensaver/
