//
// Created by kiva on 2018/3/28.
//
#include <kivm/native/java_lang_Class.h>
#include <kivm/native/java_lang_String.h>
#include <kivm/classpath/classLoader.h>
#include <kivm/oop/klass.h>
#include <kivm/oop/arrayKlass.h>
#include <kivm/oop/mirrorKlass.h>
#include <kivm/oop/mirrorOop.h>
#include <kivm/oop/arrayOop.h>
#include <kivm/bytecode/execution.h>
#include <sstream>
#include <kivm/bytecode/javaCall.h>

namespace kivm {
namespace java {
namespace lang {
HashMap<kivm::String, mirrorOop> Class::_primitiveTypeMirrors;

std::queue<kivm::String> &Class::getDelayedMirrors() {
  static std::queue<kivm::String> mirrors;
  return mirrors;
}

std::queue<kivm::Klass *> &Class::getDelayedArrayClassMirrors() {
  static std::queue<kivm::Klass *> mirrors;
  return mirrors;
}

HashMap<kivm::String, mirrorOop> &Class::getPrimitiveTypeMirrors() {
  return _primitiveTypeMirrors;
}

Class::ClassMirrorState &Class::getMirrorState() {
  static ClassMirrorState state = NOT_FIXED;
  return state;
}

mirrorOop Class::findPrimitiveTypeMirror(const kivm::String &signature) {
  auto &mirrors = getPrimitiveTypeMirrors();
  auto it = mirrors.find(signature);
  if (it != mirrors.end()) {
    return it->second;
  }
  return nullptr;
}

void Class::initialize() {
  D("native: Class::initialize()");
  auto &m = getDelayedMirrors();
  m.push(L"I");
  m.push(L"Z");
  m.push(L"B");
  m.push(L"C");
  m.push(L"S");
  m.push(L"F");
  m.push(L"J");
  m.push(L"D");
  m.push(L"V");
  m.push(L"[I");
  m.push(L"[Z");
  m.push(L"[B");
  m.push(L"[C");
  m.push(L"[S");
  m.push(L"[F");
  m.push(L"[J");
  m.push(L"[D");
  getMirrorState() = ClassMirrorState::NOT_FIXED;
}

void Class::mirrorCoreAndDelayedClasses() {
  D("native: Class::mirrorCoreAndDelayedClasses()");
  assert(getMirrorState() != ClassMirrorState::FIXED);
  assert(SystemDictionary::get()->find(L"java/lang/Class") != nullptr);

  auto &M = getDelayedMirrors();
  long size = M.size();
  for (int i = 0; i < size; ++i) {
    auto name = std::move(M.front());
    M.pop();

    bool isPrimitiveArray = false;
    wchar_t primitiveType = name[0];
    if (name.size() == 2 && name[0] == L'[') {
      isPrimitiveArray = true;
      primitiveType = name[1];
    }

    if (name.size() > 2) {
      Klass *klass = SystemDictionary::get()->find(name);
      assert(klass != nullptr);
      assert(klass->getJavaMirror() == nullptr);

      mirrorOop mirror = mirrorKlass::newMirror(klass, nullptr);
      klass->setJavaMirror(mirror);
      D("native: Class::mirrorCoreAndDelayedClasses: mirroring %S",
        (klass->getName()).c_str());

    } else {
      if (primitiveType == L'V' && isPrimitiveArray) {
        PANIC("Cannot make mirror for void array.");
      }

      switch (primitiveType) {
        case L'I':
        case L'Z':
        case L'B':
        case L'C':
        case L'S':
        case L'F':
        case L'J':
        case L'D':
        case L'V': {
          mirrorOop mirror = mirrorKlass::newMirror(nullptr, nullptr);
          getPrimitiveTypeMirrors().insert(std::make_pair(name, mirror));
          D("native: Class::mirrorCoreAndDelayedClasses: mirroring primitive type %S",
            (name).c_str());

          if (isPrimitiveArray) {
            // Only arrays need them.
            auto *array_klass = BootstrapClassLoader::get()->loadClass(name);
            mirror->setTarget(array_klass);
            array_klass->setJavaMirror(mirror);
          } else {
            // use primitiveTypeToValueTypeNoWrap() instead of primitiveTypeToValueType()
            // because in Java, booleans, chars, shorts and bytes are not ints
            mirror->setPrimitiveType(primitiveTypeToValueTypeNoWrap(primitiveType));
          }

          break;
        }
        default:PANIC("Cannot make mirror for primitive type %c",
                      primitiveType);
      }
    }
  }
  getMirrorState() = ClassMirrorState::FIXED;
  D("native: Class::mirrorCoreAndDelayedClasses(): done");
}

void Class::mirrorDelayedArrayClasses() {
  D("native: Class::mirrorDelayedArrayClasses()");
  auto &M = getDelayedArrayClassMirrors();
  long size = M.size();
  for (int i = 0; i < size; ++i) {
    auto klass = (ArrayKlass *) M.front();
    M.pop();

    Class::createMirror(klass, klass->getJavaLoader());
  }
  D("native: Class::mirrorDelayedArrayClasses(): done");
}

void Class::createMirror(Klass *klass, mirrorOop javaLoader) {
  if (getMirrorState() != ClassMirrorState::FIXED) {
    if (klass->getClassType() == ClassType::INSTANCE_CLASS) {
      getDelayedMirrors().push(klass->getName());

    } else if (klass->getClassType() == ClassType::OBJECT_ARRAY_CLASS) {
      getDelayedArrayClassMirrors().push(klass);

    } else {
      PANIC("Class::createMirror(): use of illegal mirroring policy: "
            "only instance classes are acceptable "
            "before mirrorCoreAndDelayedClasses()");
    }
    return;
  }

  if (klass->getClassType() == ClassType::INSTANCE_CLASS) {
    klass->setJavaMirror(mirrorKlass::newMirror(klass, javaLoader));

  } else if (klass->getClassType() == ClassType::TYPE_ARRAY_CLASS) {
    mirrorOop mirror = findPrimitiveTypeMirror(klass->getName());
    if (mirror == nullptr) {
      mirror = mirrorKlass::newMirror(klass, nullptr);
      getPrimitiveTypeMirrors().insert(std::make_pair(klass->getName(), mirror));
    }
    klass->setJavaMirror(mirror);

  } else if (klass->getClassType() == ClassType::OBJECT_ARRAY_CLASS) {
    klass->setJavaMirror(mirrorKlass::newMirror(klass, nullptr));

  } else {
    PANIC("Class::createMirror(): use of illegal mirroring policy: "
          "unknown class type");
  }
}
}
}
}

