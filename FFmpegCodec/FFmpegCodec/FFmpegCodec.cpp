// FFmpegCodec.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdio.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/log.h"
}

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

// 使用FFmpeg打印多媒体文件的Meta信息
void ffmpegVideoMeta() {

	av_log_set_level(AV_LOG_INFO);

	AVFormatContext *fmt_ctx = NULL;

	av_register_all();

	int ret;
	// 参数为 AVFormatContext上下文、文件名、指定的输入格式（一般为NULL，由ffmpeg自行解析）、附加参数（一般为NULL）
	ret = avformat_open_input(&fmt_ctx, "111.mp4", NULL, NULL);

	if (ret < 0) {
		printf("Cant open File: %s\n", av_err2str(ret));
	}

	// 参数为AVFormatContext上下文、流索引值（一般不用关心，直接写0）、文件名、是否是输入出文件（1：是  0：不是）	
	av_dump_format(fmt_ctx, 0, "111.mp4", 0);

	// 关闭打开的多媒体文件
	avformat_close_input(&fmt_ctx);
}


int main(int argc, char* argv[])
{
	/** 0.FFmpeg Hello World **/
	//av_register_all();
	//printf("%s\n", avcodec_configuration());

	ffmpegVideoMeta();

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
