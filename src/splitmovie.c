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

#include <opencv2/highgui/highgui_c.h>

#include "arg.h"

typedef unsigned long long longtime_t;

int nflag = 0;
int Sflag = 0;
int spf = 10;	// seconds per frame
int first = 0;	// first frame
int last = 0;	// last frame (0 = all)
int count = 0;	// number of frames to show, 0 means all

int frame_count = 0;
int frames_written = 0;

char *output_file_template = 0;

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
	fprintf(stderr, "usage: avsplit [-c frame-count] [-f first-frame] [-s seconds-per-frame] [-S] [-n] video-file filepattern\n");
	return 1;
}

int 
main (int argc, char **argv) {
	u_long frame_number = 0;
	u_long frame_count = 0;
	u_long first_frame = 0;
	CvCapture *input;
	IplImage* image;
	char *src_file_name;
	char *output_directory;
	longtime_t next = 0;

	ARGBEGIN {
	case 'c':	
		count = atoi(ARGF());	// number of frames, default = 0 (all)
		break;
	case 'f':	
		first_frame = atoi(ARGF());	// first frame, default=0
		break;
	case 'l':	
		last = atoi(ARGF());	// last frame, default=0 (all)
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
	output_directory = argv[1];

	input = cvCaptureFromFile(src_file_name);
	if (input == NULL) {
		perror("Opening movie");
		return 2;
	}

	// Skip leading frames, if desired.  In principle, there is a command
	// for that:
	//	cvSetCaptureProperty(input, CV_CAP_PROP_POS_FRAMES, 
	//		(double)start_frame);
	// but I understand it is broken in some versions of opencv

	while (frame_number < first_frame) {
		image = cvQueryFrame(input);
		if (image == NULL) {
			fprintf(stderr, "splitmovie hit EOF seeking frame %ld\n", 
			   first_frame);
				return 10;
		}
		frame_number++;
	}

	setlinebuf(stdout);

	while ((count && frame_count < count) &&
	    (last && frame_number < last) &&
	    (image = cvQueryFrame(input)) != NULL) {
		char fn[MAXPATHLEN];

		if (image == 0) {
			fprintf(stderr, "splitmovie cannot read file\n");
			return 1;
		}
		snprintf(fn, sizeof(fn), "%s/%06lu.jpeg", 
			output_directory, frame_number);
		if (cvSaveImage(fn, image, 0) < 0)
			perror("cvSaveImage");

		while (rtime_ms() < next)
			usleep(100*1000);
		printf("%s\n", fn);
		next = rtime_ms() +spf*1000;	// When to send the next one
		frame_number++;
		frame_count++;
	}

	cvReleaseCapture(&input);

#ifdef notdef
	if (Sflag) {	// show stats
		printf("Frames: %d	w: %d	h: %d\n",
			frame_count, width, height);
	}
#endif

	return 0;
}
