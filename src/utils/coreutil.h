#pragma once
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <k4a/k4a.hpp>
#include "../Extern.h"

using namespace std;


std::shared_ptr<k4a::capture> getSingleCap(std::list <std::shared_ptr<k4a::capture>>& capturebuffer);

void prepareDatapath(std::string& s);
string getPicNumberString(int a);
bool inRange(int& value, int min, int max, int default);
int getMatBytes(cv::Mat & mat);
void printMatInfo(cv::Mat * mat);

void printCalibrationDepth(k4a::calibration & calib);
void printCalibrationColor(k4a::calibration & calib);