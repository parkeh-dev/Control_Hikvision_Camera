#include <stdio.h>
#include <string>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "MvCameraControl.h"

unsigned int		g_nPayloadSize = 1440 * 1080;													// cache size 
unsigned char*		pData = (unsigned char *)malloc(sizeof(unsigned char) * (g_nPayloadSize));		// cache address of picture data

using namespace std;
using namespace cv;

void* CreateCamera(string cameraname)
{
	int		nRet	= MV_OK;					// status 
	void*	handle	= NULL;						// camera handle

	// Enum device
	MV_CC_DEVICE_INFO_LIST stDeviceList;							// device list
	memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));		// initialize
	
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDeviceList);			// get device list
	if (MV_OK != nRet)
		printf("Enum Devices fail! nRet [0x%x]\n", nRet);

	if (stDeviceList.nDeviceNum > 0)			// exists connected device 
	{					
		for (int i = 0; i < stDeviceList.nDeviceNum; i++) {
			// get camera model name
			String modelname((char*)stDeviceList.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chModelName);		
			
			// compare and create handle
			if (modelname == cameraname) {
				nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[i]);
				break;
			}
		}

		// open device
		nRet = MV_CC_OpenDevice(handle);			
		if (MV_OK != nRet)
			printf("Open Device fail! nRet [0x%x]\n", nRet);

		return handle;
	}else
		return NULL;
}

void GrabCamera(void* handle)
{
	int nRet = MV_OK;

	// Start grab image
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}
}

// convert data stream in Mat format
Mat Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData)
{
	Mat srcImage;

	srcImage = Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8U, pData);
	cvtColor(srcImage, srcImage, COLOR_BayerRG2RGB);

	return srcImage;
}

Mat GetMatFrame(void* handle)
{
	MV_FRAME_OUT_INFO_EX stImageInfo = { 0 };
	memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));

	Mat image;

	MV_CC_GetOneFrameTimeout(handle, pData, g_nPayloadSize, &stImageInfo, 1000);
	image = Convert2Mat(&stImageInfo, pData);

	return image;
}

void StopCamera(void* handle)
{
	int nRet = MV_OK;

	// Stop grab image
	nRet = MV_CC_StopGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
	}
}

void CloseCamera(void* handle)
{
	int nRet = MV_OK;

	// Close device
	nRet = MV_CC_CloseDevice(handle);
	if (MV_OK != nRet)
	{
		printf("ClosDevice fail! nRet [0x%x]\n", nRet);
	}

	// Destroy handle
	nRet = MV_CC_DestroyHandle(handle);
	if (MV_OK != nRet)
	{
		printf("Destroy Handle fail! nRet [0x%x]\n", nRet);
	}
}

void SetExposureAuto(void* handle, bool isauto)
{
	int nRet = MV_OK;

	if (isauto) {		// auto mode on
		nRet = MV_CC_SetEnumValue(handle, "ExposureAuto", 2);
	}
	else {				// auto mode off
		nRet = MV_CC_SetEnumValue(handle, "ExposureAuto", 0);
	}
	
	if (MV_OK != nRet)
	{
		printf("Set OnExposureAuto fail! nRet [0x%x]\n", nRet);
	}
}

void SetExposure(void* handle, float exposure)
{
	int nRet = MV_OK;

	nRet = MV_CC_SetFloatValue(handle, "ExposureTime", exposure);
	if (MV_OK != nRet)
	{
		printf("Set Exposure fail! nRet [0x%x]\n", nRet);
	}
}

void SetFramerate(void* handle, float framerate)
{
	int nRet = MV_OK;

	nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", framerate);
	if (MV_OK != nRet)
	{
		printf("Set FrameRate fail! nRet [0x%x]\n", nRet);
	}
}

void main() {

	void* handle;

	handle = CreateCamera("MV-CA016-10UC");			// Create Handle and Open

	SetExposureAuto(handle, false);					// set Exposure Auto off
	SetExposure(handle, 30000.0f);					// set Exposure
	SetFramerate(handle, 50.0f);					// set Frame rate

	// data type : BayerRG8
	MV_CC_SetEnumValue(handle, "PixelFormat", 0x01080009);

	GrabCamera(handle);

	Mat img;
	
	while (true) {
		img = GetMatFrame(handle);								// get frame
		resize(img, img, Size(img.cols / 2, img.rows / 2));		// resize

		imshow("camera", img);		

		char c = (char)waitKey(10);			
		if (c == 27) { 
			StopCamera(handle);						// stop camera
			CloseCamera(handle);					// close camera
			break; 
		}
	}
}