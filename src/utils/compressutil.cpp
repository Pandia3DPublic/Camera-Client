#include "compressutil.h"

using namespace cv;

//############################################################################################################
//######################################## ffmpeg video compression ##########################################
//############################################################################################################

// unencoded frame gets send to encoder, receive encoded packet, push to own buffer
static void encode(AVCodecContext* codecx, AVFrame* frame, AVPacket* packet, std::list <shared_ptr<AVPacket>>& packetbuffer)
{
	int ret;

	if (frame)
		printf("Send frame %3" PRId64"\n", frame->pts);

	ret = avcodec_send_frame(codecx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(codecx, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}

		// now encoded!!
		printf("Write packet %3" PRId64" (size=%5d)\n", packet->pts, packet->size);

		// push compressed packet to packetbuffer
		AVPacket *pkt = av_packet_clone(packet);
		packetlock.lock();
		packetbuffer.push_back(make_shared<AVPacket>(*pkt));
		cout << "packetbuffer size " << packetbuffer.size() << endl;
		packetlock.unlock();

		av_packet_unref(packet);
	}
}

int compressFFmpeg(list <shared_ptr<k4a::capture>>* capturebuffer, list <shared_ptr<AVPacket>>* colorbuffer, list <shared_ptr<AVPacket>>* depthbuffer, std::atomic<bool>* stop)
{
	av_log_set_level(AV_LOG_VERBOSE); // for debug
	avcodec_register_all();
	int ret;
	
	//#################################################### init #########################################################

	// Color
	AVPacket* pktColor; // gets filled by encoder
	AVFrame* avColor; // raw color image frame
	AVCodec* codec;
	AVCodecContext* codecx = NULL;
	int source_width_color = 2048, source_height_color = 1536;
	int target_width_color = 640, target_height_color = 480;

	// depth
	AVPacket* pktDepth; // gets filled by encoder
	AVFrame* avDepth; // raw color image frame
	AVCodec* codec2;
	AVCodecContext* codecx2 = NULL;
	int source_width_depth = 640, source_height_depth = 576;
	int target_width_depth = 640, target_height_depth = 576;

	// alloc packet
	pktColor = av_packet_alloc(); 
	if (!pktColor)
		cout << "couldn't allocate pktColor" << endl;

	pktDepth = av_packet_alloc();
	if (!pktDepth)
		cout << "couldn't allocate pktDepth" << endl;

	// alloc frame
	avColor = av_frame_alloc();
	if (!avColor)
		cout << "couldn't allocate avColor" << endl;

	avDepth = av_frame_alloc();
	if (!avDepth)
		cout << "couldn't allocate avDepth" << endl;

	// set frame parameters
	avColor->format = AV_PIX_FMT_BGR24;
	avColor->width = target_width_color;
	avColor->height = target_height_color;

	avDepth->format = AV_PIX_FMT_BGR24;
	avDepth->width = target_width_depth;
	avDepth->height = target_height_depth;

	// alloc image
	ret = av_image_alloc(avColor->data, avColor->linesize, avColor->width, avColor->height, (AVPixelFormat)avColor->format, 16);
	if (ret < 0)
		cout << "av_image_alloc error: " << ret << endl;

	ret = av_image_alloc(avDepth->data, avDepth->linesize, avDepth->width, avDepth->height, (AVPixelFormat)avDepth->format, 16);
	if (ret < 0)
		cout << "av_image_alloc error: " << ret << endl;


	//#################################################### Codec #########################################################
		
	//########## set codec parameters color ##########
	codec = avcodec_find_encoder_by_name("libx264rgb"); // input AV_CODEC_ID_H264: yuv, libx264rgb: bgr24 or rgb24
	if (!codec)
		cout << "could not find codec" << endl;

	codecx = avcodec_alloc_context3(codec); //codec context to set codec options
	if (!codecx)
		cout << "could not allocate video codec context" << endl;

	codecx->width = target_width_color;
	codecx->height = target_height_color;
	codecx->time_base = AVRational{ 1, 30 }; //fps
	codecx->gop_size = 30;
	codecx->max_b_frames = 0;
	codecx->pix_fmt = AV_PIX_FMT_BGR24;
	av_opt_set(codecx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codecx->priv_data, "tune", "zerolatency", 0);
	av_opt_set(codecx->priv_data, "crf", "18", 0);

	//open codec
	ret = avcodec_open2(codecx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//########### set codec parameters depth ############
	codec2 = avcodec_find_encoder_by_name("libx264rgb"); // input AV_CODEC_ID_H264: yuv, libx264rgb: bgr24 or rgb24
	if (!codec2)
		cout << "could not find codec" << endl;

	codecx2 = avcodec_alloc_context3(codec2); //codec context to set codec options
	if (!codecx2)
		cout << "could not allocate video codec context" << endl;

	codecx2->width = target_width_depth;
	codecx2->height = target_height_depth;
	codecx2->time_base = AVRational{ 1, 30 }; //fps
	codecx2->gop_size = 30;
	codecx2->max_b_frames = 0;
	codecx2->pix_fmt = AV_PIX_FMT_BGR24;
	av_opt_set(codecx2->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codecx2->priv_data, "tune", "zerolatency", 0);
	av_opt_set(codecx2->priv_data, "crf", "0", 0);

	//open codec
	ret = avcodec_open2(codecx2, codec2, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//#################################################### compress loop #########################################################

	// for scale color image and remove alpha channel
	struct SwsContext* scalerColor = sws_getContext(source_width_color, source_height_color, AV_PIX_FMT_BGRA, target_width_color, target_height_color, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	// encode loop
	uint64_t timestamp = 0;
	while (!(*stop)) 
	{
		// get capture
		auto capture = getSingleCap(*capturebuffer);
		auto color = capture->get_color_image();
		auto depth = capture->get_depth_image();

		// color: pointer arithmetic for sws_scale function input parameters
		int stridetmp = color.get_stride_bytes(); // num channels * width num pixels
		const int* strideCol = &stridetmp;
		uint8_t* datatmp = color.get_buffer(); // bgra color img dataCol
		const uint8_t* const* dataCol = &datatmp;

		// color: scale and convert format, fills avColor for us
		sws_scale(scalerColor, dataCol, strideCol, 0, source_height_color, avColor->data, avColor->linesize);

		// depth: split 1 16-bit channel to 3 8-bit channels: use first two 8-bit channels, set third to 0
		uint8_t* dataDepth = depth.get_buffer();
		for (int y = 0; y < target_height_depth; y++) {
			for (int x = 0; x < target_width_depth; x++) {
				avDepth->data[0][y * avDepth->linesize[0] + 3 * x] = *dataDepth++; //b
				avDepth->data[0][y * avDepth->linesize[0] + 3 * x + 1] = *dataDepth++; //g
				avDepth->data[0][y * avDepth->linesize[0] + 3 * x + 2] = 0; //r
			}
		}

		// set timestamp
		avColor->pts = timestamp;
		avDepth->pts = timestamp;

		/* encode the images */
		encode(codecx, avColor, pktColor, *colorbuffer);
		encode(codecx2, avDepth, pktDepth, *depthbuffer);

		timestamp++;
	}

	/* flush the encoders */
	encode(codecx, NULL, pktColor, *colorbuffer);
	encode(codecx2, NULL, pktDepth, *depthbuffer);

	avcodec_free_context(&codecx);
	av_frame_free(&avColor);
	av_packet_free(&pktColor);

	avcodec_free_context(&codecx2);
	av_frame_free(&avDepth);
	av_packet_free(&pktDepth);

	return 0;
}

//compresses color with H264 and depth with rvl. takes from capturebuffer, fills color and depth buffer
int compressFFmpegRVL(list <shared_ptr<k4a::capture>>* capturebuffer, list <shared_ptr<AVPacket>>* colorbuffer, list <shared_ptr<vector<char>>>* depthbuffer, std::atomic<bool>* stop)
{
	av_log_set_level(AV_LOG_VERBOSE); // for debug
	avcodec_register_all();
	int ret;

	//#################################################### init #########################################################

	// Color
	AVPacket* pktColor; // gets filled by encoder
	AVFrame* avColor; // raw color image frame
	AVCodec* codec;
	AVCodecContext* codecx = NULL; //options
	int source_width_color = 2048, source_height_color = 1536; 
	int target_width_color = 640, target_height_color = 480;

	// alloc packet
	pktColor = av_packet_alloc();
	if (!pktColor)
		cout << "couldn't allocate pktColor" << endl;

	// alloc frame
	avColor = av_frame_alloc();
	if (!avColor)
		cout << "couldn't allocate avColor" << endl;

	// set frame parameters
	avColor->format = AV_PIX_FMT_BGR24;
	avColor->width = target_width_color;
	avColor->height = target_height_color;

	// alloc image
	ret = av_image_alloc(avColor->data, avColor->linesize, avColor->width, avColor->height, (AVPixelFormat)avColor->format, 16);
	if (ret < 0)
		cout << "av_image_alloc error: " << ret << endl;

	//#################################################### Codec #########################################################

	//########## set codec parameters color ##########
	codec = avcodec_find_encoder_by_name("libx264rgb"); // input AV_CODEC_ID_H264: yuv, libx264rgb: bgr24 or rgb24
	if (!codec)
		cout << "could not find codec" << endl;

	codecx = avcodec_alloc_context3(codec); //codec context to set codec options
	if (!codecx)
		cout << "could not allocate video codec context" << endl;

	codecx->width = target_width_color;
	codecx->height = target_height_color;
	codecx->time_base = AVRational{ 1, 30 }; //fps
	codecx->gop_size = 30; //keyframe interval maybe
	codecx->max_b_frames = 0; //bidirectional frames
	codecx->pix_fmt = AV_PIX_FMT_BGR24;
	av_opt_set(codecx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codecx->priv_data, "tune", "zerolatency", 0); //this makes everything much faster
	av_opt_set(codecx->priv_data, "crf", "30", 0); //compression level here. 18 is undistinguishable by eye. Around 40 is max. 0 is losless

	//open codec
	ret = avcodec_open2(codecx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//#################################################### compress loop #########################################################

	// for scale color image and remove alpha channel
	struct SwsContext* scalerColor = sws_getContext(source_width_color, source_height_color, AV_PIX_FMT_BGRA, target_width_color, target_height_color, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	// encode loop
	uint64_t timestamp = 0;
	while (!(*stop))
	{
		// get capture
		auto capture = getSingleCap(*capturebuffer);
		auto color = capture->get_color_image();
		auto depth = capture->get_depth_image();

		// color: pointer arithmetic for sws_scale function input parameters
		int stridetmp = color.get_stride_bytes(); // num channels * width num pixels
		const int* strideCol = &stridetmp;
		uint8_t* datatmp = color.get_buffer(); // bgra color img dataCol
		const uint8_t* const* dataCol = &datatmp;

		// color: scale and convert format, fills avColor for us
		sws_scale(scalerColor, dataCol, strideCol, 0, source_height_color, avColor->data, avColor->linesize);

		// set timestamp
		avColor->pts = timestamp;

		// encode color and pushes into buffer
		encode(codecx, avColor, pktColor, *colorbuffer);

		// depth: cutoff distance
		Mat depthtmp = Mat(Size(depth.get_width_pixels(), depth.get_height_pixels()), CV_16U, (void*)depth.get_buffer(), Mat::AUTO_STEP);
		Mat depthcut = Mat(Size(depth.get_width_pixels(), depth.get_height_pixels()), CV_16U);
		cv::threshold(depthtmp, depthcut, 5000, 0, THRESH_TOZERO_INV); // 5m cutoff todo take from config

		//encode depth with rvl
		shared_ptr<vector<char>> depthrvl = make_shared<vector<char>>();
		depthrvl->resize(depth.get_size()); //malloc necessary memory
		int bytecount = ff_librvldepth_compress_rvl((short*)depthcut.data, depthrvl->data(), depth.get_width_pixels() * depth.get_height_pixels());
		depthrvl->resize(bytecount); //cut of memory for compressed image
		packetlock.lock();
		depthbuffer->push_back(depthrvl);
		packetlock.unlock();

		timestamp++;
	}

	/* flush the encoders */
	encode(codecx, NULL, pktColor, *colorbuffer);

	avcodec_free_context(&codecx);
	av_frame_free(&avColor);
	av_packet_free(&pktColor);

	return 0;
}



//############################################################################################################
//################################## Fast Lossless Depth Image Compression ###################################
//############################################################################################################

int ff_librvldepth_compress_rvl(const short* input, char* output, int numPixels) {
	int* buffer, * pBuffer, word, nibblesWritten, value;
	int zeros, nonzeros, nibble;
	int i;
	const short* p;
	const short* end;
	short previous;
	short current;
	int delta;
	int positive;
	word = 0;
	buffer = pBuffer = (int*)output;
	nibblesWritten = 0;
	end = input + numPixels;
	previous = 0;
	while (input != end) {
		zeros = 0, nonzeros = 0;
		for (; (input != end) && !*input; input++, zeros++);
		// EncodeVLE(zeros);  // number of zeros
		value = zeros;
		do {
			nibble = value & 0x7;  // lower 3 bits
			if (value >>= 3) nibble |= 0x8;  // more to come
			word <<= 4;
			word |= nibble;
			if (++nibblesWritten == 8) {
				// output word
				*pBuffer++ = word;
				nibblesWritten = 0;
				word = 0;
			}
		} while (value);
		for (p = input; (p != end) && *p++; nonzeros++);
		// EncodeVLE(nonzeros);
		value = nonzeros;
		do {
			nibble = value & 0x7;  // lower 3 bits
			if (value >>= 3) nibble |= 0x8;  // more to come
			word <<= 4;
			word |= nibble;
			if (++nibblesWritten == 8) {
				// output word
				*pBuffer++ = word;
				nibblesWritten = 0;
				word = 0;
			}
		} while (value);
		for (i = 0; i < nonzeros; i++) {
			current = *input++;
			delta = current - previous;
			positive = (delta << 1) ^ (delta >> 31);
			// EncodeVLE(positive);  // nonzero value
			value = positive;
			do {
				nibble = value & 0x7;  // lower 3 bits
				if (value >>= 3) nibble |= 0x8;  // more to come
				word <<= 4;
				word |= nibble;
				if (++nibblesWritten == 8) {
					// output word
					*pBuffer++ = word;
					nibblesWritten = 0;
					word = 0;
				}
			} while (value);
			previous = current;
		}
	}
	if (nibblesWritten)  // last few values
		*pBuffer++ = word << 4 * (8 - nibblesWritten);
	return (int)((char*)pBuffer - (char*)buffer);  // num bytes
}

void ff_librvldepth_decompress_rvl(const char* input, short* output, int numPixels) {
	const int* buffer, * pBuffer;
	int word, nibblesWritten, value, bits;
	unsigned int nibble;
	short current, previous;
	int numPixelsToDecode;
	int positive, zeros, nonzeros, delta;
	numPixelsToDecode = numPixels;
	buffer = pBuffer = (const int*)input;
	nibblesWritten = 0;
	previous = 0;
	while (numPixelsToDecode) {
		// int zeros = DecodeVLE();  // number of zeros
		value = 0;
		bits = 29;
		do {
			if (!nibblesWritten) {
				word = *pBuffer++;
				nibblesWritten = 8;
			}
			nibble = word & 0xf0000000;
			value |= (nibble << 1) >> bits;
			word <<= 4;
			nibblesWritten--;
			bits -= 3;
		} while (nibble & 0x80000000);
		zeros = value;
		numPixelsToDecode -= zeros;
		for (; zeros; zeros--)
			*output++ = 0;
		// int nonzeros = DecodeVLE();  // number of nonzeros
		value = 0;
		bits = 29;
		do {
			if (!nibblesWritten) {
				word = *pBuffer++;
				nibblesWritten = 8;
			}
			nibble = word & 0xf0000000;
			value |= (nibble << 1) >> bits;
			word <<= 4;
			nibblesWritten--;
			bits -= 3;
		} while (nibble & 0x80000000);
		nonzeros = value;
		numPixelsToDecode -= nonzeros;
		for (; nonzeros; nonzeros--) {
			// int positive = DecodeVLE();  // nonzero value
			value = 0;
			bits = 29;
			do {
				if (!nibblesWritten) {
					word = *pBuffer++;
					nibblesWritten = 8;
				}
				nibble = word & 0xf0000000;
				value |= (nibble << 1) >> bits;
				word <<= 4;
				nibblesWritten--;
				bits -= 3;
			} while (nibble & 0x80000000);
			positive = value;
			delta = (positive >> 1) ^ -(positive & 1);
			current = previous + delta;
			*output++ = current;
			previous = current;
		}
	}
}