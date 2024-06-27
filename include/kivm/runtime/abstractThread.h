//
// Created by kiva on 2018/3/23.
//
#pragma once

#include <kivm/kivm.h>
#include <kivm/runtime/threadState.h>
#include <thread>

namespace kivm {
class AbstractThread {
  friend class Threads;

 protected:
  std::thread *_nativeThread = nullptr;

  ThreadState _state;

 protected:
  virtual void run() = 0;

  virtual void onDestroy();

  virtual void onThreadLaunched();

 public:
  AbstractThread();

  virtual ~AbstractThread();

  virtual void start();

  long getNativeHandler() const;

  void setThreadName(const String &name);

  void setPriority(jint priority);

  inline ThreadState getThreadState() const {
    return _state;
  }

  inline void setThreadState(ThreadState threadState) {
    this->_state = threadState;
  }
};
}