using namespace kivm;

JAVA_NATIVE void Java_java_lang_Class_registerNatives(JNIEnv *env, jclass mirror) {
  D("java/lang/Class.registerNatives()V");
}

JAVA_NATIVE jobject Java_java_lang_Class_getPrimitiveClass(JNIEnv *env, jclass mirror,
                                                           jstring className) {
  auto stringInstance = Resolver::instance(className);
  if (stringInstance == nullptr) {
    auto thread = Threads::currentThread();
    assert(thread != nullptr);
    thread->throwException(Global::_NullPointerException, false);
    return nullptr;
  }

  const String &signature = java::lang::String::toNativeString(stringInstance);
  if (signature == L"byte") {
    return java::lang::Class::findPrimitiveTypeMirror(L"B");

  } else if (signature == L"boolean") {
    return java::lang::Class::findPrimitiveTypeMirror(L"Z");

  } else if (signature == L"char") {
    return java::lang::Class::findPrimitiveTypeMirror(L"C");

  } else if (signature == L"short") {
    return java::lang::Class::findPrimitiveTypeMirror(L"S");

  } else if (signature == L"int") {
    return java::lang::Class::findPrimitiveTypeMirror(L"I");

  } else if (signature == L"float") {
    return java::lang::Class::findPrimitiveTypeMirror(L"F");

  } else if (signature == L"long") {
    return java::lang::Class::findPrimitiveTypeMirror(L"J");

  } else if (signature == L"double") {
    return java::lang::Class::findPrimitiveTypeMirror(L"D");

  } else if (signature == L"void") {
    return java::lang::Class::findPrimitiveTypeMirror(L"V");
  }

  PANIC("Class.getPrimitiveClass(String): unknown primitive type: %S",
        (signature).c_str());
}

