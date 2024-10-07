#pragma once
#include <opencv2/opencv.hpp>

void calculateLUTPiecewisePoints(const cv::Mat& img16bit, const cv::Rect& poleRegionROI, cv::Point2f& blackPoint, cv::Point2f& whitePoint, int& minIntensityImg, int& maxIntensityImg, int blackPointOutput = 10, int whitePointOutput = 210, float cutoffRatio = 0.01);
void generateToneMappingLUT(const cv::Point2f& blackPoint, const cv::Point2f& whitePoint, std::vector<uint8_t>& LUT, int minIntensityImg, int maxIntensityImg);
cv::Mat toneMapping(const cv::Mat& img16bit, const cv::Rect& poleRegionROI, float histCutOffRatio = 0.05,  int blackPointOutput = 10, int whitePointOutput = 230);
cv::Mat toneMapping(const cv::Mat& img16bit, std::vector<uint8_t>& LUT);
void convert32BitTo8(std::string inputFolder, std::string outputFolder, float gamma);
void MarshalString ( System::String ^ s, std::string& os );
void MarshalString ( System::String ^ s,std::wstring& os );