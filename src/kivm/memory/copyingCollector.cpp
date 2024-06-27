//
// Created by kiva on 2018/6/4.
//

#include <kivm/memory/copyingHeap.h>
#include <kivm/oop/instanceKlass.h>
#include <kivm/oop/instanceOop.h>
#include <kivm/oop/arrayOop.h>
#include <kivm/oop/mirrorOop.h>
#include <kivm/runtime/javaThread.h>
#include <sparsepp/spp.h>
#include <kivm/bytecode/execution.h>
#include <kivm/native/java_lang_Class.h>


// GC-Roots include:
// [*] 0. InstanceKlass::_staticFieldValues
// [*] 1. InstanceKlass::_javaMirror
// [*] 2. InstanceKlass::_javaLoader
// [*] 3. InstanceKlass::_runtimePool's Strings
// [*] 4. instanceOopDesc::_instanceFieldValues
// [*] 5. arrayOopDesc::_elements
// [*] 6. JavaThread::_javaThreadObject
// [*] 7. JavaThread::_exceptionOop
// [*] 8. JavaThread::_frames[0 ~ the last frame]::_locals
// [*] 9. JavaThread::_frames[0 ~ the last frame]::_stack

namespace kivm {
void CopyingHeap::copyObject(HeapRegion *newRegion, HashMap<oop, oop> &map,
                             oop &target) {
  // there is no need to copy null objects
  if (target == nullptr) {
    return;
  }

  // Object has been already copied to new region
  auto iter = map.find(target);
  if (iter != map.end()) {
    target = iter->second;
    return;
  }

  // Copy to new region
  auto newOop = target->copy();
  map.insert(std::make_pair(target, newOop));

  assert(target->getMarkOop() != newOop->getMarkOop());
  target = newOop;

  assert(target == newOop);
  assert(target->getMarkOop() == newOop->getMarkOop());
  assert(target->getClass() == newOop->getClass());

  // Copy object's data
  oopType type = target->getMarkOop()->getOopType();
  switch (type) {
    case oopType::INSTANCE_OOP: {
      auto instance = (instanceOop) target;

      // instance fields
      for (auto &item : instance->_instanceFieldValues) {
        copyObject(newRegion, map, item);
      }
      break;
    }

    case oopType::OBJECT_ARRAY_OOP:
    case oopType::TYPE_ARRAY_OOP: {
      auto array = (arrayOop) target;

      // array elements
      for (auto &item : array->_elements) {
        copyObject(newRegion, map, item);
      }
      break;
    }

    case oopType::PRIMITIVE_OOP:
      // everything is copied in oopDesc::copy(), so here do nothing
      break;

    default:SHOULD_NOT_REACH_HERE();
  }
}

void CopyingHeap::copyClass(HeapRegion *newRegion, HashMap<oop, oop> &map,
                            Klass *klass) {
  if (klass == nullptr) {
    return;
  }

  // java mirror
  oop javaMirror = klass->_javaMirror;
  copyObject(newRegion, map, javaMirror);
  klass->_javaMirror = (mirrorOop) javaMirror;

  switch (klass->getClassType()) {
    case ClassType::INSTANCE_CLASS: {
      // java loader
      auto instanceClass = (InstanceKlass *) klass;
      oop javaLoader = instanceClass->_javaLoader;
      copyObject(newRegion, map, javaLoader);
      instanceClass->_javaLoader = (mirrorOop) javaLoader;

      // static fields
      for (auto &item : instanceClass->_staticFieldValues) {
        copyObject(newRegion, map, item);
      }

      // runtime constant pool strings
      auto rt = instanceClass->getRuntimeConstantPool();
      for (int i = 1; i < rt->_entryCount; ++i) {
        if (rt->_rawPool[i] != nullptr
            && rt->getConstantTag(i) == CONSTANT_String) {
          oop stringOop = Resolver::instance(rt->_pool[i]);
          if (stringOop == nullptr) {
            continue;
          }
          copyObject(newRegion, map, stringOop);
          rt->_pool[i] = stringOop;
        }
      }
      break;
    }

    case ClassType::OBJECT_ARRAY_CLASS:
    case ClassType::TYPE_ARRAY_CLASS: {
      // java loader
      auto arrayClass = (ArrayKlass *) klass;
      oop javaLoader = arrayClass->_javaLoader;
      copyObject(newRegion, map, javaLoader);
      arrayClass->_javaLoader = (mirrorOop) javaLoader;
      break;
    }

    default:SHOULD_NOT_REACH_HERE();
  }
}

void CopyingHeap::copyThread(HeapRegion *newRegion, HashMap<oop, oop> &map,
                             JavaThread *thread) {
  D("[GCThread]: copying exception oop");
  oop exceptionOop = thread->_exceptionOop;
  copyObject(newRegion, map, exceptionOop);
  thread->_exceptionOop = (instanceOop) exceptionOop;

  D("[GCThread]: copying thread oop");
  oop javaThreadOop = thread->_javaThreadObject;
  copyObject(newRegion, map, javaThreadOop);
  thread->_javaThreadObject = (instanceOop) javaThreadOop;

  D("[GCThread]: copying thread arguments");
  for (auto &item : thread->_args) {
    copyObject(newRegion, map, item);
  }

  auto currentFrame = thread->_frames._current;
  while (currentFrame != nullptr) {
    D("[GCThread]: copying frame %p: locals", currentFrame);
    copySlotArray(newRegion, map, &currentFrame->_locals._array, currentFrame->_locals._array._size);
    D("[GCThread]: copying frame %p: stacks", currentFrame);
    copySlotArray(newRegion, map, &currentFrame->_stack._array, currentFrame->_stack._sp);
    currentFrame = currentFrame->getPrevious();
  }
}

void CopyingHeap::copySlotArray(HeapRegion *newRegion, HashMap<oop, oop> &map,
                                SlotArray *slotArray, int size) {
  for (int i = 0; i < size; ++i) {
    Slot *slot = slotArray->_elements + i;
    if (slot->isObject) {
      auto object = Resolver::javaOop(slot->ref);
      if (object != nullptr) {
        copyObject(newRegion, map, object);
        slot->ref = object;
      }
    }
  }
}

void CopyingHeap::doGarbageCollection() {
  HeapRegion *current = this->_currentRegion;
  HeapRegion *next = this->_nextRegion;

  size_t total = current->getSize();
  size_t beforeUsed = current->getUsed();

  // TODO: use a more elegant way to allocate new objects during gc
  // CopyingHeap::copy() need to use oopDesc::copy() which may cause allocation,
  // and there is no need to lock, because we have already stopped the world.
  // In further implementations, it is important to deprecate this ugly workaround
  // if we are implementing an concurrent garbage collector.
  this->_currentRegion = next;

  // the old->new oop map
  HashMap<oop, oop> map;

  D("[GCThread]: copying primitive types's java mirrors");
  for (auto &item : java::lang::Class::_primitiveTypeMirrors) {
    oop mirror = item.second;
    copyObject(next, map, mirror);
    item.second = (mirrorOop) mirror;
  }

  D("[GCThread]: copying intern string pool");
  auto internStringPool = java::lang::InternStringPool::getGlobal();
  for (auto &item : internStringPool->_pool) {
    oop stringOop = item.second;
    copyObject(next, map, stringOop);
    item.second = (instanceOop) stringOop;
  }

  D("[GCThread]: copying loaded classes");
  auto sd = SystemDictionary::get();
  for (const auto &loadedClass : sd->getLoadedClasses()) {
    auto klass = loadedClass.second;
    copyClass(next, map, klass);
  }

  D("[GCThread]: copying threads, locals and stacks");
  Threads::forEach([&](JavaThread *thread) {
    copyThread(next, map, thread);
    return false;
  });

  // Done, free all unreachable objects
  // and make preparations for the next routine of gc
  current->reset();
  memset(current->_regionStart, '\0', current->_regionSize);
  this->_nextRegion = current;

  size_t afterUsed = next->getUsed();
  D("[GCDetails]: [%zd -> %zd(%zd), oops: %zd]",
    beforeUsed, afterUsed, total, map.size());
}
}
