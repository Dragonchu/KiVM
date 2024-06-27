//
// Created by kiva on 2018/2/25.
//

#pragma once

#include <kivm/kivm.h>
#include <kivm/oop/oopfwd.h>
#include <kivm/oop/klass.h>
#include <shared/lock.h>
#include <shared/monitor.h>
#include <list>

namespace kivm {

class GCJavaObject {
 public:
  GCJavaObject() = default;

  virtual ~GCJavaObject() = default;

  static void *allocate(size_t size);

  static void deallocate(void *ptr);

  static void *operator new(size_t size, bool = true) noexcept;

  static void *operator new(size_t size, const std::nothrow_t &) noexcept = delete;

  static void *operator new[](size_t size, bool = true) throw();

  static void *operator new[](size_t size, const std::nothrow_t &) noexcept = delete;

  static void operator delete(void *ptr);

  static void operator delete[](void *ptr);
};

class markOopDesc final {
 private:
  oopType _type;
  Monitor _monitor;
  jint _hash;

 public:
  explicit markOopDesc(oopType type);

  oopType getOopType() const { return _type; }

  void monitorEnter() {
    _monitor.enter();
  }

  void monitorExit() {
    _monitor.leave();
  }

  inline void setHash(jint hash) {
    this->_hash = hash;
  }

  inline jint getHash() const {
    return this->_hash;
  }

  inline void wait() { _monitor.wait(); }

  inline void wait(jlong timeout) { _monitor.wait((jlong) timeout); }

  inline void notify() { _monitor.notify(); }

  inline void notifyAll() { _monitor.notifyAll(); }

  inline void forceUnlockWhenExceptionOccurred() { _monitor.forceUnlock(); }
};

class oopDesc : public GCJavaObject {
 private:
  markOop _mark = nullptr;
  Klass *_klass = nullptr;

 public:
  explicit oopDesc(Klass *klass, oopType type);

  ~oopDesc() override;

  markOop getMarkOop() const { return _mark; }

  Klass *getClass() const { return _klass; }

  virtual oop copy() = 0;
};
}
