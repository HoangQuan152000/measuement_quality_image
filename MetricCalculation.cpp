#include "MetricCalculation.h"

#ifdef _DEBUG
//#define DEBUG_PLOT
#endif

void noiseEstimation(const cv::Mat& src, std::vector <double>& cummulativeFunc, std::vector <double>& smoothedSignal, double& Psignal, double& Pnoise, std::vector<int>& listPeak, std::vector<double>& CTFList, std::vector<double>& slopeList, double& CNR, std::vector<double>& CNRList, std::vector<int>& xCoordinate)
{
	if (src.empty())
		return;

	if (!cummulativeFunc.empty())
		cummulativeFunc.clear();

	if (!smoothedSignal.empty())
		smoothedSignal.clear();

	if (!listPeak.empty())
		listPeak.clear();

	if (!CTFList.empty())
		CTFList.clear();

	if (!slopeList.empty())
		slopeList.clear();

	if (!CNRList.empty())
		CNRList.clear();
	
	if (!xCoordinate.empty())
		xCoordinate.clear();
	
	cv::Mat srcTemp, signal;
	src.convertTo(srcTemp, CV_32FC1);
	
	//Cumulative function vertically
	cv::reduce(srcTemp, signal, 0, cv::REDUCE_AVG, CV_32FC1);
	int signalSize = signal.cols;
	
	for (int i = 0; i < signalSize; i++)
	{
		cummulativeFunc.push_back(signal.at<float>(i));
	}
	
	/*Smoothing signal x~(n) estimation method: Using LPF in Fourier domain*/
	std::vector<double> noiseSignal;
	std::vector <double> sourceDFT;
	std::vector <double> DFTFilter;
	std::vector<complex_t> vectorDFT;
	for (int i = 0; i < signalSize; i++)
	{
		sourceDFT.push_back(signal.at<float>(i));
	}
	
	cv::blur(sourceDFT, smoothedSignal, cv::Size(3,1), cv::Point(-1,-1) ,cv::BORDER_REFLECT);

	//Step 2: Subtracting the smoothed signal to the average signal to create a scaled signal
	for (int i = 0; i < signalSize; i++)
	{
		noiseSignal.push_back(sourceDFT[i] - smoothedSignal[i]);
	}

	double meanOfNoise = !noiseSignal.empty() ? std::accumulate(noiseSignal.begin(), noiseSignal.end(), 0.0) / noiseSignal.size() : 0.0;

#ifdef DEBUG_PLOT
	cv::Mat mat(600, 1200, CV_8UC3);
	mat.setTo(cv::Scalar(255, 255, 255));

	CvPlot::Axes axes = CvPlot::makePlotAxes();
	axes.enableHorizontalGrid();
	axes.enableVerticalGrid();
	axes.title("ROI");

	axes.create<CvPlot::Series>(sourceDFT, "-b");
	axes.create<CvPlot::Series>(smoothedSignal, "-r");

	int borderLeft = 70, borderRight = 10, borderTop = 30, borderBottom = 30;
	axes.setMargins(borderLeft, borderRight, borderTop, borderBottom);
	axes.setXLim(std::pair<int, int>(0, smoothedSignal.size()));
	axes.setYLim(std::pair<int, int>(0, 8000));
	axes.setXTight(true);
	axes.render(mat);
#endif //DEBUG_PLOT

	//Calculate power of noise
	Pnoise = 0;
	for (auto& n: noiseSignal)
	{
		n -= meanOfNoise;
		Pnoise = Pnoise + n * n;

	}
    Pnoise /= signalSize;

	//Calculate power of signal
	Psignal = 0.0;
	for (int i = 0; i < signalSize; i++)
	{
		Psignal = Psignal + smoothedSignal[i] * smoothedSignal[i];
	}
	Psignal /= signalSize;

	/*20210705 Calculating slope (10-90% rise/fall distance)*/
	//Check max and min point in a windows size
	//This method can skip the last max/min peak. This should be resolve later
	int windowSize = (src.cols - 1 < 20) ? src.cols - 1 : 20;
	int initWindowSize  = (src.cols < 15) ? src.cols : 15;
	int maxPeak = 1, minPeak = 1;

	bool findMax = true; //flag for determine next peak/valley need to find is maxpeak or minpeak;
	for (int i = 1; i < initWindowSize; i++)
	{
		if (smoothedSignal[i] < smoothedSignal[minPeak])
			minPeak = i;
		else if (smoothedSignal[i] > smoothedSignal[maxPeak])
			maxPeak = i;
	}
	//Check if minpeak and max peak is valley/peak or not
	if (signalSize > minPeak + 1)
	{
		if (smoothedSignal[minPeak - 1] >= smoothedSignal[minPeak] && smoothedSignal[minPeak] <= smoothedSignal[minPeak + 1])
		{
			//It's a valley
		}
		else
		{
			for (int i = 1; i < windowSize; i++)
			{
				if (smoothedSignal[i - 1] >= smoothedSignal[i] && smoothedSignal[i] <= smoothedSignal[i + 1])
				{
					minPeak = i;
					break;
				}
			}
		}
	}
	if (signalSize > maxPeak + 1)
	{
		if (smoothedSignal[maxPeak - 1] <= smoothedSignal[maxPeak] && smoothedSignal[maxPeak] >= smoothedSignal[maxPeak + 1])
		{
			//It's a peak
		}
		else
		{
			for (int i = 1; i < windowSize; i++)
			{
				if (smoothedSignal[i - 1] <= smoothedSignal[i] && smoothedSignal[i] >= smoothedSignal[i + 1])
				{
					maxPeak = i;
					break;
				}
			}
		}
	}
	if (minPeak < maxPeak)
	{
		listPeak.push_back(minPeak);
		listPeak.push_back(maxPeak);
		findMax = false;
	}
	else
	{
		listPeak.push_back(maxPeak);
		listPeak.push_back(minPeak);
		findMax = true;
	}
	int iDx = listPeak.back();
	//int iDx = 30;
	while (iDx < signalSize - 1)
	{
		maxPeak = iDx;
		minPeak = iDx;
		int limitIndex = (iDx + windowSize / 2 < signalSize) ? (iDx + windowSize / 2) : (signalSize);
		for (int i = iDx + 1; i < limitIndex; i++)
		{
			if (findMax)
			{
				if (smoothedSignal[i] > smoothedSignal[maxPeak])
				{
					maxPeak = i;
				}
			}
			else
			{
				if (smoothedSignal[i] < smoothedSignal[minPeak])
				{
					minPeak = i;
				}
			}
		}

		if (findMax)
		{
			if (maxPeak != listPeak.back())
			{
				listPeak.push_back(maxPeak);
				iDx = maxPeak - windowSize / 2;
			}
		}
		else
		{
			if (minPeak != listPeak.back())
			{
				listPeak.push_back(minPeak);
				iDx = minPeak - windowSize / 2;
			}
		}

		iDx += windowSize / 2;
		findMax = !findMax;
	}
	//Remove the last point
	listPeak.pop_back();
	int listPeakSize = listPeak.size();

	/*Calculating CTF = (Imax-Imin/Imax+Imin) */
	/*https://www.edmundoptics.com/knowledge-center/application-notes/optics/introduction-to-modulation-transfer-function/  */
	for (int i = 0; i < listPeakSize - 1; i++)
	{
		// remove caculate only in rising case
		//if (smoothedSignal[listPeak[i]] - smoothedSignal[listPeak[i + 1]] < 0)//rising
		{
			double CTF = abs(smoothedSignal[listPeak[i]] - smoothedSignal[listPeak[i + 1]]) / (smoothedSignal[listPeak[i]] + smoothedSignal[listPeak[i + 1]]);
			CTFList.push_back(CTF*100);
		}
	}

#ifdef DEBUG_PLOT
	cv::Mat mat_ctf(600, 1200, CV_8UC3);
	mat_ctf.setTo(cv::Scalar(255, 255, 255));

	CvPlot::Axes axes_ctf = CvPlot::makePlotAxes();
	axes_ctf.enableHorizontalGrid();
	axes_ctf.enableVerticalGrid();
	axes_ctf.title("ROI");

	axes_ctf.create<CvPlot::Series>(CTFList, "-b");
	//axes.create<CvPlot::Series>(smoothedSignal, "-r");

	int boderLeft = 70, boderRight = 10, boderTop = 30, boderBottom = 30;
	axes_ctf.setMargins(borderLeft, borderRight, borderTop, borderBottom);
	axes_ctf.setXLim(std::pair<int, int>(0, CTFList.size()));
	axes_ctf.setYLim(std::pair<int, int>(0, 100));
	axes_ctf.setXTight(true);
	axes_ctf.render(mat_ctf);
#endif //  DEBUG_PLOT

	/*Calculating slope 10-90%*/
	for (int i = 0; i < listPeakSize - 1; i++)
	{
		double contrastValue = smoothedSignal[listPeak[i]] - smoothedSignal[listPeak[i + 1]];
		double baseContrastValue = (contrastValue > 0) ? smoothedSignal[listPeak[i + 1]] : smoothedSignal[listPeak[i]];
		double TenPercentValue = 0.1 * abs(contrastValue);
		double NinetyPercentValue = 0.9 * abs(contrastValue);
		//bool flagFirstPointDetected = false;
		int NinetyPercentPos = 0, TenPercentPos = 0;
		for (int j = listPeak[i]; j <= listPeak[i + 1]; j++)
		{
			if (contrastValue > 0) //fall
			{
				//Do nothing
			}
			else //rise
			{
				if (smoothedSignal[j] > baseContrastValue + TenPercentValue)
				{
					TenPercentPos = j;
					break;
				}
			}
		}

		for (int j = listPeak[i + 1]; j >= listPeak[i]; j--)
		{
			if (contrastValue > 0) //fall
			{
				//Do nothing
			}
			else // rise
			{
				if (smoothedSignal[j] < baseContrastValue + NinetyPercentValue)
				{
					NinetyPercentPos = j;
					break;
				}
			}
		}
		if (contrastValue < 0)
			slopeList.push_back(abs(contrastValue) / abs(NinetyPercentPos - TenPercentPos));
	}

	//double CTFAverage = std::accumulate(CTFList.begin(), CTFList.end(), 0.0) / CTFList.size();
	//double slopeAverage = std::accumulate(slopeList.begin(), slopeList.end(), 0.0) / slopeList.size();

	/*Calculating CNR - Contrast to Noise Ratio*/
	for (int i = 0; i < listPeakSize - 1; i++)
	{
		if (smoothedSignal[listPeak[i]] - smoothedSignal[listPeak[i + 1]] < 0)//rising
		{
			double CNR_temp = abs(smoothedSignal[listPeak[i]] - smoothedSignal[listPeak[i + 1]]);
			if (Pnoise > 0)
				CNRList.push_back(CNR_temp/sqrt(Pnoise));
			else
				CNRList.push_back(0);

			xCoordinate.push_back((listPeak[i] + listPeak[i + 1]) / 2);
		}
	}
	if (CNRList.size() != 0)
		CNR = std::accumulate(CNRList.begin(), CNRList.end(), 0.0) / CNRList.size();
	else
		CNR = 0;
}

