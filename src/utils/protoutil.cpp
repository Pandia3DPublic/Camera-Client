#include "protoutil.h"

#include "timer.h"
#include "compressutil.h"
#include "coreutil.h"
#include "../cmakedefines.h" //dataset stuff
#include <mutex>
#include <string>

using namespace cv;


std::shared_ptr<proto::Calibration> generateProtoCalibration(std::vector<uint8_t>& rawCalibration)
{
	std::shared_ptr<proto::Calibration> calib = std::make_shared<proto::Calibration>();
	std::string data(rawCalibration.begin(), rawCalibration.end());
	calib->set_data(data);
	calib->set_length(rawCalibration.size());
	return calib;
}


std::shared_ptr<proto::FrameData> generateProtoFFmpeg(std::list <shared_ptr<AVPacket>>& colorbuffer, std::list <shared_ptr<AVPacket>>& depthbuffer)
{
	// get compressed packet
	while (colorbuffer.empty()) {
		this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	packetlock.lock();
	auto colorpkt = colorbuffer.front();
	colorbuffer.pop_front();
	packetlock.unlock();

	while (depthbuffer.empty()) {
		this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	packetlock.lock();
	auto depthpkt = depthbuffer.front();
	depthbuffer.pop_front();
	packetlock.unlock();

	// create and set proto
	std::shared_ptr<proto::FrameData> framedata = std::make_shared<proto::FrameData>();
	proto::ColorImage* color = new proto::ColorImage; //protobuf will take care of deleting as long as we do not call release_*
	proto::DepthImage* depth = new proto::DepthImage;

	color->set_data((void*)colorpkt->data, colorpkt->size);
	color->set_size(colorpkt->size);
	color->set_timestamp(colorpkt->pts);
	color->set_format("h264");

	depth->set_data((void*)depthpkt->data, depthpkt->size);
	depth->set_size(depthpkt->size);
	depth->set_timestamp(depthpkt->pts);
	depth->set_format("h264");

	framedata->set_allocated_colorimage(color);
	framedata->set_allocated_depthimage(depth);

	if (colorpkt->pts != depthpkt->pts) {
		cout << "Warning: color and depth have different timestamps!" << endl;
		cout << "color: " << colorpkt->pts << " depth: " << depthpkt->pts << endl;
	}

	return framedata;
}

std::shared_ptr<proto::FrameData> generateProtoFFmpegRVL(std::list <shared_ptr<AVPacket>>& colorbuffer, list <shared_ptr<vector<char>>>& depthbuffer)
{
	// get compressed packet
	while (colorbuffer.empty()) {
		this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	packetlock.lock();
	auto colorpkt = colorbuffer.front();
	colorbuffer.pop_front();
	packetlock.unlock();

	while (depthbuffer.empty()) {
		this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	packetlock.lock();
	auto vdepth = depthbuffer.front();
	depthbuffer.pop_front();
	packetlock.unlock();

	// create and set proto
	std::shared_ptr<proto::FrameData> framedata = std::make_shared<proto::FrameData>();
	proto::ColorImage* color = new proto::ColorImage; //protobuf will take care of deleting as long as we do not call release_*
	proto::DepthImage* depth = new proto::DepthImage;

	color->set_data((void*)colorpkt->data, colorpkt->size);
	color->set_size(colorpkt->size);
	color->set_timestamp(colorpkt->pts);
	color->set_format("h264");

	depth->set_data((void*)vdepth->data(), vdepth->size());
	depth->set_size(vdepth->size());
	depth->set_width(640); //todo not hardcode values
	depth->set_height(576);
	depth->set_timestamp(colorpkt->pts);
	depth->set_format("RVL");

	framedata->set_allocated_colorimage(color);
	framedata->set_allocated_depthimage(depth);

	return framedata;
}


std::shared_ptr<proto::FrameData> generateProtoDataset(int nstart)
{
	int width = 640;
	int height = 480;

	// Get dataset data
	string path;
	path = TOSTRING(datapath);
	prepareDatapath(path);

	std::string filepath_rgb(path + "\\color\\");
	std::string filepath_depth(path + "\\depth\\");

	// read images
	std::string filename_rgb_c = filepath_rgb + getPicNumberString(nstart) + ".jpg";
	Mat color = Mat(Size(width, height), CV_8UC3);
	color = imread(filename_rgb_c, IMREAD_ANYCOLOR);
	if (!color.data) {
		cout << "Could not open or find the color image" << std::endl;
	}

	std::string filename_depth_c = filepath_depth + getPicNumberString(nstart) + ".png";
	Mat depth = Mat(Size(width, height), CV_16U);
	depth = imread(filename_depth_c, IMREAD_ANYDEPTH);
	if (!depth.data) {
		cout << "Could not open or find the depth image" << std::endl;
	}

	size_t colorSize = color.total() * color.elemSize(); //Mat.total() returns rows x cols and Mat.elemSize() returns channels x sizeof(type)
	size_t depthSize = depth.total() * depth.elemSize();

	//visualization
	//cout << nstart << endl;
	//namedWindow("Display window", WINDOW_AUTOSIZE);// Create a window for display.
	//imshow("Display window", color);                   // Show our image inside it.
	//waitKey(1);

	// protobuf message init
	std::shared_ptr<proto::FrameData> framedata = std::make_shared<proto::FrameData>();
	proto::ColorImage* colorImg = new proto::ColorImage; //protobuf will take care of deleting as long as we do not call release_*
	proto::DepthImage* depthImg = new proto::DepthImage;

	colorImg->set_data((void*)color.data, colorSize);
	colorImg->set_width(width);
	colorImg->set_height(height);
	colorImg->set_size(colorSize);
	colorImg->set_timestamp(0);
	colorImg->set_stridebytes(0);
	colorImg->set_format("JPG");

	depthImg->set_data((void*)depth.data, depthSize);
	depthImg->set_width(width);
	depthImg->set_height(height);
	depthImg->set_size(depthSize);
	depthImg->set_timestamp(0);
	depthImg->set_stridebytes(0);
	depthImg->set_format("PNG");

	framedata->set_allocated_colorimage(colorImg);
	framedata->set_allocated_depthimage(depthImg);

	return framedata;
}