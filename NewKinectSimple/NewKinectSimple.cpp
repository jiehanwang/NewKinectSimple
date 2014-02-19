// NewKinectSimple.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "opencv2\opencv.hpp"
using namespace cv;
using namespace std;

// Current Kinect
IKinectSensor*          m_pKinectSensor;

// Depth reader
//IDepthFrameReader*      m_pDepthFrameReader;

// Color reader
IColorFrameReader*      m_pColorFrameReader(NULL);

RGBQUAD*                m_pColorRGBX;
static const int        cColorWidth  = 1920;
static const int        cColorHeight = 1080;

HRESULT InitializeDefaultSensor()
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get the color reader
		IColorFrameSource* pColorFrameSource = NULL;

		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
		}

		SafeRelease(pColorFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		cout<<"No ready Kinect found!"<<endl;
		return E_FAIL;
	}

	return hr;
}

bool Update(IplImage* img)
{
	if (!m_pColorFrameReader)
	{
		return false;
	}

	IColorFrame* pColorFrame = NULL;

	HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);

	bool retu;
	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nBufferSize = 0;
		RGBQUAD *pBuffer = NULL;

		hr = pColorFrame->get_RelativeTime(&nTime);

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_FrameDescription(&pFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Width(&nWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Height(&nHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
			}
			else if (m_pColorRGBX)
			{
				pBuffer = m_pColorRGBX;
				nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);            
			}
			else
			{
				hr = E_FAIL;
			}
		}

		if (SUCCEEDED(hr))
		{
			//ProcessColor(nTime, pBuffer, nWidth, nHeight);
			for (int y=0;y<nHeight;y++)
			{
				uchar* src_ptr = (uchar*)(img->imageData + y*img->widthStep);
				for (int x=0;x<nWidth;x++)
				{
					(BYTE)src_ptr[3*x+0] = pBuffer[y*nWidth + x].rgbBlue;
					(BYTE)src_ptr[3*x+1] = pBuffer[y*nWidth + x].rgbGreen;
					(BYTE)src_ptr[3*x+2] = pBuffer[y*nWidth + x].rgbRed;
					
				}
			}
		}

		SafeRelease(pFrameDescription);
		retu = true;
	}
	else
	{
		retu = false;
	}

	SafeRelease(pColorFrame);
	return retu;


}

int _tmain(int argc, _TCHAR* argv[])
{

	// create heap storage for color pixel data in RGBX format
	m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];

	InitializeDefaultSensor();

	IplImage* showImg = cvCreateImage(cvSize(cColorWidth,cColorHeight),8,3);
	IplImage* showImg_640_480 = cvCreateImage(cvSize(640,480),8,3);
	cvNamedWindow("show",1);
	
	while (1)
	{
		Update(showImg);
		cvResize(showImg, showImg_640_480);
		cvShowImage("show",showImg_640_480);
		waitKey(10);
	}
	
	return 0;
}

