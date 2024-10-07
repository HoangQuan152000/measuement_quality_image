#include "TMO.h"
#include <opencv2/opencv.hpp>

void calculateLUTPiecewisePoints(const cv::Mat& img16bit, const cv::Rect& poleRegionROI, cv::Point2f& blackPoint, cv::Point2f& whitePoint, int& minIntensityImg, int& maxIntensityImg, int blackPointOutput, int whitePointOutput, float cutoffRatios)
{
	/*cv::Mat hdrMantiuk = img16bit.clone();
	hdrMantiuk.convertTo(hdrMantiuk, CV_32FC1);
	cv::cvtColor(hdrMantiuk, hdrMantiuk, cv::COLOR_GRAY2BGR);
	cv::Mat ldrMantiuk;
	cv::Ptr<cv::TonemapMantiuk> tonemapMantiuk = cv::createTonemapMantiuk(2.2,0.85, 1.2);
	tonemapMantiuk->process(hdrMantiuk, ldrMantiuk);
	ldrMantiuk = ldrMantiuk * 65535;
	cv::cvtColor(ldrMantiuk, ldrMantiuk, cv::COLOR_BGR2GRAY);
	ldrMantiuk.convertTo(img16bit, CV_16UC1);*/
	
	if (!img16bit.empty() && img16bit.type() == CV_16UC1)
	{
		//Todo: Need a step of RoiRefinement here
		cv::Mat imgPole = img16bit(poleRegionROI);
		double minVal; 
		double maxVal;

		cv::minMaxIdx(img16bit, &minVal, &maxVal);
		minIntensityImg = (int)floor(minVal);
		maxIntensityImg = (int)ceil(maxVal);

		cv::minMaxIdx(imgPole, &minVal, &maxVal);
		if (maxVal > 65534)
			maxVal = 65534;

		if (cutoffRatios > 0)
		{
			/*Histogram cutoff*/
			int lbins = (int)ceil(maxVal) - (int)floor(minVal) + 1;
			int histSize[] = {lbins};
			float lranges [] = {(int)floor(minVal), (int)ceil(maxVal) + 1};
			const float* ranges[]  = {lranges};
			int channels[] = {0};
			cv::MatND hist;

			cv::calcHist(&imgPole, 1, channels, cv::Mat(), hist, 1, histSize, ranges, true, false);

			float sum = cv::sum(hist)[0];
			float cutoffSum = sum * cutoffRatios;
			float sumAccumulate = 0;
			for (int i = 0; i < hist.total(); i++)
			{
				sumAccumulate += hist.at<float>(i);
				if (sumAccumulate > cutoffSum)
				{
					minVal = minVal + i;
					break;
				}
			}

			sumAccumulate = 0;
			for (int i = hist.total() - 1; i >= 0; i--)
			{
				sumAccumulate += hist.at<float>(i);
				if (sumAccumulate > cutoffSum)
				{
					maxVal = maxVal - (hist.total() - 1) + i;
					break;
				}
			}
		}
		
		if (blackPointOutput < 0)
			blackPointOutput = 0;
		if (blackPointOutput > UCHAR_MAX)
			blackPointOutput = UCHAR_MAX;
		if (whitePointOutput < 0)
			whitePointOutput = 0;
		if (whitePointOutput > UCHAR_MAX)
			whitePointOutput = UCHAR_MAX;
		
		blackPoint = cv::Point2f((int)floor(minVal), blackPointOutput);
		whitePoint = cv::Point2f((int)ceil(maxVal), whitePointOutput);

		if (blackPoint.x < blackPoint.y)
			blackPoint.y = blackPoint.x;
		
		if ((whitePoint.x - blackPoint.x) < (whitePoint.y - blackPoint.y))
			whitePoint.y = blackPoint.y + (whitePoint.x - blackPoint.x);
		if (whitePoint.y > UCHAR_MAX)
			whitePoint.y = UCHAR_MAX;
		
	}
}

void generateToneMappingLUT(const cv::Point2f& blackPoint, const cv::Point2f& whitePoint, std::vector<uint8_t>& LUT, int minIntensityImg, int maxIntensityImg)
{
	if (blackPoint.x >=0 && blackPoint.x <= whitePoint.x && whitePoint.x <= USHRT_MAX
		&& blackPoint.y >=0 && blackPoint.y <= whitePoint.y && whitePoint.y <= UCHAR_MAX)
	{
		LUT = std::vector<uint8_t>(USHRT_MAX + 1,0);
		float gamma = 0.6;
		int temp;
		for (int i = 0; i <= USHRT_MAX; i++)
		{
			if (i <= minIntensityImg)
			{
				temp = 0;
			}
			else if (i >= maxIntensityImg)
			{
				temp = UCHAR_MAX;
			}
			else if (i <=  blackPoint.x)
			{
				temp = blackPoint.y/blackPoint.x * i;
			}
			else if (i >= whitePoint.x)
			{
				temp = (i - whitePoint.x) * (UCHAR_MAX - whitePoint.y) / (maxIntensityImg - whitePoint.x) + whitePoint.y;
			}
			else
			{
				//temp = (i - blackPoint.x)*(whitePoint.y - blackPoint.y)/(whitePoint.x - blackPoint.x) + blackPoint.y;
				temp = blackPoint.y + (whitePoint.y - blackPoint.y) * pow((i - blackPoint.x), gamma) / pow((whitePoint.x - blackPoint.x),gamma);
			}

			if (temp > UCHAR_MAX)
			{
				LUT[i] = UCHAR_MAX;
			}
			else if (temp < 0)
			{
				LUT[i] = 0;
			}
			else
			{
				LUT[i] = (uint8_t)temp;
			}
		}	
	}
}

