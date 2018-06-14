BIN=${HOME}/bin

CFLAGS+=-I/opt/local/include -I/usr/local/include # -Wno-deprecated-declarations # -static
LDFLAGS+=-L/usr/local/lib -lavcodec -lavutil -lavformat -lswscale -lpthread # -static

P=avdecode

all::	${P}

${P}:	${P}.o
	${CC} ${CFLAGS} ${P}.o ${LDFLAGS} -o $@

clean::
	rm -f *.o ${P}

install::	${BIN}/${P}

${BIN}/avdecode:	${P}
	-mkdir -p ${BIN}
	cp ${P} $@

test::		${P}
	mkdir -p /var/tmp/test/
	(cd /var/tmp/test; ls | xargs rm -f)
	./${P} -f 500 -s 1 -c 10 /r/media/slowmovies/catalog/Ellivesdranreb/video.mp4 \
		/var/tmp/test/frame_%08d.pnm
