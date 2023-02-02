#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include <list>
#include <iostream>
#include <atomic>
#include "k4a/k4a.hpp"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "31415"
#define SERVER_IP "localhost" //IP Address or host name of server (e.g. localhost, 10.1.5.10 or 192.168.42.140). Warning port must be freed in firewall. Remote in client, local in server

int startClient(std::list <std::shared_ptr<k4a::capture>>& capturebuffer, k4a::calibration& calibration, std::vector<uint8_t>& rawCalibration, 
	std::atomic<bool>& stopRecording, std::atomic<bool>& stopConnection, std::atomic<bool>& cameraParameterSet);