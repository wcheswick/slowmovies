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

#include <opencv2/highgui/highgui_c.h>

CvCapture *input;
IplImage* image;

char *output_directory;

int
init_vid_io(char *src_file_name) {
	input = cvCaptureFromFile(src_file_name);
	if (input == NULL) {
		fprintf(stderr, "cv_split: error on open of %s\n", src_file_name);
		perror("perror");
		return 0;	// fail
	}
	output_directory = frames_directory;
	return 1;	// succeed
}

// In principle, there is a command
// for skipping leading frames::
//	cvSetCaptureProperty(input, CV_CAP_PROP_POS_FRAMES, 
//		(double)start_frame);
// but I understand it is broken in some versions of opencv

int
copy_frames(int spf) {
	frames_read = 0;
	while (frames_read < first_frame) {
		image = cvQueryFrame(input);
		if (image == NULL) {
			fprintf(stderr, "splitmovie hit EOF seeking frame %ld\n", 
			   first_frame);
				return 0;	//fail
		}
		frames_read++;
	}

	while ((frame_count && frames_written < frame_count) &&
	    (last_frame && frames_read < last_frame) &&
	    (image = cvQueryFrame(input)) != NULL) {
		char fn[MAXPATHLEN];

		if (image == 0) {
			fprintf(stderr, "cv+split cannot read file\n");
			return 0;
		}
		frames_read++;
		if (cvSaveImage(fn, image, 0) < 0) {
			perror("cvSaveImage");
			return 1;	// fail
		}

		sleep_until_spf_time(spf);
		printf("%s\n", fn);
		frames_written++;
	}
	cvReleaseCapture(&input);
	return 0;
}