JAVA_NATIVE jboolean Java_java_lang_Class_desiredAssertionStatus0(JNIEnv *env, jclass mirror) {
  return JNI_FALSE;
}

JAVA_NATIVE jobjectArray Java_java_lang_Class_getDeclaredFields0(JNIEnv *env,
                                                                 jobject mirror,
                                                                 jboolean publicOnly) {
  auto arrayClass = (ObjectArrayKlass *) BootstrapClassLoader::get()
      ->loadClass(L"[Ljava/lang/reflect/Field;");

  std::vector<instanceOop> fields;
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget->getClassType() != ClassType::INSTANCE_CLASS) {
    PANIC("native: attempt to get fields of non-instance oops");
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;

  // TODO: consider refactoring
  for (const auto &e : instanceClass->getStaticFields()) {
    FieldID *fieldId = e.second;
    if (publicOnly && !fieldId->_field->isPublic()) {
      continue;
    }
    fields.push_back(newJavaFieldObject(fieldId));
  }

  for (const auto &e : instanceClass->getInstanceFields()) {
    FieldID *fieldId = e.second;
    if (publicOnly && !fieldId->_field->isPublic()) {
      continue;
    }
    fields.push_back(newJavaFieldObject(fieldId));
  }

  auto fieldOopArray = arrayClass->newInstance((int) fields.size());
  for (int i = 0; i < fields.size(); ++i) {
    fieldOopArray->setElementAt(i, fields[i]);
  }
  return fieldOopArray;
}

JAVA_NATIVE jobjectArray Java_java_lang_Class_getDeclaredMethods0(JNIEnv *env,
                                                                  jobject mirror,
                                                                  jboolean publicOnly) {
  auto arrayClass = (ObjectArrayKlass *) BootstrapClassLoader::get()
      ->loadClass(L"[Ljava/lang/reflect/Method;");

  std::vector<instanceOop> methods;
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget->getClassType() != ClassType::INSTANCE_CLASS) {
    PANIC("native: attempt to get fields of non-instance oops");
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;

  for (const auto &e : instanceClass->getDeclaredMethods()) {
    methods.push_back(newJavaMethodObject(e.second));
  }

  auto methodOopArray = arrayClass->newInstance((int) methods.size());
  for (int i = 0; i < methods.size(); ++i) {
    methodOopArray->setElementAt(i, methods[i]);
  }
  return methodOopArray;
}

JAVA_NATIVE jobjectArray Java_java_lang_Class_getDeclaredConstructors0(JNIEnv *env,
                                                                       jobject mirror,
                                                                       jboolean publicOnly) {
  auto arrayClass = (ObjectArrayKlass *) BootstrapClassLoader::get()
      ->loadClass(L"[Ljava/lang/reflect/Constructor;");

  std::vector<instanceOop> ctors;
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget->getClassType() != ClassType::INSTANCE_CLASS) {
    PANIC("native: attempt to get fields of non-instance oops");
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;

  for (const auto &e : instanceClass->getDeclaredMethods()) {
    auto methodId = e.second;
    if (methodId->_method->getName() == L"<init>") {
      ctors.push_back(newJavaConstructorObject(e.second));
    }
  }

  auto ctorOopArray = arrayClass->newInstance((int) ctors.size());
  for (int i = 0; i < ctors.size(); ++i) {
    ctorOopArray->setElementAt(i, ctors[i]);
  }
  return ctorOopArray;
}

