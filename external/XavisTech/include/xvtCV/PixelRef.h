#pragma once
#include "xvtCV/xvtDefine.h"
#include <cassert>
#include <functional>

namespace xvt {

constexpr float Pixel2MM(float px, float pixelSize)
{
    return px * pixelSize;
}

constexpr float MM2Pixel(float mm, float pixelSize)
{
    assert(pixelSize != 0);
    return mm / pixelSize;
}

//Class that support reference to pixel value
class XVT_EXPORTS PixelRef
{
public:
    explicit PixelRef(float const& vl) :mPixelSize{ std::cref(vl) } {}
    explicit PixelRef(float&& vl) = delete;
    //Convert from pixel to milimeter
    auto ToMilimet(float const& px) const -> float { return px * mPixelSize.get(); }
    //Convert from milimeter to pixel
    auto ToPixel(float const& mm) const -> float { return mm / mPixelSize.get(); }
    auto GetPixelSize()const->float { return mPixelSize.get(); }
public:
    //Pixel size in mm.
    std::reference_wrapper<const float> mPixelSize;
};

}