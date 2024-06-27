//
// Created by kiva on 2018/4/21.
//
#pragma once

#include <kivm/kivm.h>

namespace kivm {
struct HeapRegion final {
  size_t _regionSize{};
  jbyte *_regionStart = nullptr;
  jbyte *_current = nullptr;

  inline size_t getUsed() const {
    return _current - _regionStart;
  }

  inline size_t getSize() const {
    return _regionSize;
  }

  inline jbyte *getRegionEnd() const {
    return _regionStart + getSize();
  }

  inline bool shouldAllocate(size_t size) const {
    return (_current + size) < getRegionEnd();
  }

  inline void *allocate(size_t size) {
    // bump the pointer
    jbyte *m = _current;
    _current += size;
    return m;
  }

  inline void reset() {
    this->_current = this->_regionStart;
  }
};
}
