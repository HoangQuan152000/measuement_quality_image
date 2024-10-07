#pragma once
#include <string>
#include <vector>
#include <opencv2/core/types.hpp>
#include "xvtCV/xvtTypes.h"
#include "xvtCV/xvtDefine.h"
#include <iostream>
#include <xvtCV/xvtPen.h>
#include <xvtCV/IInspection.h>

namespace xvt {

class RoiInfo;
class IntensityAnalyzerResult;

class XVT_EXPORTS IntensityAnalyzer 
{
public:
    auto Inspect(cv::Mat const& inImg) const->IntensityAnalyzerResult;
    // Return true if load successfully
    auto Load(const std::wstring& settingPath)  &-> bool;

public:
    bool mEnable = true;
    VecRect mRoiList;
    bool mIsUse8Bit = true;
};

struct XVT_EXPORTS RoiInfo
{
    cv::Rect mSetRoi{};
    cv::Rect mUseRoi{};
    double mAvg=0.0;
    double mMin=0.0;
    double mMax=0.0;
    double mStd=0.0;

    auto GetCSVData(VecKeyValueStr& out, std::string prefix = "", bool isRecursive = true)const->void;
};

class XVT_EXPORTS IntensityAnalyzerResult : public InspectionResult
{
public:
    virtual void DrawResult(cv::Mat& img, cv::Point offSetPoint = cv::Point(), CVPen pen = CVPen()) const override;

    virtual auto GetCSVData(VecKeyValueStr& out, std::string prefix = "", bool isRecursive = true)const->void override;

    virtual auto GetResultStr() const->std::string override;

public:
    std::vector<RoiInfo> mRois;
};

}
