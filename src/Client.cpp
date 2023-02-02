#include "Client.h"

#include "utils/protoutil.h"
#include "utils/timer.h"
#include "utils/compressutil.h"

using namespace std;


template <typename Proto>
int sendProto(std::shared_ptr<Proto> proto, SOCKET& ConnectSocket) //Serialize and send a proto file
{
	// Serialize and write to buffer
	int size = proto->ByteSize() + sizeof(google::protobuf::uint32); //4 extra bytes for length of message
	char* buffer = new char[size];
	google::protobuf::io::ArrayOutputStream aos(buffer, size);
	google::protobuf::io::CodedOutputStream* output = new google::protobuf::io::CodedOutputStream(&aos);
	if (!google::protobuf::util::SerializeDelimitedToCodedStream(*proto, output)) { //Serialize including 4 extra bytes header containing message size
		cout << "serialize delimited to coded stream failed" << endl;
		return 1;
	}
	delete output;

	// Send the buffer
	int iResult = send(ConnectSocket, buffer, size, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		/*closesocket(ConnectSocket);
		WSACleanup();*/
		return 1;
	}
	printf("Bytes Sent: %ld\n", iResult);

	delete[] buffer;
	return 0;
}

int startClient(std::list <std::shared_ptr<k4a::capture>>& capturebuffer, k4a::calibration& calibration, vector<uint8_t>& rawCalibration, 
	std::atomic<bool>& stopRecording, std::atomic<bool>& stopConnection, std::atomic<bool>& cameraParameterSet)
{
	//########################################## Initialize sockets ###################################################
	WSADATA wsaData;
	int iResult;

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL, * ptr = NULL,hints;
	char ipserver[INET6_ADDRSTRLEN]; //for printing server's ip addr

	// Initialize Winsock, makeword defines version
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	//options
	hints.ai_family = AF_INET; //ipv4
	hints.ai_socktype = SOCK_STREAM; //socket type for streams
	hints.ai_protocol = IPPROTO_TCP; //tcp not udp

	// Resolve the server address and port. getaddrinfo fills the rest of  result
	iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result); //IP Address or host name of server (e.g. localhost or 10.1.5.10)
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//########################################## Connect to server ###################################################
	// Attempt to connect to an address until one succeeds
	//for loop prob not necessary
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		cout << "trying to connect to server" << endl;

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen); //on server side listen and put in queue and accept
		if (iResult == SOCKET_ERROR) {
			printf("connect failed with error: %ld\n", WSAGetLastError());
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	/*inet_ntop(ptr->ai_family, ptr->ai_addr, ipserver, sizeof(ipserver));
	std::cout << "Client: connected to server " << ipserver << std::endl;*/
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//disable Nagles algorithm as it causes 500ms delay (tested on localhost connection)
	BOOL opt = FALSE;
	int optlen = sizeof(BOOL);
	iResult = setsockopt(ConnectSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, optlen);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"setsockopt for TCP_NODELAY failed with error: %u\n", WSAGetLastError());
	}

	//#################################### camera stuff and send intrinsics ###########################################
	while (!cameraParameterSet) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// send calibration
	cout << "Sending raw calibration..." << endl;
	auto calib = generateProtoCalibration(rawCalibration);
	if (sendProto<proto::Calibration>(calib, ConnectSocket) == 1) {
		return 1;
	}

	// Start pushing to capturebuffer
	stopRecording = false;

	//############################### compress captures, push to packetbuffer #######################################
	std::list <shared_ptr<AVPacket>> colorbuffer; // packetbuffer gets filled by compress thread. H264 lossfull
	//std::list <shared_ptr<AVPacket>> depthbuffer;
	list <shared_ptr<vector<char>>> depthbuffer; //compressionless with rvl algorithm
	//this writes compressed packetes and images in depth and color buffer.
	std::thread compressThread(compressFFmpegRVL, &capturebuffer, &colorbuffer, &depthbuffer, &stopConnection);

	//########################################## Send image data #####################################################
	// Sending loop
	cout << "Sending image data..." << endl;
	while (!stopConnection)
	{
		Timer t("complete send loop");

		std::shared_ptr<proto::FrameData> framedata = generateProtoFFmpegRVL(colorbuffer, depthbuffer); //this locks if packetbuffer is empty
		cout << "\nsending... " << endl;
		if (sendProto<proto::FrameData>(framedata, ConnectSocket) == 1) {
			stopConnection = true; //todo test if this is okay with bad connection. Add stop button
		}
	}
	compressThread.join();
	stopRecording = true;

	//############################################## Shutdown ########################################################
	// shutdown the connection since no more data will be sent
	cout << "Shutting down connection" << endl;
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	cout << "Performing cleanup" << endl;
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}