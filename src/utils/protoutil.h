#pragma once
#include "../Extern.h"
#include <k4a/k4a.hpp>
#include <opencv2/opencv.hpp>
#include <atomic>
#include "../protos/framedata.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/delimited_message_util.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;

// raw calibration with intrinsics
std::shared_ptr<proto::Calibration> generateProtoCalibration(std::vector<uint8_t>& rawCalibration);

// compressed ffmpeg
std::shared_ptr<proto::FrameData> generateProtoFFmpeg(std::list <shared_ptr<AVPacket>>& colorbuffer, std::list <shared_ptr<AVPacket>>& depthbuffer);
std::shared_ptr<proto::FrameData> generateProtoFFmpegRVL(std::list <shared_ptr<AVPacket>>& colorbuffer, list <shared_ptr<vector<char>>>& depthbuffer);

// dataset
std::shared_ptr<proto::FrameData> generateProtoDataset(int nstart);