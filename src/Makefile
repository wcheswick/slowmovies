
BIN=$${HOME}/bin

CFLAGS+=-I/opt/local/include -I/usr/local/include -Wno-implicit-function-declaration
LDFLAGS+=-L/usr/local/lib -L/opt/local/lib
LDFLAGS+=-lopencv_highgui -lopencv_core -lopencv_imgcodecs  -lopencv_videoio
LDFLAGS+=-lopencv_video -lopencv_videostab -lopencv_imgproc

all::	splitmovie

splitmovie:	splitmovie.c
	${CC} ${CFLAGS} ${LDFLAGS} splitmovie.c -o $@

clean::
	rm -f *.o splitmovie

install::	${BIN}/splitmovie ${BIN}/surfside \
		${BIN}/fetchslowmovies ${BIN}/slowdefs

${BIN}/splitmovie:	splitmovie
	cp $< $@

${BIN}/surfside:	surfside
	-mv -f $@ ${BIN}/OLD$<
	cp $< $@

${BIN}/setwallpaper:	setwallpaper
	cp $< $@

${BIN}/fetchslowmovies:	fetchslowmovies
	cp $< $@

${BIN}/slowdefs:	slowdefs
	cp $< $@
