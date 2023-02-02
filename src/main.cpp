#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <list>
#include <mutex>
#include <thread>
#include <string>
#include <atomic>

#include "Client.h"
#include "Extern.h"
#include <k4a/k4a.hpp>
#include "utils/coreutil.h"

using namespace std;
//puts caputres inside the capture buffer
void cameraThread(list <shared_ptr<k4a::capture>>* capturebuffer, k4a::calibration* calibration, vector<uint8_t>* rawCalibration, atomic<bool>* stopRecording, atomic<bool>* cameraParameterSet)
{
	//Create and open device
	cout << "\nFound " << k4a_device_get_installed_count() << " connected Azure Kinect device(s)" << endl;
	k4a::device kinect = kinect.open(K4A_DEVICE_DEFAULT);
	cout << "Azure Kinect device opened!" << endl;
	cout << "Serial number: " << kinect.get_serialnum() << endl;

	//Configuration parameters
	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_1536P; //2048x1536
	config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED; //640x576
	config.camera_fps = K4A_FRAMES_PER_SECOND_30;
	config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
	config.synchronized_images_only = true;

	//Start Cameras
	kinect.start_cameras(&config);

	//Calibration object contains camera specific information and is used for all transformation functions
	*calibration = kinect.get_calibration(config.depth_mode, config.color_resolution);
	printCalibrationDepth(*calibration);

	//get raw calibration with intrinsics etc
	*rawCalibration = kinect.get_raw_calibration();

	////Capture contains multiple related images like color and depth
	k4a::capture capture;
	for (int i = 0; i < 30; i++) {
		kinect.get_capture(&capture); //dropping several frames for auto-exposure
		capture.reset();
	}
	*cameraParameterSet = true;

	//Wait until connection is established
	while (*stopRecording) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20ms));
	}

	//Camera loop
	while (!(*stopRecording)) {
		kinect.get_capture(&capture);
		capturelock.lock();
		capturebuffer->push_back(make_shared<k4a::capture>(capture));
		capturelock.unlock();
	}
	cout << "closing kinect" << endl;
	kinect.stop_cameras();
	kinect.close();
}


int main()
{
	list <shared_ptr<k4a::capture>> capturebuffer;
	k4a::calibration calibration;
	std::vector<uint8_t> rawCalibration; //calibration blob to be sent
	atomic<bool> stopRecording(true); // to avoid filling ram early, start pushing to caputurebuffer when connection is established and calibration was sent
	atomic<bool> stopConnection(false); // condition for send and compress loop. Also used as start variable once calibration is send, from startClient
	atomic<bool> cameraParameterSet(false); // only send camera related stuff when camera parameters are set 

	//camera stuff fills capture buffer
	std::thread cameraThread(cameraThread, &capturebuffer, &calibration, &rawCalibration, &stopRecording, &cameraParameterSet);
	//network stuff. Calls a send loop
	startClient(capturebuffer, calibration, rawCalibration, stopRecording, stopConnection, cameraParameterSet);

	stopRecording = true;
	cameraThread.join();
	
	return 0;
}