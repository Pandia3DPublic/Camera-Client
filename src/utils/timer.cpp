#include "timer.h"

Timer::Timer() {
	start = std::chrono::high_resolution_clock::now();
}

Timer::Timer(string a, TimeUnit unit) {
	content = a;
	unit_ = unit;
	start = std::chrono::high_resolution_clock::now();

}

Timer::Timer(string a) {
	content = a;
	start = std::chrono::high_resolution_clock::now();

}

Timer::~Timer() {
	end = std::chrono::high_resolution_clock::now();
	if (!done) {
		if (unit_ == millisecond) {
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			printf("Time elapsed in miliseconds in %s : %lli \n", content.c_str(), duration.count());
		}
		if (unit_ == microsecond) {
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			printf("Time elapsed in microseconds in %s : %lli \n", content.c_str(), duration.count());
		}
		if (unit_ == nanosecond) {
			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			printf("Time elapsed in nanoseconds in %s : %lli \n", content.c_str(), duration.count());
		}
	}
	done = true;
	//std::cout <<"Time elapsed in miliseconds in " << content << " : "<< duration.count() << std::endl;
}