cv::Mat toneMapping(const cv::Mat& img16bit, const cv::Rect& poleRegionROI, float histCutOffRatio, int blackPointOutput, int whitePointOutput)
{
	cv::Mat img8bit = cv::Mat(img16bit.rows, img16bit.cols, CV_8UC1);
	if (!img16bit.empty() && img16bit.type() == CV_16UC1)
	{
		cv::Point2f blackPoint, whitePoint;
		std::vector<uint8_t> LUT;
		int minIntensityImg, maxIntensityImg;
		calculateLUTPiecewisePoints(img16bit, poleRegionROI, blackPoint, whitePoint, minIntensityImg, maxIntensityImg, blackPointOutput, whitePointOutput, histCutOffRatio);
		generateToneMappingLUT(blackPoint, whitePoint, LUT, minIntensityImg, maxIntensityImg);

		if (LUT.size() == USHRT_MAX + 1)
		{
			uint8_t* ptr8 = img8bit.ptr<uint8_t>(0);
			const uint16_t* ptr16 = img16bit.ptr<uint16_t>(0);
			int size = img16bit.total();
			for (int i = 0; i < size; i++)
			{
				ptr8[i] = LUT[ptr16[i]];
			}
		}
	}
	return img8bit;
}

cv::Mat toneMapping(const cv::Mat& img16bit, std::vector<uint8_t>& LUT)
{
	cv::Mat img8bit = cv::Mat(img16bit.rows, img16bit.cols, CV_8UC1);
	if (!img16bit.empty() && img16bit.type() == CV_16UC1)
	{
		if (LUT.size() == USHRT_MAX + 1)
		{
			uint8_t* ptr8 = img8bit.ptr<uint8_t>(0);
			const uint16_t* ptr16 = img16bit.ptr<uint16_t>(0);
			int size = img16bit.total();
			for (int i = 0; i < size; i++)
			{
				ptr8[i] = LUT[ptr16[i]];
			}
		}
	}
	return img8bit;
}

void convert32BitTo8(std::string inputFolder, std::string outputFolder, float gamma)
{
	cv::String path(inputFolder + "/*.tif");
	std::vector<cv::String> imgName;
	cv::Mat inputImg;
	cv::glob(path, imgName, true);

	double minAll = 9999, maxAll = -50;
	double minTemp, maxTemp;
	for (int i = 0; i < imgName.size(); i++)
	{
		inputImg = cv::imread(imgName[i], cv::IMREAD_ANYDEPTH);
		if (!inputImg.empty() && inputImg.channels() == 1)
		{
			cv::minMaxIdx(inputImg, &minTemp, &maxTemp);
			if (minTemp < minAll)
			{
				minAll = minTemp;
			}
			if (maxTemp > maxAll)
			{
				maxAll = maxTemp;
			}
		}
	}
	
	for (int i = 0; i < imgName.size(); i++) 
	{
		inputImg = cv::imread(imgName[i], cv::IMREAD_ANYDEPTH);
		if (!inputImg.empty() && inputImg.channels() == 1)
		{
			//cv::normalize(inputImg, inputImg, 0.0, 1.0, cv::NORM_MINMAX);

			int size = inputImg.total();
			float* ptr = inputImg.ptr<float>(0);
			for (int j = 0; j < size; j++)
			{
				if (ptr[j] < 0)
				{
					ptr[j] = 0;
				}
				else if (ptr[j] > 5)
				{
					ptr[j] = 1;
				}
				else
				{
					ptr[j] = pow((ptr[j] - 0)/(5 - 0), 1/gamma);
				}
				
			}

			inputImg.convertTo(inputImg, CV_8UC1, 255);

			std::string outputName = imgName[i];
			outputName.resize(outputName.size()-4);
			outputName = outputName.substr(outputName.find_last_of("/\\") + 1);
						
			cv::imwrite(outputFolder + "/" + outputName + ".bmp", inputImg);
			inputImg.release();
		}
		
	}
	std::cout << std::endl;
}

void MarshalString ( System::String ^ s, std::string& os ) {
   using namespace System::Runtime::InteropServices;
   const char* chars =
      (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
   os = chars;
   Marshal::FreeHGlobal(System::IntPtr((void*)chars));
}

void MarshalString ( System::String ^ s,std::wstring& os ) {
   using namespace System::Runtime::InteropServices;
   const wchar_t* chars =
      (const wchar_t*)(Marshal::StringToHGlobalUni(s)).ToPointer();
   os = chars;
   Marshal::FreeHGlobal(System::IntPtr((void*)chars));
}