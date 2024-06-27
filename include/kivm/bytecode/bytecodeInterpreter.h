//
// Created by kiva on 2018/2/25.
//
#pragma once

#include <compileTimeConfig.h>
#include <kivm/kivm.h>
#include <kivm/oop/oop.h>
#include <kivm/runtime/javaThread.h>

namespace kivm {
class ByteCodeInterpreter final {
 public:
  /**
   * Run a thread method
   *
   * @param thread Java Thread that contains method
   * @return method return value(nullptr if void) o
   *         exception object(if thrown and not handled)
   */
  static oop interp(JavaThread *thread);

  static void initialize();
};
}
