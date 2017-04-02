BIN=$${HOME}/bin

install::	${BIN}/surfside ${BIN}/fetchslowmovies ${BIN}/slowdefs

${BIN}/surfside:	surfside
	-mv -f $@ ${BIN}/OLD$<
	cp $< $@

${BIN}/setwallpaper:	setwallpaper
	cp $< $@

${BIN}/fetchslowmovies:	fetchslowmovies
	cp $< $@

${BIN}/slowdefs:	slowdefs
	cp $< $@