void MetricCalculation(const cv::Mat& src, Metric& metric)
{
	double Pnoise = 0;
	double Psignal = 0;
	//For debugging
	std::vector <double> x_nEstimated;
	std::vector<int> peakPoint;
	std::vector<double> peakValue;
	std::vector<double> SlopeList;

	metric.cumulativeFunc.clear();
	metric.CTFLeftList.clear();
	metric.CNRLeftList.clear();
	noiseEstimation(src, metric.cumulativeFunc, x_nEstimated, Psignal, Pnoise, peakPoint, metric.CTFLeftList, SlopeList, metric.CNRLeft, metric.CNRLeftList, metric.xCoordinateLeftList);

	if (Psignal != 0 && Pnoise != 0)
		metric.SNRLeft = 10*log10(Psignal/Pnoise);
	else
		metric.SNRLeft = 0;
	if (metric.CTFLeftList.size() != 0)
		metric.CTFLeft = std::accumulate(metric.CTFLeftList.begin(), metric.CTFLeftList.end(), 0.0)/metric.CTFLeftList.size();
	else
		metric.CTFLeft = 0;

	metric.SNRRight = metric.SNRLeft;
	metric.CTFRight = metric.CTFLeft;
	metric.CNRRight = metric.CNRLeft;
	metric.CNRRightList = metric.CNRLeftList;
	metric.CTFRightList = metric.CTFLeftList;
	metric.xCoordinateRightList = metric.xCoordinateLeftList;
}

