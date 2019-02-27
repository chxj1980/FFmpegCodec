# FFmpegCodec
FFmpeg中级开发总结（主要是使用FFmpeg进行音视频编解码）



## 1. H264 编解码


#### 1). 关键头文件：
libavcodec/avcodec.h --> 包含常见的编解码的API


#### 2). 关键结构体：

AVCodec --> 编码器结构体

AVCodecContext -->  编码器上下文（串联整个流程）

AVFrame --> 解码后的帧


#### 3). 结构体内存的分配与释放的关键API：
av_frame_alloc() / av_frame_free()

avcodec_alloc_context3()

avcodec_free_context()	

#### 4). 解码步骤

a. 查找解码器（avcodec_find_decoder）

b. 打开解码器（avcodec_open2）

c. 解码（avcodec_decode_video2）


## 2. H264编码流程


a. 查找编码器（avcodec_find_encoder_by_name）

b. 设置编码参数，并打开编码器（avcodec_open2）

c. 编码（avcodec_encode_video2）


