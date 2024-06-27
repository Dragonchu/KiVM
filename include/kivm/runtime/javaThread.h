//
// Created by kiva on 2018/6/4.
//
#pragma once

#include <kivm/runtime/abstractThread.h>
#include <kivm/oop/instanceOop.h>
#include <kivm/runtime/stack.h>
#include <kivm/runtime/frame.h>
#include <list>
#include <functional>

namespace kivm {
// The Java app thread
class JavaThread : public AbstractThread {
  friend class KiVM;

  friend class Threads;

  friend class ByteCodeInterpreter;

  friend class ThreadedInterpreter;

  friend class ScratchInterpreter;

  friend class JavaCall;

  friend class JavaCall;

  friend class CopyingHeap;

  friend class FrameWalker;

 protected:
  FrameList _frames;
  std::list<oop> _args;
  u4 _pc;
  bool _inSafepoint;

  instanceOop _javaThreadObject = nullptr;
  instanceOop _exceptionOop = nullptr;

  // note: this is not the current method
  // use getCurrentMethod() instead
  Method *_method = nullptr;

 protected:
  void run() override;

  int tryHandleException(instanceOop exceptionOop);

  void setJavaThreadObject(instanceOop javaThread) {
    this->_javaThreadObject = javaThread;
  }

 public:
  JavaThread(Method *method, const std::list<oop> &args);

  void start(instanceOop javaThread);

  void start() override;

  void onDestroy() override;

  void throwException(InstanceKlass *exceptionClass, bool rethrow);

  void throwException(InstanceKlass *exceptionClass, const String &message, bool rethrow);

  void throwException(instanceOop exception, bool rethrow);

  void enterSafepoint();

  void enterSafepointIfNeeded();

  inline u4 getPc() const {
    return _pc;
  }

  inline bool isInSafepoint() const {
    return _inSafepoint;
  }

  inline instanceOop getJavaThreadObject() const {
    return _javaThreadObject;
  }

  inline Frame *getCurrentFrame() {
    return _frames.getCurrentFrame();
  }

  inline Method *getCurrentMethod() const {
    if (_frames.getSize() == 0) {
      return nullptr;
    }
    return _frames.getCurrentFrame()->getMethod();
  }

  inline bool isExceptionOccurred() const {
    return _exceptionOop != nullptr;
  }

  inline instanceOop getException() const {
    return _exceptionOop;
  }
};

// The Java main thread
// implemented in src/kivm/runtime/init.cpp
class JavaMainThread : public JavaThread {
 private:
  bool _classFromStream;
  u1 *_mainClassBytes;
  size_t _mainClassSize;
  String _mainClassName;
  std::vector<String> _arguments;

 protected:
  void run() override;

  void onThreadLaunched() override;

 public:
  JavaMainThread(const String &mainClassName, const std::vector<String> &arguments);

  JavaMainThread(u1 *mainClassBytes, size_t classSize, const std::vector<String> &arguments);

  inline void start() override {
    JavaThread::start(nullptr);
  }
};

class Threads final {
  friend class GCThread;

 private:
  static int &getRunningJavaThreadCount() {
    static int appThreadCount;
    return appThreadCount;
  }

  static std::vector<JavaThread *> &getJavaThreadList() {
    static std::vector<JavaThread *> appThreads;
    return appThreads;
  }

  static void initializeVMStructs(BootstrapClassLoader *cl, JavaMainThread *thread);

  static void hackJavaClasses(BootstrapClassLoader *cl, JavaMainThread *thread);

 public:
  static void initializeJVM(JavaMainThread *thread);

  static JavaThread *currentThread();

  static JavaThread *searchNativeThread(instanceOop threadObject);

  static void forEach(const std::function<bool(JavaThread *)> &callback) {
    LockGuard lockGuard(appThreadLock());
    for (auto it : getJavaThreadList()) {
      if (callback(it)) {
        break;
      }
    }
  }

  static Lock &appThreadLock() {
    static Lock lock;
    return lock;
  }

  static Lock &threadStateChangeLock() {
    static Lock lock;
    return lock;
  }

  static inline void addJavaThread(JavaThread *javaThread) {
    LockGuard lockGuard(appThreadLock());
    D("Adding thread: %p", javaThread);
    EXPLORE("Adding thread: %p", javaThread);
    getJavaThreadList().push_back(javaThread);
    ++Threads::getRunningJavaThreadCount();
  }

  static inline int getRunningJavaThreadCountLocked() {
    LockGuard lockGuard(appThreadLock());
    int threads = Threads::getRunningJavaThreadCount();
    return threads;
  }

  static inline void notifyJavaThreadDeadLocked(JavaThread *javaThread) {
    LockGuard lockGuard(appThreadLock());
    --Threads::getRunningJavaThreadCount();
  }

  static inline void setThreadStateLocked(JavaThread *javaThread, ThreadState newState) {
    LockGuard lockGuard(Threads::threadStateChangeLock());
    javaThread->setThreadState(newState);
  }
};
}
