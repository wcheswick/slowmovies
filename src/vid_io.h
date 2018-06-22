/* vidio.h - interface to the video/image routines */

#include <sys/types.h>
#include <sys/param.h>

typedef unsigned long long longtime_t;

// in split.c

extern	void sleep_until_spf_time(int spf);

extern	u_long frames_read;
extern	u_long frames_skipped;
extern	u_long frames_written;
extern	char *frames_directory;

extern	u_long first_frame;
extern	u_long frame_count;
extern	u_long last_frame;

// in {av|cv}_io.c
extern	int	init_vid_io(char *src_file_name);
extern	int	copy_frames(int spf);
extern	void	finish_vid_io(void);
