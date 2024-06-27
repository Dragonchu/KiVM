//
// Created by kiva on 2018/2/25.
//
#pragma once

#include <compileTimeConfig.h>

#if defined(KIVM_THREADED) && !defined(KIVM_DEBUG)

#include <kivm/kivm.h>
#include <kivm/oop/oop.h>
#include <kivm/runtime/javaThread.h>
#include <kivm/bytecode/bytecodes.h>
#include <kivm/bytecode/codeBlob.h>

namespace kivm {
class Frame;

class Method;

class InstanceKlass;

class ThreadedInterpreter final {
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

 private:
  static void *_jumpTable[OPC_NUM_OPCODES];

  static oop threaded(JavaThread *thread, void **jump,
                      Frame *currentFrame,
                      Method *currentMethod,
                      InstanceKlass *currentClass,
                      const CodeBlob &codeBlob,
                      u4 &pc,
                      Stack &stack,
                      Locals &locals);
};
}
#endif