JAVA_NATIVE jstring Java_java_lang_Class_getName0(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget == nullptr) {
    return java::lang::String::intern(valueTypeToPrimitiveTypeName(
        classMirror->getPrimitiveType()));
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;
  const auto &nativeClassName = instanceClass->getName();
  const auto &javaClassName = strings::replaceAll(nativeClassName, Global::SLASH, Global::DOT);
  return java::lang::String::intern(javaClassName);
}

JAVA_NATIVE jstring Java_java_lang_Class_getSuperclass(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget == nullptr || mirrorTarget == Global::_Object) {
    return nullptr;
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;
  return instanceClass->getSuperClass()->getJavaMirror();
}

JAVA_NATIVE jboolean Java_java_lang_Class_isPrimitive(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  return classMirror->getTarget() == nullptr ? JNI_TRUE : JNI_FALSE;
}

JAVA_NATIVE jboolean Java_java_lang_Class_isAssignableFrom(JNIEnv *env,
                                                           jobject mirrorL,
                                                           jobject mirrorR) {
  auto lhs = Resolver::mirror(mirrorL);
  auto rhs = Resolver::mirror(mirrorR);

  auto lhsKlass = lhs->getTarget();
  auto rhsKlass = rhs->getTarget();

  if (lhsKlass == nullptr && rhsKlass == nullptr) {
    return JBOOLEAN(lhs == rhs);
  }

  return JBOOLEAN(Execution::instanceOf(rhsKlass, lhsKlass));
}

JAVA_NATIVE jclass Java_java_lang_Class_forName0(JNIEnv *env, jclass mirror,
                                                 jstring javaName,
                                                 jboolean initialize,
                                                 jobject javaClassLoader,
                                                 jobject callerMirror) {
  auto thread = Threads::currentThread();
  assert(thread != nullptr);

  if (javaClassLoader != nullptr) {
    // TODO: support app class loader
    PANIC("More work to do");
  }

  auto nameOop = Resolver::instance(javaName);
  if (nameOop == nullptr) {
    thread->throwException(Global::_NullPointerException,
                           L"name cannot be null");
    return nullptr;
  }

  const String &className = java::lang::String::toNativeString(nameOop);
  const String &fixedName = strings::replaceAll(className, Global::DOT, Global::SLASH);

  D("Class.forName0(): %S", (fixedName).c_str());

  auto klass = BootstrapClassLoader::get()->loadClass(fixedName);
  if (klass == nullptr || klass->getClassType() != ClassType::INSTANCE_CLASS) {
    thread->throwException(Global::_ClassNotFoundException, className, false);
    return nullptr;
  }

  if (initialize) {
    if (!Execution::initializeClass(thread, (InstanceKlass *) klass)) {
      return nullptr;
    }
  }

  return klass->getJavaMirror();
}

JAVA_NATIVE jboolean Java_java_lang_Class_isInterface(JNIEnv *env, jobject mirror) {
  auto mirrorOop = Resolver::mirror(mirror);
  return JBOOLEAN(mirrorOop != nullptr
                      && mirrorOop->getTarget() != nullptr
                      && mirrorOop->getTarget()->isInterface());
}

JAVA_NATIVE jint Java_java_lang_Class_getModifiers(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget == nullptr) {
    return ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC;
  }

  auto instanceClass = (InstanceKlass *) mirrorTarget;
  return instanceClass->getAccessFlag();
}

JAVA_NATIVE jboolean Java_java_lang_Class_isArray(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();

  if (mirrorTarget == nullptr) {
    return JNI_FALSE;
  }

  auto classType = mirrorTarget->getClassType();
  return JBOOLEAN(classType == ClassType::TYPE_ARRAY_CLASS
                      || classType == ClassType::OBJECT_ARRAY_CLASS);
}

