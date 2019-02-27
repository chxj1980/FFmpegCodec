// FFmpegCodec.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/log.h"
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

/** 使用FFmpeg进行视频合并 */
void videoMerge() {

	int ret;

	av_log_set_level(AV_LOG_INFO);
	
	av_register_all();

	// int_1提供声音，int_2提供画面
	AVFormatContext *ifmt_1_ctx = NULL, *ifmt_2_ctx = NULL;

	AVFormatContext *ofmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;

	AVPacket pkt;

	int videoindex_v = -1, videoindex_out = -1;
	int audioindex_a = -1, audioindex_out = -1;

	int frame_index = 0;
	int64_t cur_pts_v = 0, cur_pts_a = 0;

	if ((ret = avformat_open_input(&ifmt_1_ctx, "int_1.mp4", 0, 0)) < 0) {
		printf("open input_1 file failed");
		return;
	}

	if ((ret = avformat_open_input(&ifmt_2_ctx, "int_2.mp4", 0, 0)) < 0) {
		printf("open input_2 file failed");
		return;
	}

	if ((ret = avformat_find_stream_info(ifmt_1_ctx, 0)) < 0) {
		printf("open input_1 file failed");
		return;
	}

	if ((ret = avformat_find_stream_info(ifmt_2_ctx, 0)) < 0) {
		printf("open input_2 file failed");
		return;
	}

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, "out.mp4");

	if (!ofmt_ctx) {
		fprintf(stderr, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		return;
	}

	ofmt = ofmt_ctx->oformat;
	
	for (int i = 0; i < ifmt_1_ctx->nb_streams; i++) {
		if (ifmt_1_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			AVStream *in_audio_stream = ifmt_1_ctx->streams[i];
			AVStream *out_audio_stream = avformat_new_stream(ofmt_ctx, in_audio_stream->codec->codec);
			audioindex_a = i;
			if (!out_audio_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				return;
			}
			audioindex_out = out_audio_stream->index;
			
			//Copy the settings of AVCodecContext
			if (avcodec_copy_context(out_audio_stream->codec, in_audio_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				return;
			}

			out_audio_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_audio_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			break;
		}
	}

	for (int i = 0; i < ifmt_2_ctx->nb_streams; i++) {
		if (ifmt_2_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			AVStream *in_video_stream = ifmt_2_ctx->streams[i];
			AVStream *out_video_stream = avformat_new_stream(ofmt_ctx, in_video_stream->codec->codec);
			videoindex_v = i;
			if (!out_video_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				return;
			}
			videoindex_out = out_video_stream->index;

			//Copy the settings of AVCodecContext
			if (avcodec_copy_context(out_video_stream->codec, in_video_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				return;
			}

			out_video_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_video_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			break;
		}
	}

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, "out.mp4", AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", "out.mp4");
			return;
		}
	}
	//Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
		return;
	}

	while (1) {
		AVFormatContext *ifmt_ctx;
		int stream_index = 0;
		AVStream *in_stream, *out_stream;

		//Get an AVPacket
		if (av_compare_ts(cur_pts_v, ifmt_2_ctx->streams[videoindex_v]->time_base, cur_pts_a, ifmt_1_ctx->streams[audioindex_a]->time_base) <= 0) {
			ifmt_ctx = ifmt_2_ctx;
			stream_index = videoindex_out;

			if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
				do {
					in_stream = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if (pkt.stream_index == videoindex_v) {
						//FIX：No PTS (Example: Raw H.264)
						//Simple Write PTS
						if (pkt.pts == AV_NOPTS_VALUE) {
							//Write PTS
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters
							pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts = pkt.pts;
							pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}

						cur_pts_v = pkt.pts;
						break;
					}
				} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
			}
			else {
				break;
			}
		}
		else {
			ifmt_ctx = ifmt_1_ctx;
			stream_index = audioindex_out;
			if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
				do {
					in_stream = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if (pkt.stream_index == audioindex_a) {

						//FIX：No PTS
						//Simple Write PTS
						if (pkt.pts == AV_NOPTS_VALUE) {
							//Write PTS
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters
							pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts = pkt.pts;
							pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}
						cur_pts_a = pkt.pts;

						break;
					}
				} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
			}
			else {
				break;
			}
		}

		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = stream_index;

		printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
		//Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);
	}

	av_write_trailer(ofmt_ctx);

	avformat_close_input(&ifmt_2_ctx);
	avformat_close_input(&ifmt_1_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return;
	}
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile) {
	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %d. \n", frame->pts);

	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		return;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			return;
		}

		printf("Write packet %d (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
}

void encodeH264() {
	const char *filename, *codec_name;
	const AVCodec *codec;
	AVCodecContext *c = NULL;
	int i, ret, x, y;
	FILE *f;
	AVFrame *frame;
	AVPacket *pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	filename = "111.h264";
	codec_name = "libx264";

	/* find the mpeg1video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		fprintf(stderr, "Codec '%s' not found\n", codec_name);
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = 352;
	c->height = 288;
	/* frames per second */
	AVRational framerate = { 25, 1 };
	AVRational time_base = { 1, 25 };
	c->time_base = time_base;
	c->framerate = framerate;
	//c->time_base = (AVRational) { 1, 25 };
	//c->framerate = (AVRational) { 25, 1 };

	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	ret = av_frame_get_buffer(frame, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		return;
	}

	/* encode 1 second of video */
	for (i = 0; i < 25; i++) {
		fflush(stdout);

		/* make sure the frame data is writable */
		ret = av_frame_make_writable(frame);
		if (ret < 0)
			exit(1);

		/* prepare a dummy image */
		/* Y */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (y = 0; y < c->height / 2; y++) {
			for (x = 0; x < c->width / 2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		frame->pts = i;

		/* encode the image */
		encode(c, frame, pkt, f);
	}

	/* flush the encoder */
	encode(c, NULL, pkt, f);

	/* add sequence end code to have a real MPEG file */
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);

}


int main(int argc, char* argv[])
{
	/** 0.FFmpeg Hello World **/
	//av_register_all();
	//printf("%s\n", avcodec_configuration());

	/** 1. 使用FFmpeg进行视频合并 */
	//videoMerge();

	/** 2. 使用FFmpeg进行视频编码 */
	encodeH264();

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
