#pragma once
#ifndef _METRIC_CAL_H
#define _METRIC_CAL_H
#define CVPLOT_HEADER_ONLY
#include <CvPlot/cvplot.h>
#include <numeric>
#include <complex>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

typedef std::complex <double> complex_t;
#define eps 1e-6 

struct Metric
{
	int xLeft = 0;
	int xRight = 0;
	int widthLeft = 0;
	int widthRight = 0;
	double SNRLeft = 0;
	double SNRRight = 0;
	double CTFLeft = 0;
	double CTFRight = 0;
	double CNRLeft = 0;
	double CNRRight = 0;
	std::vector<double> cumulativeFunc;
	std::vector<double> CTFLeftList;
	std::vector<double> CTFRightList;
	std::vector<double> CNRLeftList;
	std::vector<double> CNRRightList;
	std::vector<int> xCoordinateLeftList;
	std::vector<int> xCoordinateRightList;
};

void noiseEstimation(const cv::Mat& src, std::vector <double>& cummulativeFunc, std::vector <double>& smoothedSignal, double&Psignal, double& Pnoise, std::vector<int>& listPeak, std::vector<double>& CTFList, std::vector<double>& slopeList, double& CNR, std::vector<double>& CNRList, std::vector<int>& xCoordinate);
void MetricCalculation(const cv::Mat& src, Metric& metric);
void MetricCalculationWholeRegion(const cv::Mat& src, Metric& metric);
#endif