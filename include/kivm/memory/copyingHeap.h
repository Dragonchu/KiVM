//
// Created by kiva on 2018/4/19.
//
#pragma once

#include <kivm/oop/klass.h>
#include <kivm/memory/collectedHeap.h>
#include <kivm/memory/heapRegion.h>
#include <shared/hashMap.h>
#include <kivm/runtime/stack.h>

namespace kivm {
class CopyingHeap : public CollectedHeap {
 private:
  jbyte *_memoryStart = nullptr;
  size_t _totalSize;
  HeapRegion *_regions = nullptr;
  HeapRegion *_currentRegion = nullptr;
  HeapRegion *_nextRegion = nullptr;

 private:
  void initializeRegions();

  void copyObject(HeapRegion *newRegion, HashMap<oop, oop> &map, oop &target);

  void copyClass(HeapRegion *newRegion, HashMap<oop, oop> &map, Klass *klass);

  void copyThread(HeapRegion *newRegion, HashMap<oop, oop> &map, JavaThread *thread);

  void copySlotArray(HeapRegion *newRegion, HashMap<oop, oop> &map, SlotArray *slotArray, int size);

 public:
  CopyingHeap();

  ~CopyingHeap() override;

  void *allocate(size_t size) override;

  void initializeAll() override;

  void doGarbageCollection() override;

  inline void *getHeapStart() override {
    return _memoryStart;
  }

  inline void *getHeapEnd() override {
    return _memoryStart + getHeapSize();
  }

  inline size_t getHeapSize() override {
    return _totalSize;
  }

  inline bool isHeapObject(void *addr) override {
    return addr >= getHeapStart() && addr < getHeapEnd();
  }
};
}
