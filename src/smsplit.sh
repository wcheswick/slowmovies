#!/bin/sh
#
prog=smsplit
usage="$prog [-c frame-count] [-f first-frame] [-s seconds-per-frame] [-S] movie output_directory"
#
#	Split a video file into frames, and feed out some or all of the
#	frame file names to stdout.  The frames are written in a cache
#	directory (if they aren't already there).

. smdefs	|| exit 99

frame_count=0
first_frame=0
last_frame=0
show_stats=
spf=10

while [ $# -gt 0 ]
do
	case "$1" in
	-c)	shift; frame_count=$1
		shift;;
	-f)	shift; first_frame=$1
		shift;;
	-l)	shift; last_frame=$1
		shift;;
	-s)	shift; spf=$1
		shift;;
	-S)	show_stats=true
		shift;;
	-*)	echo "$usage" 1>&2
		exit 1;;
	*)	break
	esac
done

if [ $# -eq 2 ]
then
	video_path="$1"
	output_directory="$2"
else
	echo "$usage" 1>&2
	exit 2
fi

movie=`echo "$video_path" | awk -v FS=/ '{print $(NF-1)}'`

WRKDIR=${WRKDIR:-${TMPDIR}/${prog}}
mkdir -p $WRKDIR/cache

OUT_CACHE_DIR="$WRKDIR/cache/$movie"

if [ ! -d "$OUT_CACHE_DIR" ]
then
	log "prog:	'$movie': unpacking"

	TMP_CACHE=$WRKDIR/incoming
	rm -rf $TMP_CACHE
	mkdir -p $TMP_CACHE

	if ffmpeg -hide_banner -v 8 -i "$video_path" -qscale:v 2 \
		$TMP_CACHE/frame%09d.jpeg
	then
		log "$prog:	`(cd $CACHE; ls | wc -l | 
			tr -d ' ')` frames extracted"
	else
		log "$prog: unpack failed"
		exit 10
	fi
	mv $TMP_CACHE "$OUT_CACHE_DIR"	|| exit 11
fi

cd "$OUT_CACHE_DIR"

total_frames=`ls | wc -l | tr -d ' '`

ls |
case "$first_frame" in		# skip leading frames, if desired
"")	cat -;;
0)	cat -;;
*)	sed "1,${first_frame}d"
	log "$prog: 	skipping ${first_frame} frames"
esac |
case $frame_count in		# limit frame count, if desired
"")	cat -;;
all)	cat -;;
*)	sed -n "1,${frame_count}"p
	log "$prog: 	limited to ${frame_count} frames"
esac |
while read frame
do
	cp $frame $output_directory
	echo "$output_directory/$frame"
	sleep $spf
done

log "$prog	exiting"