void MetricCalculationWholeRegion(const cv::Mat& src, Metric& metric)
{
	cv::Mat middleLeft, middleRight, signal, srcTemp;
	int midWidth = 170;
	int centerPos = src.cols/2;
	int offset = 50; // cutting unwanted region in outter and inner
	
	double PnoiseLeft = 0;
	double PsignalLeft = 0;

	double PnoiseRight = 0;
	double PsignalRight = 0;
	
	//For debugging
	std::vector <double> y_n;
	std::vector <double> x_nEstimated;
	std::vector<int> peakPoint;
	std::vector<double> peakValue;
	std::vector<double> SlopeList;

	cv::Mat srcClone = src.clone();
	srcClone.convertTo(srcTemp, CV_32FC1);
	
	//Cumulative function vertically
	cv::reduce(srcTemp, srcTemp, 0, cv::REDUCE_AVG, CV_32FC1);
	int signalSize = srcTemp.cols;

	metric.cumulativeFunc.clear();
	for (int i = 0; i < signalSize; i++)
	{
		metric.cumulativeFunc.push_back(srcTemp.at<float>(i));
	}


	metric.xLeft = 0;//(centerPos-midWidth/2)/3;
	metric.widthLeft = centerPos - midWidth/2;
	if (metric.widthLeft > 0 && src.rows > 0)
	{
		/*Middle Left*/
		middleLeft = src(cv::Rect(metric.xLeft, 0, metric.widthLeft, src.rows)).clone();
		noiseEstimation(middleLeft, y_n, x_nEstimated, PsignalLeft, PnoiseLeft, peakPoint, metric.CTFLeftList, SlopeList, metric.CNRLeft, metric.CNRLeftList, metric.xCoordinateLeftList);
		if (PsignalLeft != 0 && PnoiseLeft != 0)
			metric.SNRLeft = 10*log10(PsignalLeft/PnoiseLeft);
		else
			metric.SNRLeft = 0;
		if (metric.CTFLeftList.size() != 0)
			metric.CTFLeft = std::accumulate(metric.CTFLeftList.begin(), metric.CTFLeftList.end(), 0.0)/metric.CTFLeftList.size();
		else
			metric.CTFLeft = 0;
	}
	cv::Scalar mL = cv::Scalar(0);
	cv::Scalar stdDevL = cv::Scalar(0);
	cv::meanStdDev(metric.CTFLeftList, mL, stdDevL);
	float sL = stdDevL[0];

	y_n.clear();
	x_nEstimated.clear();
	peakPoint.clear();
	peakValue.clear();
	SlopeList.clear();

	metric.xRight = centerPos + midWidth / 2;
	metric.widthRight = centerPos - midWidth / 2;
	if (metric.xRight > 0 && metric.widthRight > 0 && src.rows > 0)
	{
		/*Middle Right*/
		middleRight = src(cv::Rect(metric.xRight, 0, metric.widthRight, src.rows)).clone();
		noiseEstimation(middleRight, y_n, x_nEstimated, PsignalRight, PnoiseRight, peakPoint, metric.CTFRightList, SlopeList, metric.CNRRight, metric.CNRRightList, metric.xCoordinateRightList);
		if (PsignalRight != 0 && PnoiseRight != 0)
			metric.SNRRight = 10*log10(PsignalRight/PnoiseRight);
		else
			metric.SNRRight = 0;
		if (metric.CTFRightList.size() != 0)
			metric.CTFRight = std::accumulate(metric.CTFRightList.begin(), metric.CTFRightList.end(), 0.0)/metric.CTFRightList.size();
		else
			metric.CTFRight = 0;
	}
	cv::Scalar mR = cv::Scalar(0);
	cv::Scalar stdDevR = cv::Scalar(0);
	cv::meanStdDev(metric.CTFRightList, mR, stdDevR);
	float sR = stdDevR[0];

	/*Get the average value*/
	/*SNR = (SNRLeft + SNRRight) / 2;
	CTF = (CTFLeft + CTFRight) / 2;
	CNR = (CNRLeft + CNRRight) / 2;*/

	/*Draw cumulative function*/
	/*if (src.cols > 0 && src.rows > 0)
	{
		src.convertTo(srcTemp, CV_16S);
		cv::reduce(srcTemp, signal, 0, cv::REDUCE_AVG, CV_32FC1);
		metric.cumulativeFunc.clear();
		for (int i = 0; i < signal.cols; i++)
		{
			metric.cumulativeFunc.push_back(signal.at<float>(i));
		}
	}*/
}