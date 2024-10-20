#pragma once
#include "xvtCV/xvtDefine.h"
#include <string>

#define XVT_BATTERY_VERSION_MAJOR 0
#define XVT_BATTERY_VERSION_MINOR 8
#define XVT_BATTERY_VERSION_PATCH 4
#define XVT_BATTERY_VERSION_NUMBER 0
#define XVT_BATTERY_VERSION_COMIT af3c799
#define XVT_BATTERY_VERSION_DIRTY 
#define XVT_BATTERY_BRANCH master

namespace xvt {
namespace battery {

XVT_EXPORTS
auto GetVersionMajor()->std::string;

XVT_EXPORTS
auto GetVersionMinor()->std::string;

XVT_EXPORTS
auto GetVersionPatch()->std::string;

XVT_EXPORTS
auto GetVersionNumber()->std::string;

XVT_EXPORTS
auto GetVersionHash()->std::string;

XVT_EXPORTS
auto GetVersionDirty()->std::string;

XVT_EXPORTS
auto GetVersionPreID()->std::string;

XVT_EXPORTS
auto GetVersionInfo()->std::string;

}
}
