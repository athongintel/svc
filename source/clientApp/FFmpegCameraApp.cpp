#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <time.h>

#include "../src/svc/SVC.h"
#include "../src/svc/host/SVCHostIP.h"
#include "../src/svc/authenticator/SVCAuthenticatorSharedSecret.h"

#include "../src/utils/camera-util.h"

#define RETRY_TIME 5

using namespace cv;

using namespace std;

float timeDistance(const struct timespec* greater, const struct timespec* smaller){
	float sec = greater->tv_sec - smaller->tv_sec;
	float nsec;
	if (greater->tv_nsec < smaller->tv_nsec){
		sec -= 1;
		nsec = greater->tv_nsec + 1000000000 - smaller->tv_nsec;
	}
	else{
		nsec = greater->tv_nsec - smaller->tv_nsec;
	}
	nsec /= 1000000000;
	sec += nsec;
	return sec;
}

void sendPacket(SVCEndpoint* endpoint, uint8_t* imgData, int imgSize, int frameSeq) {
	//TODO: remove frameSeq

	if(endpoint==NULL) {
		return;
	}

	static const uint32_t bufferSize = 1400;
	static uint8_t buffer[bufferSize] = "";

	int packets = imgSize/(bufferSize-1);
	buffer[0] = 0x01;
	memcpy(buffer+1, &imgSize, 4);
	memcpy(buffer+1+4, &frameSeq, 4);
	for (int i=0;i<RETRY_TIME;i++){
		endpoint->sendData(buffer, 1+4+4);
	}

	buffer[0] = 0x02;
	for (int i = 0; i < packets; ++i)
	{
		memcpy(buffer+1, imgData+i*(bufferSize-1), bufferSize-1);
		endpoint->sendData(buffer,bufferSize);
	}

	int lastPacketSize = imgSize % (bufferSize-1);
	if(lastPacketSize != 0) {
		memcpy(buffer+1, imgData+packets*(bufferSize-1), lastPacketSize);
		endpoint->sendData(buffer,lastPacketSize+1);
	}

	buffer[0] = 0x03;
	for (int i=0;i<RETRY_TIME;i++){
		endpoint->sendData(buffer, 1);
	}

	printf("\nFrame %d sent, framesize = %d", frameSeq, imgSize);
}

void sendStream(SVCEndpoint* endpoint)
{
	initFFmpeg(1);
	AVFormatContext *inFormatCtx = NULL;
	AVCodecContext *pCodecCtx = NULL;
	int videoStream;

	if(!openCamera(&inFormatCtx, &pCodecCtx, &videoStream)){
		return;
	}
	// if(inFormatCtx == NULL || pCodecCtx==NULL) {
	// 	return;
	// }

	int width = pCodecCtx->width;
	int height = pCodecCtx->height;

	AVFrame *pFrame = NULL;
	pFrame=av_frame_alloc();

	AVFrame *pFrameYUV420 = NULL;
	pFrameYUV420 = new_av_frame(AV_PIX_FMT_YUV420P, width, height);

	/* video encoder*/
	AVCodecContext* encoderCtx;
	initEncoderContext(&encoderCtx, width, height);

	struct SwsContext *sws_ctx_YUV420P = NULL;
	sws_ctx_YUV420P = sws_getContext(pCodecCtx->width,
	    pCodecCtx->height,
	    pCodecCtx->pix_fmt,
	    pCodecCtx->width,
	    pCodecCtx->height,
	    PIX_FMT_YUV420P,
	    SWS_BILINEAR,
	    NULL,
	    NULL,
	    NULL
	    );

	Graphics* g = new Graphics(pCodecCtx->width,  pCodecCtx->height, "My Window");
	if(strcmp(g->getError(), "") != 0) {
		printf("SDL error: %s\n", g->getError());
		return;
	}

	SDL_Event       event;

	SDL_Rect sdlRect;  
    sdlRect.x = 0;  
    sdlRect.y = 0;  
    sdlRect.w = pCodecCtx->width;  
    sdlRect.h = pCodecCtx->height;  

	AVPacket packet;
	AVPacket outPacket;
	int gotOutput;
	int frameFinished;
	
	int frameSeq = 1;
	while(1)
    {
        av_read_frame(inFormatCtx, &packet);

        if(packet.stream_index==videoStream) {
	    	
	    	avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

	    	if(frameFinished) {	    
				sws_scale(sws_ctx_YUV420P, (uint8_t const * const *)pFrame->data,
				  pFrame->linesize, 0, pCodecCtx->height,
				  pFrameYUV420->data, pFrameYUV420->linesize);

				g->displayFFmpegYUVFrame(pFrameYUV420, &sdlRect);

				//encode frame to video
				av_init_packet(&outPacket);
	          	outPacket.data = NULL;
	          	outPacket.size = 0;
          		
          		pFrameYUV420->pts = (1.0 / 30) * 90 * frameSeq++;

          		if (avcodec_encode_video2(encoderCtx, &outPacket, pFrameYUV420, &gotOutput) < 0) {
			        fprintf(stderr, "Failed to encode frame\n");
			        continue;
			    }

			    if(gotOutput) {
			    	//send outPacket
			    	sendPacket(endpoint, outPacket.data, outPacket.size, frameSeq);
			    }
			}
        }

		SDL_Delay(50);
		SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
			SDL_Quit();
			break;
		}
        
    }
		// printf("\nimage sended!\n");
}

int main(int argc, char** argv){

	//int RETRY_TIME = atoi(argv[2]);
	SVCHost* remoteHost;
	
	// string appID = string("CAMERA_APP");
	string appID = string("CAMERA_APP");
	// SVCHost* remoteHost = new SVCHostIP("149.56.142.13");
	if (argc>1){
		remoteHost = new SVCHostIP(argv[1]);
	}
	else {
		remoteHost = new SVCHostIP("192.168.43.149");
	}

	SVCAuthenticatorSharedSecret* authenticator = new SVCAuthenticatorSharedSecret("./private/sharedsecret");

	try{
		SVC* svc = new SVC(appID, authenticator);
		struct timespec startingTime;
		struct timespec echelon;
		clock_gettime(CLOCK_REALTIME, &startingTime);
		
		SVCEndpoint* endpoint = svc->establishConnection(remoteHost, 0);
		if (endpoint!=NULL){
			if (endpoint->negotiate()){
				clock_gettime(CLOCK_REALTIME, &echelon);
				printf("\n[%0.2f] Connection established.", timeDistance(&echelon, &startingTime)); fflush(stdout);

				sendStream(endpoint);

			}
			else{
				printf("\nCannot establish connection. Program terminated.\n");
			}
			delete endpoint;
		}
		else {
			printf("\nCannot create the endpoint. Program terminated.\n");
		}
		svc->shutdownSVC();
		delete svc;
		
		clock_gettime(CLOCK_REALTIME, &echelon);
		printf("\n[%0.2f] Program terminated\n", timeDistance(&echelon, &startingTime)); fflush(stdout);
	}
	catch (const char* str){
		printf("\nError: %s\n", str);
	}
	
	delete authenticator;
	delete remoteHost;
	
		
}
