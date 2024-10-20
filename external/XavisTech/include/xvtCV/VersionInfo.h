#pragma once
#include "xvtCV/xvtDefine.h"
#include <string>

#define XVT_VERSION_MAJOR 0
#define XVT_VERSION_MINOR 2
#define XVT_VERSION_PATCH 1
#define XVT_VERSION_NUMBER 12
#define XVT_VERSION_COMIT 9728c34
#define XVT_VERSION_DIRTY 
#define XVT_BRANCH master

namespace xvt {
namespace xvtCV {

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
