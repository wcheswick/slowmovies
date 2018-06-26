#!/bin/sh
#
prog=sssplit
usage="$prog [-c frame-count] [-f first-frame] [-s seconds-per-frame] [-S] video ramfs_directory"
#
#	'movie' is a video file relative to our current directory.
#	'ramfs_directory' is a previously-allocated ram drive we stage
#	movie frames into.
#
#	We use ffmpeg to split the frames
#	off, but we want them slowly.  We stifle ffmpeg with a STOP signal
#	when we have enough, and CONT when we need more.  This is a race
#	condition I am not happy about, but it seems (mostly) tamed
#	by this routine, and gets me out of the software installation swamp
#	for tools like libav and opencv.
#
#	*** The consumer must delete the images as they are used, because we wait
#	for space in the ramdisk.

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
	ramfs_directory="$2"
else
	echo "$usage" 1>&2
	exit 2
fi

if [ ! -s "$video_path" ]
then
	log "$prog:	video file missing: $video_path"
	exit 3
fi

if [ ! -d $ramfs_directory ]
then
	log "$prog:	ramfs_directory missing"
	exit 4
fi

ram_capacity () {
	df -h $ramfs_directory | tail -1 |
			awk '{print $5}' |
			tr -d '%'
}

ffmpeg_running() {
	pgrep -F $FFMPEG_PID_FILE >/dev/null 2>/dev/null
}

movie_name=`echo "$video_path" | awk -v FS=/ '{print $(NF-1)}'`
FFMPEG_PID_FILE=$ramfs_directory/.pid
FRAME_PATH_FMT="$ramfs_directory/frame%09d.jpeg"

RAM_STOP_PCT=40	# small enough that we stop ffmpeg before the ramdisk fills up
RAM_CONT_PCT=8	# large enough to get more frames before the consumer runs out

# launch ffmpeg, and stop it immediately.

ffmpeg -hide_banner -v 8 -i "$video_path" -qscale:v 2 $FRAME_PATH_FMT 1>&2 &
log "$prog: ffmpeg started"

ffmpeg_pid=$!
echo $ffmpeg_pid >$FFMPEG_PID_FILE

kill -STOP $ffmpeg_pid

ram_capacity=`df -h $ramfs_directory | tail -1 |
	awk '{print $5}' | tr -d '%'`
log "$prog: ffmpeg stopped	$ram_capacity" 1>&2

ffmpeg_status=stopped

frame_number=1
while true
do
	framefn=`printf "$FRAME_PATH_FMT" $frame_number`
	if [ -s $framefn ]
	then
		# The next frame is not in the ram cache
		# If there is data in the cache, wait until
		# it gets below the CONT level and resume
		# ffmpeg to get more frames.

		rc=`ram_capacity`
		log "$prog:	Need more frames, at $rc%"

		while [ $rc -gt $RAM_CONT_PCT ]
		do
			if ! ffmpeg_running
			then
				log "$prog: ffmpeg finished, out of frames, done"
				exit 0
			fi
#			log "$prog:  waiting for frame consumption" 1>&2
			sleep 5
			rc=`ram_capacity`
		done

		log "$prog:	Restarting ffmpeg, at $rc%"

		if ! kill -CONT $ffmpeg_pid 2>/dev/null
		then
			log "$prog:	Cannot resume ffmpeg $ffmpeg_pid"
			exit 1
		fi

		while [ $rc -lt $RAM_STOP_PCT ]
		do
			sleep 0		# This is a race we never want to lose
			rc=`ram_capacity`
		done

		if ! kill -STOP $ffmpeg_pid 2>/dev/null
		then
			log "$prog:	Cannot resume ffmpeg $ffmpeg_pid"
		else
			log "$prog:	re-paused ffmpeg, at $rc%"
		fi

	fi

	if [ ! -s $framefn ]
	then
		if ! ffmpeg_running
		then
			exit 0
		else
			log "$prog:	stuck without further frames for some reason"
			exit 1
		fi
	fi
	echo "$framefn"
	frame_number=`expr $frame_number + 1`
done |
case "$first_frame" in		# skip leading frames, if desired
"")	cat -;;
0)	cat -;;
*)	sed "1,${first_frame}d"
	log "$prog: 	skipping ${first_frame} frames"
esac |
case $frame_count in		# limit frame count, if desired
"")	cat -;;
0)	cat -;;
all)	cat -;;
*)	sed -n "1,${frame_count}"p
	log "$prog: 	limited to ${frame_count} frames"
esac |
while read framefn
do
	echo "$framefn"
	sleep $spf
done

kill 0

log "$prog	exiting"
