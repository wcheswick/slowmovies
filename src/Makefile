BIN=$${HOME}/bin

CFLAGS+=-I/opt/local/include -I/usr/local/include # -Wno-deprecated-declarations # -static
LDFLAGS+=-L/usr/local/lib -lavcodec -lavutil -lavformat -lswscale -lpthread # -static

all::	avdecode

# This compiles with warning of deprecated calls.  Don't care at the moment.

avdecode:	avdecode.o
	${CC} ${CFLAGS} avdecode.o ${LDFLAGS} -o $@

clean::
	rm -f *.o avdecode

install::	${BIN}/avdecode

${BIN}/avdecode:	avsavdecodeplit
	cp $< $@

test::		avdecode
#	avdecode -f 500 -s 5 -c 10  x
	mkdir -p /var/tmp/test/
	(cd /var/tmp/test; ls | xargs rm -f)
	./avdecode -f 500 -s 1 -c 10 /r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 \
		/var/tmp/test/frame_%08d.pnm


#test::		avdecode
#	avdecode -s 5 -c 10 \
#		/r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 \
#		/tmp/frames%09d
