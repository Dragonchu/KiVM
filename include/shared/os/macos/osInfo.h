//
// Created by kiva on 2018/6/10.
//

#pragma once

#include <compileTimeConfig.h>
#include <shared/string.h>

#if defined(KIVM_PLATFORM_APPLE)
namespace kivm {
class MacOSInformation final {
 public:
  static String getOSName();

  static String getOSVersion();

  static int getCpuNumbers();
};
}
#endif
