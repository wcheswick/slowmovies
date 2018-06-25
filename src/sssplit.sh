#!/bin/sh
#
prog=sssplit
usage="$prog [-c frame-count] [-f first-frame] [-s seconds-per-frame] [-S] movie ramfs_directory"
#
#	Split a video file into frames. We use ffmpeg to split the frames
#	off, but we want them slowly.  So we queue some in a ramdisk (to
#	save wear on SSDs, for example), and stifle ffmpeg with a STOP signal.
#	We send it a continue when we need more.  
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

movie=`echo "$video_path" | awk -v FS=/ '{print $(NF-1)}'`
FFMPEG_PID_FILE=$ramfs_directory/.pid


# start up ffmpeg.  We will start and stop it based on how full the 
# ramdisk is

FRAME_PATH_FMT="$ramfs_directory/frame%09d.jpeg"

ffmpeg -hide_banner -v 8 -i "$video_path" -qscale:v 2 $FRAME_PATH_FMT 1>&2 &
echo "$prog: ffmpeg started" 1>&2

ffmpeg_pid=$!
echo $ffmpeg_pid >$FFMPEG_PID_FILE

kill -STOP $ffmpeg_pid
ram_capacity=`df -h $ramfs_directory | tail -1 |
	awk '{print $5}' | tr -d '%'`
echo "$prog: ffmpeg stopped	$ram_capacity" 1>&2

RAM_STOP_PCT=40
RAM_CONT_PCT=8

ffmpeg_status=stopped

frame_number=1
while true
do
	framefn=`printf "$FRAME_PATH_FMT" $frame_number`
	while [ $ffmpeg_status != "finished" -a ! -s $framefn ]
	do	# frame is not there, what's up?
		ram_capacity=`df -h $ramfs_directory | tail -1 |
			awk '{print $5}' | tr -d '%'`
		case $ffmpeg_status in
		running)		# see if we have hit the high-water mark
			if ! pgrep -F $FFMPEG_PID_FILE >/dev/null 2>/dev/null
			then
				echo "$prog: ffmpeg went away	$ram_capacity" 1>&2
				exit 10
			fi
			if [ $ram_capacity -ge $RAM_STOP_PCT ]
			then
				if ! kill -STOP $ffmpeg_pid 2>/dev/null
				then
					log "$prog:	$ffmpeg_pid STOP error"
					ffmpeg_status=unknown
				else
#echo "$prog: stopped	$ram_capacity" 1>&2
					ffmpeg_status=stopped
				fi
			fi;;
		stopped)
			if [ $ram_capacity -lt $RAM_CONT_PCT ]
			then
				if ! kill -CONT $ffmpeg_pid 2>/dev/null
				then
					ffmpeg_status=unknown
					log "$prog:	$ffmpeg_pid CONT error"
				else
#echo "$prog: started	$ram_capacity" 1>&2
					ffmpeg_status=running
				fi
			else	# we are waiting for him to eat frames
#echo "$prog:  waiting for consumption" 1>&2
				sleep 5
			fi
		esac
	done
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