JAVA_NATIVE jclass Java_java_lang_Class_getComponentType(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  auto mirrorTarget = classMirror->getTarget();
  if (mirrorTarget == nullptr) {
    SHOULD_NOT_REACH_HERE();
  }

  switch (mirrorTarget->getClassType()) {
    case ClassType::OBJECT_ARRAY_CLASS: {
      auto comp = ((ObjectArrayKlass *) mirrorTarget)->getComponentType()->getJavaMirror();
      return comp;
    }
    case ClassType::TYPE_ARRAY_CLASS: {
      ValueType valueType = ((TypeArrayKlass *) mirrorTarget)->getComponentType();
      return java::lang::Class::findPrimitiveTypeMirror(valueTypeToPrimitiveTypeName(valueType));
    }
    default:SHOULD_NOT_REACH_HERE();
  }
}

JAVA_NATIVE jobjectArray Java_java_lang_Class_getEnclosingMethod0(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  if (classMirror == nullptr || classMirror->getTarget() == nullptr
      || classMirror->getTarget()->getClassType() != ClassType::INSTANCE_CLASS) {
    return nullptr;
  }

  auto target = (InstanceKlass *) classMirror->getTarget();
  auto enclosing = target->getEnclosingMethodInfo();
  if (enclosing == nullptr) {
    return nullptr;
  }

  if (enclosing->class_index == 0) {
    SHOULD_NOT_REACH_HERE_M("enclosing->class_index should not be 0");
  }

  auto arrayClass = (ObjectArrayKlass *) BootstrapClassLoader::get()
      ->loadClass(L"[Ljava/lang/Object;");
  if (arrayClass == nullptr) {
    SHOULD_NOT_REACH_HERE();
  }

  auto array = arrayClass->newInstance(3);
  auto rt = target->getRuntimeConstantPool();

  auto klass = rt->getClass(enclosing->class_index);
  if (klass == nullptr) {
    SHOULD_NOT_REACH_HERE();
  }
  array->setElementAt(0, klass->getJavaMirror());

  if (enclosing->method_index != 0) {
    auto nameAndType = rt->getNameAndType(enclosing->method_index);
    auto name = nameAndType->first;
    auto desc = nameAndType->second;
    array->setElementAt(1, java::lang::String::intern(*name));
    array->setElementAt(2, java::lang::String::intern(*desc));
  }
  return array;
}

JAVA_NATIVE jclass Java_java_lang_Class_getDeclaringClass0(JNIEnv *env, jobject mirror) {
  auto classMirror = Resolver::mirror(mirror);
  if (classMirror == nullptr || classMirror->getTarget() == nullptr
      || classMirror->getTarget()->getClassType() != ClassType::INSTANCE_CLASS) {
    return nullptr;
  }

  auto target = (InstanceKlass *) classMirror->getTarget();
  auto inner = target->getInnerClassInfo();
  if (inner == nullptr) {
    return nullptr;
  }

  auto rt = target->getRuntimeConstantPool();
  for (int i = 0; i < inner->number_of_classes; i++) {
    int innerClassInfo = inner->classes[i].inner_class_info_index;
    if (innerClassInfo == 0) {
      continue;
    }

    auto checkInnerClass = rt->getClass(innerClassInfo);
    if (checkInnerClass == nullptr) {
      SHOULD_NOT_REACH_HERE();
    }

    // Find this inner class
    if (checkInnerClass == target) {
      int outerClassInfoIndex = inner->classes[i].outer_class_info_index;

      if (outerClassInfoIndex == 0) {
        return nullptr;

      } else {
        auto outerKlass = rt->getClass(outerClassInfoIndex);
        if (outerKlass == nullptr) {
          SHOULD_NOT_REACH_HERE();
        }

        return outerKlass->getJavaMirror();
      }
    }
  }

  return nullptr;
}
