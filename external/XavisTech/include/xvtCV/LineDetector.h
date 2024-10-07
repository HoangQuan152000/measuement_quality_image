#pragma once
#include "xvtCV/xvtDefine.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace xvt {

/// l(alpha,r) =  x*sin(alpha) - y*cos(alpha) + r = 0
//Vector t(cos(alpha), sin(alpha)) is the normalized vector that colinear to the line
class XVT_EXPORTS Line
{
public:
	double alpha;//the angle of the line and the x axis
	double r;//Distance form the original to the line
	double rms;//root mean square error.
	double distanceThreshold;//Distance threshold from the line to the candidate points

	double vx; //Vector t(vx, vy) is the normalized vector that colinear to the line
	double vy; //Vector t(vx, vy) is the normalized vector that colinear to the line

	cv::Point2f x0;//Point on the line
	/*cv::Point2d startPoint;
	cv::Point2d endPoint;*/
	std::vector<cv::Point2d> points;//Ponits are belong to the line.

	Line():
		alpha{ 0.0 }, r{ 10.0 }, rms{ 0.0 }, vx{ 0 }, vy{ 0 }, x0{}, points{}
	{
		distanceThreshold = 1.5;
		/*startPoint.x = INTMAX_MAX;
		endPoint.x = INTMAX_MIN;*/
	}

	friend std::ostream& operator<<(std::ostream& os, const Line& l)
	{
		os << "alpha: " << l.alpha / CV_PI * 180 << ", r: " << l.r << ", rms: " << l.rms << ",vx: " << l.vx << ",vy: " << l.vy;
		return os;
	}

	static double DirectedFit(std::vector<cv::Point2d> points, Line& l);

	// Compute mean square error
	static double RMSError(std::vector<cv::Point2d> points, Line l);

	static bool Intersect(Line l1, Line l2, cv::Point2d& points);

	static void CalculateStartEndPoint(Line l, cv::Rect roi, cv::Point2d& start, cv::Point2d& end);
};

}
