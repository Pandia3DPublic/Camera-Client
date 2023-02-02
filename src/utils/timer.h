#pragma once
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;

enum TimeUnit { millisecond, microsecond, nanosecond };

struct Timer {
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	string content = "scope";
	bool done = false;
	TimeUnit unit_ = microsecond;
	Timer();
	Timer(std::string a, TimeUnit unit);
	Timer(std::string a);
	~Timer();
};