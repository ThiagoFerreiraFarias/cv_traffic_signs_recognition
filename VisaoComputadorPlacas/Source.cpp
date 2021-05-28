#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\imgproc.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>

extern "C" {
	#include "vc.h"
}


using namespace cv;
using namespace std;

const int fps = 120;
const int frameWidth = 640;
const int frameHeight = 480;

//Para filtragem de cores hsv
int hmin = 0, smin = 0, vmin = 0;
int hmax = 179, smax = 255, vmax = 255;


//Para filtragem de vermelho
int redHmin = 108, redSmin = 151, redVmin = 135;
int redHmax = 127, redSmax = 255, redVmax = 255;

//Para filtragem de azul
int blueHmin = 0, blueSmin = 24, blueVmin = 20;
int blueHmax = 26, blueSmax = 255, blueVmax = 255;

//tools box para ajustar o kernel
int threshouldOne = 50;
int threshouldTwo = 100;

int approxCurve = 0.001;

//para identificacao de contornos
String getSignNameByNumber(int);


int main(void) {

	Mat frame;
	
	//VideoCapture vid(0);

	char videofile[28] = "Opencv.mp4";
	cv::VideoCapture vid;
	vid.open(videofile);
	vid.set(3, frameWidth);
	vid.set(4, frameHeight);

	//vid.set(CAP_PROP_FPS, 160);

	std::string str;
	int key = 0;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	

	if (!vid.isOpened()) {
		return -1;
	}

	/**

	//Para o filtro de cores HSV cria uma barra de opções
	namedWindow("trackbars", (640, 200));
	createTrackbar("Hue Min", "trackbars", &blueHmin, 179);
	createTrackbar("Hue Max", "trackbars", &blueHmax, 179);
	createTrackbar("Saturation Min", "trackbars", &blueSmin, 255);
	createTrackbar("Saturation Max", "trackbars", &blueSmax, 255);
	createTrackbar("Value Min", "trackbars", &blueVmin, 255);
	createTrackbar("Value Max", "trackbars", &blueVmax, 255);
	**/

	// N�mero total de frames no v�deo 
	video.ntotalframes = (int)vid.get(cv::CAP_PROP_FRAME_COUNT);
	// Frame rate do v�deo 
	video.fps = (int)vid.get(cv::CAP_PROP_FPS);
	// Resolucaoo do v�deo 
	video.width = (int)vid.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT);


	Mat frame_1;
	Mat frame_maskRed;
	Mat frame_maskBlue;
	Mat frame_3;
	Mat frame_4;

	while (vid.read(frame)) {

		//vid.read(frame);
		frame_1 = frame.clone();
		frame_3 = frame.clone();
		frame_4 = frame.clone();

		Mat imgHsv, maskRed, maskBlue;
		cvtColor(frame, imgHsv, COLOR_RGB2HSV);
		Scalar lowerRed(redHmin, redSmin, redVmin);
		Scalar upperRed(redHmax, redSmax, redVmax);
		inRange(imgHsv, lowerRed, upperRed, maskRed);

		Scalar lowerBlue(blueHmin, blueSmin, blueVmin);
		Scalar upperBlue(blueHmax, blueSmax, blueVmax);
		inRange(imgHsv, lowerBlue, upperBlue, maskBlue);

		frame_maskRed = maskRed.clone();
		frame_maskBlue = maskBlue.clone();

		IVC* segmentedImageRed = vc_image_new(video.width, video.height, 3, 255);
		IVC* segmentedImageBlue = vc_image_new(video.width, video.height, 3, 255);

		IVC* segmentedImageOneChanellRed = vc_image_new(video.width, video.height, 1, 255);
		IVC* segmentedImageOneChanellBlue = vc_image_new(video.width, video.height, 1, 255);
		IVC* segmentedImageOneChanellBlueClose = vc_image_new(video.width, video.height, 1, 255);

		int vectorSize = video.width * video.height * 1;

		for (int x = 0; x < vectorSize; x += 1) {

			int segmentPost = x * 3;
			segmentedImageRed->data[segmentPost] = frame_maskRed.data[x];
			segmentedImageRed->data[segmentPost + 1] = frame_maskRed.data[x];
			segmentedImageRed->data[segmentPost + 2] = frame_maskRed.data[x];

			segmentedImageBlue->data[segmentPost] = frame_maskBlue.data[x];
			segmentedImageBlue->data[segmentPost + 1] = frame_maskBlue.data[x];
			segmentedImageBlue->data[segmentPost + 2] = frame_maskBlue.data[x];

			segmentedImageOneChanellRed->data[x] = frame_maskRed.data[x];
			segmentedImageOneChanellBlue->data[x] = frame_maskBlue.data[x];
		}


		IVC* imageRedIsolated = vc_image_new(video.width, video.height, 3, 255);
		IVC* imageBlueIsolated = vc_image_new(video.width, video.height, 3, 255);

		memcpy(imageRedIsolated->data, frame_1.data, video.width * video.height * 3);
		memcpy(imageBlueIsolated->data, frame_1.data, video.width * video.height * 3);

		vector<IVC*> vectorChannel(2);
		

		int kernelErode = video.height <720 ? 3 : 7;

		vc_binary_close(segmentedImageOneChanellBlue, kernelErode, kernelErode);

		for (int x = 0; x < vectorSize; x += 1) {
			int segmentPost = x * 3;

			segmentedImageBlue->data[segmentPost] = segmentedImageOneChanellBlue->data[x];
			segmentedImageBlue->data[segmentPost + 1] = segmentedImageOneChanellBlue->data[x];
			segmentedImageBlue->data[segmentPost + 2] = segmentedImageOneChanellBlue->data[x];


			segmentedImageOneChanellBlue->data[x] = segmentedImageOneChanellBlue->data[x];

		}

		
		vectorChannel[0] = segmentedImageOneChanellRed;
		vectorChannel[1] = segmentedImageOneChanellBlue;

		//estas isolam a imagem mostrando todo o resto preto
		//extractImage(imageRedIsolated, segmentedImageRed);
		//extractImage(imageBlueIsolated, segmentedImageBlue);



		memcpy(frame_3.data, segmentedImageRed->data, video.width * video.height * 3);
		memcpy(frame_4.data, segmentedImageBlue->data, video.width * video.height * 3);

		imshow("frame_3", frame_3);
		imshow("frame_4", frame_4);
		imshow("HSV2", imgHsv);
		imshow("maskRed", maskRed);
		imshow("maskBlue", maskBlue);

		
		const int SIZE = 2;

		vector<OVC*> vectorblob(SIZE);
		vector<IVC*> vectorImageOut(SIZE);
		int totalLabelsRed = 0;
		int totalLabelsBlue = 0;

		IVC* imageOutRed = vc_image_new(video.width, video.height, 1, 255);
		IVC* imageOutBlue = vc_image_new(video.width, video.height, 1, 255);
		vectorImageOut[0] = imageOutRed;
		vectorImageOut[1] = imageOutBlue;

		
		for (int segmentation = 0; segmentation < SIZE; segmentation++) {
			int* nLabels = (int*)malloc(sizeof(int));


			OVC* blobOut = vc_binary_blob_labelling(vectorChannel[segmentation], vectorImageOut[segmentation], nLabels);
			if (blobOut != NULL) {
				vectorblob[segmentation] = blobOut;
				int num = *nLabels;
				segmentation == 0 ? totalLabelsRed = num : totalLabelsBlue = num;
			}
		}


		IVC* imageBoundingBoxes = vc_image_new(video.width, video.height, 3, 255);
		memcpy(imageBoundingBoxes->data, frame_1.data, video.width * video.height * 3);

		int totalLabels = totalLabelsRed + totalLabelsBlue;
		vector<Coord*> coordChannelVector(totalLabels);
		vector<string> coordChannelColor(totalLabels);
		vector<string> coordType(totalLabels);
		vector<int> vectorAreaPoints(totalLabels);
		int areaPosition = 0;

		for (int segm = 0; segm < vectorblob.size(); segm++) {

			int numLabels = segm == 0 ? totalLabelsRed : totalLabelsBlue;

			vc_binary_blob_info(vectorImageOut[segm], vectorblob[segm], numLabels);

			for (int position = 0; position < numLabels; position++) {
				if (vectorblob[segm][position].area >5000) {
					Coord* coord = drawBlobBox(imageBoundingBoxes, vectorblob[segm][position]);
					coordChannelVector[areaPosition] = coord;
					coordChannelColor[areaPosition] = segm == 0 ? "Cor Vermelha" : "Cor Azul";
					int type = analyzesQuadrants(imageBoundingBoxes, vectorblob[segm][position], segm);
					coordType[areaPosition] = getSignNameByNumber(type);
					vectorAreaPoints[areaPosition] = position;
					areaPosition++;
				}
			}

		}

		
		memcpy(frame_1.data, imageBoundingBoxes->data, video.width * video.height * 3);

		for (int pos = 0; pos < coordChannelVector.size(); pos++) {
			if (coordChannelVector[pos] != 0) {
				Coord* coord = coordChannelVector[pos];
				if (coordType[pos] != "") {
					cv::putText(frame_1, "Placa identificada -" + coordChannelColor[pos], cv::Point(coord->eixoXstart, coord->eixoYend + 35), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 140, 255), 1.8);
					cv::putText(frame_1, "Type: " + coordType[pos], cv::Point(coord->eixoXstart, coord->eixoYend + 55), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 140, 255),1.8);
				}
			}
		}
	

		//display images
		imshow("frame_1", frame_1);

		//show other parts of the process


		waitKey(fps/100);
		
	}
	
	//vid.release();

	return -1;
}


String getSignNameByNumber(int type) {
	String typeDesc;

	switch (type) {
	case 1:
		typeDesc = "arrow left";
		break;
	case 2:
		typeDesc = "arrow right";
		break;
	case 3:
		typeDesc = "Car/Parking";
		break;
	case 4:
		typeDesc = "Highway";
		break;
	case 5:
		typeDesc = "Forbidden";
		break;
	case 6:
		typeDesc = "Stop";
		break;
	default:
		typeDesc = "";
		break;
	};
	return typeDesc;
}