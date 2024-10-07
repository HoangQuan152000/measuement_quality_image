#pragma once
#include "xvtCV/xvtTypes.h"
#include "xvtCV/xvtDefine.h"
#include <opencv2/core/types.hpp>

namespace xvt {

class XVT_EXPORTS Polyfit
{
public:
    Polyfit(std::vector<double>const& xdata, std::vector<double>const& ydata, int order);

    Polyfit(const std::vector<cv::Point>& points, int order);

    int GetOrder() const
    {
        return (std::max)(mCoeff.size() - 1,(size_t)0);
    }

    std::vector<double>const& GetCoeff() const&
    {
        return mCoeff;
    }

    std::vector<double>const GetCoeff() &&
    {
        return std::move(mCoeff);
    }

    double GetRMS() const
    {
        return mRMS;
    }

    double f(double x) const
    {
        return f(mCoeff, x);
    }

    void f(std::vector<double>const& x, std::vector<double>& y) const
    {
        f(mCoeff, x, y);
    }

    //This fitting used leastsquare method. This function only fits to the order bigger than 0
    //	Input: xdata, ydata, size of the data and the order of the output fitting polinomial. 
    //	Output: coeff array with the smallest order first: Coeff[0] +  Coeff[1]*x^1 +...+ Coeff[Order]*x^Order 
    static auto LSQFit(std::vector<double>const& xdata, std::vector<double>const& ydata, int order, std::vector<double>& coeff)->double;
    static auto LSQFit(std::vector<double>const& xdata, std::vector<double>const& ydata, int order, std::vector<double>& coeff, double& error)->bool;

    //Return the value of f(x)
    static auto f(std::vector<double>const& coeff, double x)->double;
    static void f(std::vector<double>const& coeff, std::vector<double>const& x, std::vector<double>& y);

    static auto LSQFit(const std::vector<cv::Point>& points, int order, std::vector<double>& coeff)->double;
    static bool LSQFit(const std::vector<cv::Point>& points, int order, std::vector<double>& coeff, double& error);
    static bool RansacFit(std::vector<cv::Point> const& points
                        , std::vector<cv::Point>& chosePoints
                        , std::vector<cv::Point>& outlierPoints
                        , std::vector<double>& coeff
                        , double& e
                        , int order
                        , double maxDistance = 1.5
                        , double coverRate = 0.01
    );

#pragma region not verify functions
public:
    //Find the maximum of f(xi): f(xi) | xi=[xStart:xEnd:step]. It will time out when run 1000 iteration even not find the maximum
    // Input: 
    //	 min_max <= 0 find minimum other find the maximum
    //   coeff: array with the smallest order first: Coeff[0] +  Coeff[1]*x^1 +...+ Coeff[Order]*x^Order.
    //   order: order of the polynomial.
    //   xStart, xEnd: range to find the maximum.
    //   e: acuracy. step: number of step in range.
    // Output:
    //   xMax: x position.
    //   maxValue: y value.
    static auto GetMinMax(VecDouble const& coeff
        , int min_max
        , float xStart, float xEnd
        , double& xMax, double& maxValue
        , double e = 1.e-3, int step = 1000
    )->double;

    //xStart is the start position
    //Change "alpha" to change the momemtum
    //min_max <= 0 find minimum other find the maximum
    static auto GetMaxMinDescent(VecDouble const& coeff
        , int min_max, float xStart
        , double& xMax, double& maxValue
        , float alpha = 0.09, int maxIteritor = 90000
        , double e = 1.e-7
    )->double;

private:
    //This function only works with order bigger than 0
    static VecDouble GetFirstDerivativeCoeff(VecDouble const& coeff);
#pragma endregion

private:
    static cv::Mat MakeDataMatrix(std::vector<double>const& xdata, int order);
    static double GetRMS(cv::Mat const& A, cv::Mat const& coeff, cv::Mat const& ydata);

private:
    //Co-efficient vector
    std::vector<double> mCoeff;
    //Root-Mean-Square error
    double mRMS = std::numeric_limits<double>::infinity();
};

}
