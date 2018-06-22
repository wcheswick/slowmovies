/*
 * Read a video file and extract frames one-by-one.  They are written
 * to a file singly, that file name given by a template, and the 
 * path name is written to stdout.  Another routine can read the file name,
 * process the file, and remove it.
 *
 * We may pause between the frames to yield the desired frame rate, in seconds
 * per frame.
 *
 * Last updated by Bill Cheswick, June 2018.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <assert.h>

#include "vid_io.h"
#include "arg.h"

u_long frames_read = 0;
u_long frames_skipped = 0;
u_long frames_written = 0;

char *frames_directory;

u_long first_frame;
u_long frame_count;
u_long last_frame;

/*
 * real time, microseconds
 */
longtime_t
rtime_ms(void) {
        struct timeval tp;
        struct timezone tz;

        if (gettimeofday(&tp, &tz) < 0)
                perror("gettimeofday");
        return (tp.tv_sec*1000000 + tp.tv_usec)/1000;
}

longtime_t next_time = 0;

void
sleep_until_spf_time(int spf) {
	while (rtime_ms() < next_time)
		usleep(100*1000);
	next_time = rtime_ms() + spf*1000;	// When to send the next one
}

static int
usage(void) {
	fprintf(stderr, "usage: split [-c frame-count] [-f first-frame] [-s seconds-per-frame] [-S] [-n] video-file output_directory\n");
	return 1;
}

int 
main (int argc, char **argv) {
	char *src_file_name;
	u_long first_frame = 0;	// first frame
	u_long last_frame = 0;	// last frame (0 = all)
	u_long frame_count = 0;	// number of frames to show, 0 means all

	int nflag = 0;
	int Sflag = 0;
	int spf = 10;	// seconds per frame

	ARGBEGIN {
	case 'c':	
		frame_count = strtoul(ARGF(), (char **)NULL, 10); // default = 0 (all)
		break;
	case 'f':	
		first_frame = strtoul(ARGF(), (char **)NULL, 10); // default = 0
		break;
	case 'l':	
		last_frame = strtoul(ARGF(), (char **)NULL, 10); // default = 0 (all)
		break;
	case 'n':		// suppress non-stats output
		nflag++;	
		break;
	case 's':		// seconds per frame, default is 10
		spf = atoi(ARGF());	
		break;
	case 'S':		// output stats
		Sflag++;		
		break;
	default:
		return usage();
	} 
	ARGEND;

	if (argc != 2) {
		return usage();
	}
	src_file_name    = argv[0];
	frames_directory = argv[1];

	if (!init_vid_io(src_file_name)) {
		return 10;
	}

	setlinebuf(stdout);
	if (!copy_frames(spf)) {
		return 20;
	}

	if (Sflag) {	// show stats
		printf("Frames skipped: %lu, written: %lu\n",
			frames_skipped, frames_written);
	}

	return 0;
}
