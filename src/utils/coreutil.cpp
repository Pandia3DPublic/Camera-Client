#include "coreutil.h"

std::shared_ptr<k4a::capture> getSingleCap(std::list <std::shared_ptr<k4a::capture>>& capturebuffer)
{
	while (capturebuffer.empty()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	capturelock.lock();
	auto tmp = capturebuffer.front();
	capturebuffer.pop_front();
	capturelock.unlock();

	return tmp;
}

void prepareDatapath(std::string& s) {
	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '/') {
			s.insert(i + 1, "/");
			i++;
		}
	}
}

std::string getPicNumberString(int a) {
	std::string out = std::to_string(a);
	while (out.length() < 6) {
		out = "0" + out;
	}
	return out;
}

bool inRange(int& value, int min, int max, int default)
{
	if (value > max || value < min) {
		value = default;
		return false;
	}
	else {
		return true;
	}
}

//k4a_calibration_intrinsic_parameters_t::_param getScaledIntr(k4a::calibration& calibration, bool isDepth = false) {
//	auto intr = calibration.color_camera_calibration.intrinsics.parameters.param;
//	double xfactor = (double)width / calibration.color_camera_calibration.resolution_width; //xres
//	double yfactor = (double)height / calibration.color_camera_calibration.resolution_height; //yres
//
//}

int getMatBytes(cv::Mat & mat)
{
	if (mat.isContinuous()) {
		return mat.total() * mat.elemSize();
	}
	else {
		return mat.step[0] * mat.rows;
	}
}

void printMatInfo(cv::Mat * mat)
{
	size_t size = mat->total() * mat->elemSize(); //Mat.total() returns rows x cols and Mat.elemSize() returns channels x sizeof(type)
	cout << "size in bytes: " << size << endl;
	cout << "number of elements: " << mat->total() << endl;
	cout << "cols: " << mat->cols << endl;
	cout << "rows: " << mat->rows << endl;
	cout << "elem size: " << mat->elemSize() << endl;
	cout << "channels: " << mat->channels() << endl;
	cout << "is continous: " << mat->isContinuous() << endl;
	cout << endl;
}

void printCalibrationDepth(k4a::calibration & calib)
{
	auto color = calib.color_camera_calibration;
	auto depth = calib.depth_camera_calibration;
	cout << "\n===== Device Calibration information (depth) =====\n";
	cout << "color resolution width: " << color.resolution_width << endl;
	cout << "color resolution height: " << color.resolution_height << endl;
	cout << "depth resolution width: " << depth.resolution_width << endl;
	cout << "depth resolution height: " << depth.resolution_height << endl;
	cout << "principal point x: " << depth.intrinsics.parameters.param.cx << endl;
	cout << "principal point y: " << depth.intrinsics.parameters.param.cy << endl;
	cout << "focal length x: " << depth.intrinsics.parameters.param.fx << endl;
	cout << "focal length y: " << depth.intrinsics.parameters.param.fy << endl;
	cout << "radial distortion coefficients:" << endl;
	cout << "k1: " << depth.intrinsics.parameters.param.k1 << endl;
	cout << "k2: " << depth.intrinsics.parameters.param.k2 << endl;
	cout << "k3: " << depth.intrinsics.parameters.param.k3 << endl;
	cout << "k4: " << depth.intrinsics.parameters.param.k4 << endl;
	cout << "k5: " << depth.intrinsics.parameters.param.k5 << endl;
	cout << "k6: " << depth.intrinsics.parameters.param.k6 << endl;
	cout << "center of distortion in Z=1 plane, x: " << depth.intrinsics.parameters.param.codx << endl;
	cout << "center of distortion in Z=1 plane, y: " << depth.intrinsics.parameters.param.cody << endl;
	cout << "tangential distortion coefficient x: " << depth.intrinsics.parameters.param.p1 << endl;
	cout << "tangential distortion coefficient y: " << depth.intrinsics.parameters.param.p2 << endl;
	cout << "metric radius: " << depth.intrinsics.parameters.param.metric_radius << endl;
	cout << endl;

}

void printCalibrationColor(k4a::calibration & calib)
{
	auto color = calib.color_camera_calibration;
	auto depth = calib.depth_camera_calibration;
	cout << "\n===== Device Calibration information (color) =====\n";
	cout << "color resolution width: " << color.resolution_width << endl;
	cout << "color resolution height: " << color.resolution_height << endl;
	cout << "depth resolution width: " << depth.resolution_width << endl;
	cout << "depth resolution height: " << depth.resolution_height << endl;
	cout << "principal point x: " << color.intrinsics.parameters.param.cx << endl;
	cout << "principal point y: " << color.intrinsics.parameters.param.cy << endl;
	cout << "focal length x: " << color.intrinsics.parameters.param.fx << endl;
	cout << "focal length y: " << color.intrinsics.parameters.param.fy << endl;
	cout << "radial distortion coefficients:" << endl;
	cout << "k1: " << color.intrinsics.parameters.param.k1 << endl;
	cout << "k2: " << color.intrinsics.parameters.param.k2 << endl;
	cout << "k3: " << color.intrinsics.parameters.param.k3 << endl;
	cout << "k4: " << color.intrinsics.parameters.param.k4 << endl;
	cout << "k5: " << color.intrinsics.parameters.param.k5 << endl;
	cout << "k6: " << color.intrinsics.parameters.param.k6 << endl;
	cout << "center of distortion in Z=1 plane, x: " << color.intrinsics.parameters.param.codx << endl;
	cout << "center of distortion in Z=1 plane, y: " << color.intrinsics.parameters.param.cody << endl;
	cout << "tangential distortion coefficient x: " << color.intrinsics.parameters.param.p1 << endl;
	cout << "tangential distortion coefficient y: " << color.intrinsics.parameters.param.p2 << endl;
	cout << "metric radius: " << color.intrinsics.parameters.param.metric_radius << endl;
	cout << endl;

}