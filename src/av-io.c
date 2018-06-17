/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// http://dranger.com/ffmpeg/tutorial01.html

/**
 * @file
 * Demuxing and decoding example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c
 */

#include <unistd.h>
#include <assert.h>

#include "vidio.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

AVFormatContext *fmt_ctx = NULL;
AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
int width, height;
enum AVPixelFormat pix_fmt;
AVStream *video_stream = NULL, *audio_stream = NULL;

uint8_t *video_dst_data[4] = {
	NULL};
int      video_dst_linesize[4];
int video_dst_bufsize;

int video_stream_idx = -1, audio_stream_idx = -1;
AVFrame *frame = NULL;
AVPacket pkt;

struct SwsContext *swsContext;
AVFrame *pFrameRGB;
uint8_t *RGBbuffer;

/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
static int refcount = 0;

void
save_frame_as_pnm(AVCodecContext *pCodecCtx, AVFrame *frame, char *pathname) {
	FILE *f;
	int y;

	sws_scale(swsContext, 
		(uint8_t const * const *)frame->data,
		frame->linesize, 0, pCodecCtx->height,
		pFrameRGB->data, pFrameRGB->linesize);
	
        // Save the frame to disk
 	if ((f = fopen(pathname,"w")) == NULL) {
		perror(pathname);
		return;
	}
	fprintf(f, "P6\n%d %d\n%d\n", frame->width, frame->height, 255);
	for(y=0; y<frame->height; y++) {
		int n = fwrite(pFrameRGB->data[0] + y*pFrameRGB->linesize[0], 1,
			pFrameRGB->width*3, f);
		if (n < 0)
			perror("writing image");
	}
	fclose(f);
}


static int
decode_packet(int *got_frame, int cached) {
	int ret = 0;
	int decoded = pkt.size;

	*got_frame = 0;

	if (pkt.stream_index == video_stream_idx) {
		ret = avcodec_send_packet(video_dec_ctx, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error sending a packet for decoding\n");
			exit(1);
		}
		while (ret >= 0) {
			char pathname[1024];

			ret = avcodec_receive_frame(video_dec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				return -1;
			else if (ret < 0) {
				fprintf(stderr, "Error during decoding\n");
				exit(1);
			}
			frame_count++;

			if (nflag)
				continue;
			if (count && frames_written >= count)
				return -2;	// finished
			if (first && frame_count < first)
				continue;

			fflush(stdout);
			snprintf(pathname, sizeof(pathname),
				output_file_template, frame_count);
			save_frame_as_pnm(video_dec_ctx, frame, pathname);

			printf("%s\n", pathname);
			frames_written++;
			if (spf > 0)
				sleep(spf);
		}
	}

	/* If we use frame reference counting, we own the data and need
	     * to de-reference it when we don't use it anymore */
	if (*got_frame && refcount)
	av_frame_unref(frame);

	return decoded;
}

static int open_codec_context(int *stream_idx,
AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream *st;
	AVCodec *dec = NULL;
	AVDictionary *opts = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
		    av_get_media_type_string(type), src_filename);
		return ret;
	} else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			fprintf(stderr, "Failed to find %s codec\n",
			    av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			fprintf(stderr, "Failed to allocate the %s codec context\n",
			    av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
			    av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
		if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
			    av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

static int get_format_from_sample_fmt(const char **fmt,
enum AVSampleFormat sample_fmt)
{
	int i;
	struct sample_fmt_entry {
		enum AVSampleFormat sample_fmt; 
		const char *fmt_be, *fmt_le;
	} sample_fmt_entries[] = {
		{ AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
		{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
		{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
		{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
		{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
	};
	*fmt = NULL;

	for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
		struct sample_fmt_entry *entry = &sample_fmt_entries[i];
		if (sample_fmt == entry->sample_fmt) {
			*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
			return 0;
		}
	}

	fprintf(stderr,
	    "sample format %s is not supported as output format\n",
	    av_get_sample_fmt_name(sample_fmt));
	return -1;
}

void
initvidio(char *src_file_name) {
	av_register_all();

	/* open input file, and allocate format context */
	if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", src_filename);
		exit(1);
	}

	/* retrieve stream information */
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(2);
	}

	if (open_codec_context(&video_stream_idx, &video_dec_ctx, 
	    fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
		video_stream = fmt_ctx->streams[video_stream_idx];

		/* allocate image where the decoded image will be put */
		width = video_dec_ctx->width;
		height = video_dec_ctx->height;
		pix_fmt = video_dec_ctx->pix_fmt;
		ret = av_image_alloc(video_dst_data, video_dst_linesize,
		    width, height, pix_fmt, 1);
		if (ret < 0) {
			fprintf(stderr, "Could not allocate raw video buffer\n");
			exit(3);
		}
		video_dst_bufsize = ret;
	}

	if (!audio_stream && !video_stream) {
		fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
		exit(10);
	}

	swsContext = sws_getContext(video_dec_ctx->width, video_dec_ctx->height, 
		video_dec_ctx->pix_fmt,
		video_dec_ctx->width, video_dec_ctx->height,
		AV_PIX_FMT_RGB24, SWS_BILINEAR, 
		NULL, NULL, NULL);

	pFrameRGB = av_frame_alloc();
	if (!pFrameRGB) {
		fprintf(stderr, "Could not allocate pFrameRGB\n");
		ret = AVERROR(ENOMEM);
		exit(6);
	}

	size_t bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, 
		video_dec_ctx->width, video_dec_ctx->height, 32);
	RGBbuffer = (uint8_t *)av_malloc(sizeof(uint8_t) *bufsize);
	if (!RGBbuffer) {
		fprintf(stderr, "Could not allocate RGBbuffer\n");
		ret = AVERROR(ENOMEM);
		exit(6);
	}

	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
		RGBbuffer, AV_PIX_FMT_RGB24,
		video_dec_ctx->width, video_dec_ctx->height, 1);
	pFrameRGB->width = video_dec_ctx->width;
	pFrameRGB->height = video_dec_ctx->height;
	pFrameRGB->format = AV_PIX_FMT_RGB24;

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate frame\n");
		ret = AVERROR(ENOMEM);
		exit(6);
	}

	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	/* read frames from the file */
	ret = 0;

}

	while (ret > -2 && av_read_frame(fmt_ctx, &pkt) >= 0) {
		AVPacket orig_pkt = pkt;
		do {
			ret = decode_packet(&got_frame, 0);
			if (ret < 0)
				break;
			pkt.data += ret;
			pkt.size -= ret;
		} while (pkt.size > 0);
		av_packet_unref(&orig_pkt);
	}

	/* flush cached frames */
	pkt.data = NULL;
	pkt.size = 0;
	do {
		decode_packet(&got_frame, 1);
	} while (got_frame);

	if (Sflag) {	// show stats
		printf("Frames: %d	w: %d	h: %d\n",
			frame_count, width, height);
	}

	return 0;
}

void
finish_vidio(void) {
	/* flush cached frames */
	pkt.data = NULL;
	pkt.size = 0;
	do {
		decode_packet(&got_frame, 1);
	} while (got_frame);
}
