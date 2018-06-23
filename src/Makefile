BIN=${HOME}/bin

CFLAGS+=-I/opt/local/include -I/usr/local/include -Wno-implicit-function-declaration
LDFLAGS+=-L/usr/local/lib -L/opt/local/lib

CV_LDFLAGS!=pkg-config --libs --cflags opencv

#CV_LDFLAGS+=-lopencv_highgui -lopencv_core -lopencv_imgcodecs # -lopencv_videoio
#CV_LDFLAGS+=-lopencv_video -lopencv_videostab -lopencv_imgproc -lm

AV_LDFLAGS+=-lavcodec -lavutil -lavformat -lswscale -lpthread

all::	cv_split av_split

#-lopencv_imgcodecs -lopencv_videoio

cv_split:	split.o cv_io.o
	${CC} ${CFLAGS} ${CV_LDFLAGS} ${LDFLAGS}  split.o cv_io.o -o $@

av_split:	split.o av_io.o
	${CC} ${CFLAGS} ${AV_LDFLAGS} ${LDFLAGS} split.o av_io.o -o $@

av_io.o cv_io.o split.o:	vid_io.h

clean::
	rm -f *.o cv_split av_split

av_install::	${BIN}/av_split

cv_install::	${BIN}/cv_split

install::	${BIN}/cv_split ${BIN}/av_split

${BIN}/cv_split:	cv_split
	cp cv_split $@
	rm -f ${BIN}/smsplit
	ln ${BIN}/cv_split ${BIN}/smsplit

${BIN}/av_split:	av_split
	cp av_split $@
	rm -f ${BIN}/smsplit
	ln ${BIN}/av_split ${BIN}/smsplit

cv_test::	cv_split
	./cv_split -S -s 2 -f 0 -c 10 /r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 /var/tmp

av_test::	av_split
	./av_split -S -s 2 -f 0 -c 10 /r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 /var/tmp
