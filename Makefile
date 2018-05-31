BIN=${HOME}/bin

all::	${BIN}/addmovie ${BIN}/fetchslowmovies ${BIN}/playsm

${BIN}/addmovie:	addmovie
	cp $> $@

${BIN}/fetchslowmovies:	fetchslowmovies
	cp $> $@

${BIN}/playsm:	playsm
	cp $> $@
