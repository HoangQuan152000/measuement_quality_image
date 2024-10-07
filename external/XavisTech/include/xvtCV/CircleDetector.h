#pragma once
#include "xvtCV/Utils.h"
#include "xvtCV/xvtRange.h"
#include "xvtCV/xvtDefine.h"
#include <opencv2/core/types.hpp>
#include <iostream>
#include <string>

namespace xvt {

class XVT_EXPORTS CircleDetector
{
public:
    CircleDetector();
    /// <summary>
    /// CircleDetector parameters
    /// </summary>
    /// <param name="radiusRange">Radius range of circle</param>
    /// <param name="dataThreshold">Threshold to consider a sample is belong to model</param>
    /// <param name="modelThreshold">Threshold for number of points that match circle</param>
    /// <param name="maxCoverRate">Max cover rate</param>
    CircleDetector(Ranged radiusRange, double dataThreshold, int modelThreshold, double maxCoverRate);

    bool Fit(const std::vector<cv::Point>& pointList, std::vector<cv::Point>& rChoosenPointList, std::vector<cv::Point>& rOutlierPointList,
             cv::Point2f& rCenterPoint, double& rPr, double& rPe, double& rCoverRate) const;

    bool FitRANSAC(const std::vector<cv::Point>& pointList, std::vector<cv::Point>& rChoosenPointList, std::vector<cv::Point>& rOutlierPointList,
                   cv::Point2f& rCenterPoint, double& rPr, double& rPe, double& rCoverRate) const;


    double GetDataThreshold() const { return mDataThreshold; }
    int GetModelThreshold() const { return mModelThreshold; }

    bool SetDataThreshold(double dataThreshold);
    bool SetModelThreshold(int modelThreshold);

    //====================================Static methodes ========================================

        //---------------------------------------------------------------------
        // Fits a circle to a given set of points. There must be at least 2 points
        // The circle equation is of the form: (x-xc)^2 + (y-yc)^2 = r^2
        // Returns true if there is a fit, false in case no circles can be fit
        // rPr: Predict radius
        // rPe: Predict error
        // refer: Least-Squares Circle Fit, Randy Bullock
    static bool FitLeastSquare(const std::vector<cv::Point>& pointList, cv::Point2f& rCenterPoint, double& rPr, double& rPe);
    // Compute mean square error
    static double ComputeRMSE(const std::vector<cv::Point>& pointList, double centerX, double centerY, double R);
    static bool IsOnCircle(double centerX, double centerY, double r, double x, double y, double tolerance);
public:
    //Enable or disable fitting. If disable the center of mass and average of width height will be return as center and radius
    bool mIsEnable = true;

    //Fitting method
    FittingMethod mFitMethod = FittingMethod::RANSAC;

    //Probability that after running this algorithm, we can find one set of correct model.
    double mSuccessProbability = 0.99;

    //Filter the fitted result that has the RMSE in the valid range
    Ranged mValidFitErrorRange = Ranged(0, 1.5, true);

    // The minimum number of data values required to fit the model
    int mModelSize = 3;

    //Searching radius range
    Ranged mRadiusRange;

    double mMaxCoverRate;

private:
    //Threshold to consider a sample is belong to model
    double mDataThreshold;

    //Threshold for number of points that match circle. Minimum is 2
    int mModelThreshold;
};

}
