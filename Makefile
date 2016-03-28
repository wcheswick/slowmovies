BIN=${HOME}/bin

CFLAGS+=-I/opt/local/include -Wno-implicit-function-declaration # -I/usr/local/include
LDFLAGS+=-L/opt/local/lib -L/usr/local/lib
LDFLAGS+=-lopencv_highgui -lopencv_video -lopencv_core -lopencv_imgcodecs
LDFLAGS+=-lopencv_videostab -lopencv_imgproc -lopencv_videoio

all::	splitmovie

splitmovie:	splitmovie.c
	${CC} ${CFLAGS} ${LDFLAGS} splitmovie.c -o $@

clean::
	rm -f *.o splitmovie

install::	${BIN}/splitmovie ${BIN}/setwallpaper ${BIN}/surfside \
		${BIN}/ptsm ${BIN}/prepmovie

${BIN}/ptsm:	ptsm
	cp $< $@

${BIN}/prepmovie:	prepmovie
	cp $< $@

${BIN}/surfside:	surfside
	cp $< $@

${BIN}/splitmovie:	splitmovie
	cp $< $@

${BIN}/setwallpaper:	setwallpaper
	cp $< $@
