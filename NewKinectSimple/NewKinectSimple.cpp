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
IDepthFrameReader*      m_pDepthFrameReader(NULL);

// Color reader
IColorFrameReader*      m_pColorFrameReader(NULL);

RGBQUAD*                m_pColorRGBX;
static const int        cColorWidth  = 1920;
static const int        cColorHeight = 1080;
static const int        cDepthWidth  = 512;
static const int        cDepthHeight = 424;

HRESULT InitializeDefaultSensor()
{
	HRESULT hr;
	HRESULT hr_color;
	HRESULT hr_depth;
	HRESULT hr_skeleton;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get the color and depth reader
		IColorFrameSource* pColorFrameSource = NULL;
		IDepthFrameSource* pDepthFrameSource = NULL;

		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr_color = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
			hr_depth = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr_color) && SUCCEEDED(hr_depth))
		{
			hr_color = pColorFrameSource->OpenReader(&m_pColorFrameReader);
			hr_depth = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		SafeRelease(pColorFrameSource);
		SafeRelease(pDepthFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr_color) || FAILED(hr_depth))
	{
		cout<<"No ready Kinect found!"<<endl;
		return E_FAIL;
	}

	return hr;
}

bool UpdateColor(IplImage* img)
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

void UpdateDepth(IplImage* img)
{
	if (!m_pDepthFrameReader)
	{
		return;
	}

	IDepthFrame* pDepthFrame = NULL;

	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		USHORT nDepthMinReliableDistance = 0;
		USHORT nDepthMaxReliableDistance = 0;
		UINT nBufferSize = 0;
		UINT16 *pBuffer = NULL;

		hr = pDepthFrame->get_RelativeTime(&nTime);

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
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
			hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxReliableDistance);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);            
		}

		if (SUCCEEDED(hr))
		{
			//ProcessDepth(nTime, pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxReliableDistance);
			for (int y=0;y<nHeight;y++)
			{
				uchar* src_ptr = (uchar*)(img->imageData + y*img->widthStep);
				for (int x=0;x<nWidth;x++)
				{
					(BYTE)src_ptr[x] = (pBuffer[y*nWidth + x]-nDepthMinReliableDistance)*255/nDepthMaxReliableDistance;

				}
			}
		}

		SafeRelease(pFrameDescription);
	}

	SafeRelease(pDepthFrame);
}

int _tmain(int argc, _TCHAR* argv[])
{

	// create heap storage for color pixel data in RGBX format
	m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];

	InitializeDefaultSensor();

	IplImage* showImg_color = cvCreateImage(cvSize(cColorWidth,cColorHeight),8,3);
	IplImage* showImg_depth = cvCreateImage(cvSize(cDepthWidth,cDepthHeight),8,1);
	IplImage* showImg_color_640_480 = cvCreateImage(cvSize(640,480),8,3);
	IplImage* showImg_depth_640_480 = cvCreateImage(cvSize(640,480),8,1);
	cvNamedWindow("show_color",1);
	cvNamedWindow("show_depth",1);
	
	while (1)
	{
		UpdateColor(showImg_color);
		UpdateDepth(showImg_depth);
		cvResize(showImg_color, showImg_color_640_480);
		cvResize(showImg_depth,showImg_depth_640_480);
		cvShowImage("show_color",showImg_color_640_480);
		cvShowImage("show_depth",showImg_depth);
		waitKey(10);
	}
	
	cvDestroyWindow("show_color");
	cvDestroyWindow("show_depth");


	//Release
	cvReleaseImage(&showImg_color);
	cvReleaseImage(&showImg_depth);
	cvReleaseImage(&showImg_color_640_480);
	cvReleaseImage(&showImg_depth_640_480);
	if (m_pColorRGBX)
	{
		delete [] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}

	// done with color frame reader
	SafeRelease(m_pColorFrameReader);

	// close the Kinect Sensor
	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}
	SafeRelease(m_pKinectSensor);
	return 0;
}

