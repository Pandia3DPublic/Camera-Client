#pragma once
#include <opencv2/opencv.hpp>
#include <k4a/k4a.hpp>
#include <vector>
#include <atomic>
#include "coreutil.h"
#include "../protos/framedata.pb.h"
#include "../Extern.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;

//ffmpeg video compression
int compressFFmpeg(list <shared_ptr<k4a::capture>>* capturebuffer, list <shared_ptr<AVPacket>>* colorbuffer, list <shared_ptr<AVPacket>>* depthbuffer, atomic<bool>* stop); //color and depth with ffmpeg h264
int compressFFmpegRVL(list <shared_ptr<k4a::capture>>* capturebuffer, list <shared_ptr<AVPacket>>* colorbuffer, list <shared_ptr<vector<char>>>* depthbuffer, atomic<bool>* stop); //color with ffmpeg h264, depth rvl

//Fast Lossless Depth Image Compression, Wilson 2017
int ff_librvldepth_compress_rvl(const short* input, char* output, int numPixels);
void ff_librvldepth_decompress_rvl(const char* input, short* output, int numPixels);
