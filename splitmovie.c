/*
 * splitmovie: starting at a given frame number in a movie, extract each frame
 * to a temporary file, and write the file name and frame number to stdout.
 * Write this information at the given frame rate, which should be pretty slow,
 * though a frame rate of zero will skip the pause.
 *
 * Bill Cheswick, Nantucket, August 2011.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <opencv2/highgui/highgui_c.h>
#include "arg.h"

typedef unsigned long long longtime_t;

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

int
usage(void) {
	fprintf(stderr, "usage: splitmovie [-n number-of-frames] [-s start_frame] [-r seconds_per_frame] moviefile tmpdir\n");
	return 1;
}

int
main(int argc, char *argv[]) {
	u_long start_frame = 0;
	int spf = 5;
	long number_of_frames = -1;	// -1 for all frames
	u_long current_frame;
	u_long n = 0;
	char *mfn;
	char *tmpdir;
	CvCapture *input;
	IplImage* image;
	longtime_t next;;

	ARGBEGIN {
	case 'n':	number_of_frames = atol(ARGF());	break;
	case 's':	start_frame = atol(ARGF());	break;
	case 'r':	spf = atoi(ARGF());		break;
	default:
		return usage();
	} ARGEND;

	if (argc != 2)
		return usage();

	mfn = *argv++;
	tmpdir = *argv++;

	input = cvCaptureFromFile(mfn);
	if (input == NULL) {
		perror("Opening movie");
		return 2;
	}

	if (start_frame != 0) {
#ifdef notdef	// This command is broken with current ffmpeg lib, it is said
	    cvSetCaptureProperty(input, CV_CAP_PROP_POS_FRAMES, (double)start_frame);
#else
		u_long i;
		fprintf(stderr, "splitmovie skipping to frame %ld....", start_frame);
		for (i=0; i<start_frame-1; i++) {
			image = cvQueryFrame(input);
			if (image == NULL) {
				fprintf(stderr, "splitmovie hit EOF seeking frame %ld\n", start_frame);
				return 0;
			}
		}
		fprintf(stderr, "done\n");
#endif
	}

	setbuf(stdout, 0);

	next = rtime_ms();
	current_frame = start_frame;

	while ((image = cvQueryFrame(input)) != NULL) {
		char fn[PATH_MAX];

		n++;
		current_frame++;
		if (number_of_frames >= 0 && n > number_of_frames)
			break;

		snprintf(fn, sizeof(fn), "%s/%06lu.jpeg", tmpdir, current_frame);
		printf("%s	%ld\n", fn, current_frame);

		if (cvSaveImage(fn, image, 0) < 0) {
			perror("cvSaveImage");
			return 10;
		}

		if (spf > 0) {
			while (rtime_ms() < next)
				usleep(100*1000);
			next = rtime_ms() + spf*1000;	// When to se d the next one
		}
	}

	cvReleaseCapture(&input);
	fprintf(stderr, "Frames read: %lu\n", n);
	return 0;
}
