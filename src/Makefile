BIN=${HOME}/bin

CFLAGS+=-I/opt/local/include -I/usr/local/include -Wno-implicit-function-declaration
LDFLAGS+=-L/usr/local/lib -L/opt/local/lib
LDFLAGS+=-lopencv_highgui -lopencv_core -lopencv_videoio -lopencv_imgcodecs
LDFLAGS+=-lopencv_video -lopencv_videostab -lopencv_imgproc -lm

all::	splitmovie

splitmovie:	splitmovie.c
	${CC} ${CFLAGS} ${LDFLAGS} splitmovie.c -o $@

clean::
	rm -f *.o splitmovie

install::	${BIN}/splitmovie

${BIN}/splitmovie:	splitmovie
	cp splitmovie $@


test::	splitmovie
	./splitmovie -S -s 2 -f 0 -c 10 /r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 /var/tmp
