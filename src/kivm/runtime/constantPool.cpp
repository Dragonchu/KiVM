//
// Created by kiva on 2018/3/31.
//

#include <kivm/runtime/constantPool.h>
#include <kivm/oop/instanceKlass.h>
#include <kivm/oop/arrayOop.h>

namespace kivm {
RuntimeConstantPool::RuntimeConstantPool(InstanceKlass *instanceKlass)
    : _classLoader(instanceKlass->getClassLoader()),
      _rawPool(nullptr), _entryCount(0) {
}

namespace pools {
namespace impl {
typedef FieldID *(InstanceKlass::*FieldInfoGetterType)(const String &,
                                                       const String &,
                                                       const String &) const;

typedef Method *(InstanceKlass::*MethodGetterType)(const String &,
                                                   const String &) const;

FieldPoolEntry getField(RuntimeConstantPool *rt, cp_info **pool, int index, bool isStatic) {
  auto fieldRef = (CONSTANT_Fieldref_info *) pool[index];
  Klass *klass = rt->getClass(fieldRef->class_index);

  if (klass->getClassType() == ClassType::INSTANCE_CLASS) {
    auto instanceKlass = (InstanceKlass *) klass;
    const auto &nameAndType = rt->getNameAndType(fieldRef->name_and_type_index);

    FieldInfoGetterType fieldInfoGetter = isStatic
                                          ? &InstanceKlass::getStaticFieldInfo
                                          : &InstanceKlass::getInstanceFieldInfo;

    auto currentClass = instanceKlass;
    while (currentClass != nullptr) {
      auto found = (currentClass->*fieldInfoGetter)(currentClass->getName(),
                                                    *nameAndType->first,
                                                    *nameAndType->second);
      if (found != nullptr) {
        return found;
      }

      // field not found in current method
      // try superclass
      currentClass = currentClass->getSuperClass();
    }
  }
  SHOULD_NOT_REACH_HERE_M("Unsupported field & class type.");
  return nullptr;
}

MethodPoolEntry getMethod(InstanceKlass *instanceKlass, MethodGetterType getter,
                          const String &name, const String &desc) {
  auto currentClass = instanceKlass;
  while (currentClass != nullptr) {
    auto found = (currentClass->*getter)(name, desc);
    if (found != nullptr) {
      return found;
    }

    // method not found in current method
    // try superclass
    currentClass = currentClass->getSuperClass();
  }
  return nullptr;
}

MethodPoolEntry getInterfaceMethod(InstanceKlass *instanceKlass,
                                   const String &name, const String &desc) {
  auto currentClass = instanceKlass;
  Method *found = nullptr;

  while (currentClass != nullptr) {
    found = currentClass->getVirtualMethod(name, desc);

    // Trial 1: if this method was declared in itself
    if (found != nullptr) {
      return found;
    }

    // Trial 2: if this method was declared in its implemented interfaces
    for (auto &i : currentClass->getInterfaces()) {
      auto klass = i.second;
      found = klass->getVirtualMethod(name, desc);
      if (found != nullptr) {
        return found;
      }
    }

    // method not found in current method
    // try superclass
    currentClass = currentClass->getSuperClass();
  }
  return nullptr;
}
}

FieldPoolEntry
StaticFieldCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  return impl::getField(rt, pool, index, true);
}

FieldPoolEntry
InstanceFieldCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  return impl::getField(rt, pool, index, false);
}

ClassPoolEntey ClassCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  auto classInfo = (CONSTANT_Class_info *) pool[index];
  return BootstrapClassLoader::get()->loadClass(
      *rt->getUtf8(classInfo->name_index));
}

StringPoolEntry StringCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  auto classInfo = (CONSTANT_String_info *) pool[index];
  return java::lang::String::intern(
      *rt->getUtf8(classInfo->string_index));
}

MethodPoolEntry MethodCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  int tag = rt->getConstantTag(index);
  u2 classIndex = 0;
  u2 nameAndTypeIndex = 0;
  if (tag == CONSTANT_Methodref) {
    auto methodRef = (CONSTANT_Methodref_info *) pool[index];
    classIndex = methodRef->class_index;
    nameAndTypeIndex = methodRef->name_and_type_index;

  } else if (tag == CONSTANT_InterfaceMethodref) {
    auto interfaceMethodRef = (CONSTANT_InterfaceMethodref_info *) pool[index];
    classIndex = interfaceMethodRef->class_index;
    nameAndTypeIndex = interfaceMethodRef->name_and_type_index;

  } else {
    SHOULD_NOT_REACH_HERE_M("Unsupported method & class type.");
  }

  Klass *klass = rt->getClass(classIndex);
  const auto &nameAndType = rt->getNameAndType(nameAndTypeIndex);

  if (klass->getClassType() == ClassType::INSTANCE_CLASS) {
    auto instanceKlass = (InstanceKlass *) klass;

    if (tag == CONSTANT_Methodref) {
      // invokespecial, invokestatic and invokevirtual
      // Note: we should use getVirtualMethod() to locate virtual methods
      // but getThisClassMethod() has covered getVirtualMethod()
      // so there is no need to check again
      return impl::getMethod(instanceKlass, &InstanceKlass::getThisClassMethod,
                             *nameAndType->first, *nameAndType->second);

    } else {
      // invokeinterface
      return impl::getInterfaceMethod(instanceKlass,
                                      *nameAndType->first, *nameAndType->second);
    }

  } else if (klass->getClassType() == ClassType::OBJECT_ARRAY_CLASS
      || klass->getClassType() == ClassType::TYPE_ARRAY_CLASS) {
    auto arrayKlass = (ArrayKlass *) klass;
    return arrayKlass->getSuperClass()->getThisClassMethod(*nameAndType->first, *nameAndType->second);
  }

  return nullptr;
}

NameAndTypePoolEntry
NameAndTypeCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  auto nameAndType = (CONSTANT_NameAndType_info *) pool[index];
  auto name = rt->getUtf8(nameAndType->name_index);
  auto desc = rt->getUtf8(nameAndType->descriptor_index);
  return new std::pair<Utf8PoolEntry, Utf8PoolEntry>(name, desc);
}

InvokeDynamicPoolEntry
InvokeDynamicCreator::operator()(RuntimeConstantPool *rt, cp_info **pool, int index) {
  auto entry = new InvokeDynamicInfo;
  auto invokeInfo = (CONSTANT_InvokeDynamic_info *) pool[index];
  entry->methodIndex = invokeInfo->bootstrap_method_attr_index;
  entry->methodNameAndType = rt->getNameAndType(invokeInfo->name_and_type_index);
  return entry;
}
}
}

