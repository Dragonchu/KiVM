//
// Created by kiva on 2018/2/27.
//
#pragma once

#include <kivm/kivm.h>
#include <kivm/oop/oopfwd.h>
#include <kivm/bytecode/codeBlob.h>
#include <kivm/classfile/attributeInfo.h>
#include <kivm/classfile/annotation.h>
#include <shared/hashMap.h>
#include <list>
#include <vector>

namespace kivm {
class InstanceKlass;

class method_info;

class cp_info;

class Code_attribute;

class Exceptions_attribute;

class JavaNativeMethod;

class Method {
 public:
  static bool isSame(const Method *lhs, const Method *rhs);

  static String makeIdentity(const Method *m);

 private:
  InstanceKlass *_klass = nullptr;
  String _name;
  String _descriptor;
  String _signature;
  u2 _accessFlag;

  /**
   * basic information about a method
   */
  CodeBlob _codeBlob;
  method_info *_methodInfo = nullptr;
  Exceptions_attribute *_exceptionAttr = nullptr;
  Code_attribute *_codeAttr = nullptr;

  /**
   * only available when this method is a native method
   */
  JavaNativeMethod *_nativePointer = nullptr;

  /**
   * flags related to descriptor parsing
   */
  bool _argumentValueTypesResolved;
  bool _argumentValueTypesNoWrapResolved;
  bool _argumentClassTypesResolved;
  bool _returnTypeNoWrapResolved;
  bool _returnTypeResolved;
  bool _checkedExceptionsResolved;
  bool _linked;

  /**
   * result of descriptor parsing
   */
  ValueType _returnType;
  ValueType _returnTypeNoWrap;
  std::vector<ValueType> _argumentValueTypes;
  std::vector<ValueType> _argumentValueTypesNoWrap;
  std::vector<mirrorOop> _argumentClassTypes;
  mirrorOop _returnClassType;

  /** this method is likely to throw these checked exceptions **/
  std::list<InstanceKlass *> _checkedExceptions;

  /** map<start-pc, line-number> **/
  HashMap<u2, u2> _lineNumberTable;

  /**
   * annotations
   */
  ParameterAnnotation *_runtimeVisibleAnnos;
  std::list<ParameterAnnotation *> _runtimeVisibleParameterAnnos;
  std::list<TypeAnnotation *> _runtimeVisibleTypeAnnos;

 private:
  void linkAttributes(cp_info **pool);

  void linkExceptionAttribute(cp_info **pool, Exceptions_attribute *attr);

  void linkCodeAttribute(cp_info **pool, Code_attribute *attr);

  bool isPcCorrect(u4 pc);

 public:
  Method(InstanceKlass *clazz, method_info *methodInfo);

  void linkMethod(cp_info **pool);

  /**
   * Used for Java calls.
   * Parse descriptor and map arguments to value types
   * short, boolean, bool and char will be wrapped to int
   * @return argument value type mapping parsed from descriptor
   */
  const std::vector<ValueType> &getArgumentValueTypes();

  /**
   * Parse descriptor and map result type to value types
   * short, boolean, bool and char will be wrapped to int
   * @return result value type parsed from descriptor
   */
  ValueType getReturnType();

  /**
   * Extract result class type from method descriptor
   * @return return type
   */
  mirrorOop getReturnClassType();

  /**
   * Extract argument list from method descriptor
   * @return argument list
   */
  const std::vector<mirrorOop> &getArgumentClassTypes();

  /**
   * Used for JNI calls.
   * Parse descriptor and map arguments to value types
   * short, boolean, bool and char will remains its original value type
   * @return argument value type mapping parsed from descriptor
   */
  const std::vector<ValueType> &getArgumentValueTypesNoWrap();

  /**
   * Parse descriptor and map result type to value types
   * short, boolean, bool and char will remains its original value type
   * @return result value type parsed from descriptor
   */
  ValueType getReturnTypeNoWrap();

  /**
   * Locate native method address
   * @return address of the native method
   */
  JavaNativeMethod *getNativeMethod();

  /**
   * Checked exceptions that this method may throw
   * @return checked exceptions
   */
  const std::list<InstanceKlass *> &getCheckedExceptions();

 public:
  int findExceptionHandler(u4 currentPc, InstanceKlass *exceptionClass);

  int getLineNumber(u4 pc);

  bool checkAnnotation(const String &annotationName);

  /*
   * Public getters and setters
   */

  InstanceKlass *getClass() const {
    return _klass;
  }

  const String &getName() const {
    return _name;
  }

  const String &getDescriptor() const {
    return _descriptor;
  }

  const String &getSignature() const {
    return _signature;
  }

  u2 getAccessFlag() const {
    return _accessFlag;
  }

  const CodeBlob &getCodeBlob() const {
    return _codeBlob;
  }

  bool isLinked() const {
    return _linked;
  }

  bool isPublic() const {
    return (getAccessFlag() & ACC_PUBLIC) == ACC_PUBLIC;
  }

  bool isPrivate() const {
    return (getAccessFlag() & ACC_PRIVATE) == ACC_PRIVATE;
  }

  bool isProtected() const {
    return (getAccessFlag() & ACC_PROTECTED) == ACC_PROTECTED;
  }

  bool isSynchronized() const {
    return (getAccessFlag() & ACC_SYNCHRONIZED) == ACC_SYNCHRONIZED;
  }

  bool isFinal() const {
    return (getAccessFlag() & ACC_FINAL) == ACC_FINAL;
  }

  bool isStatic() {
    return (getAccessFlag() & ACC_STATIC) == ACC_STATIC;
  }

  bool isAbstract() {
    return (getAccessFlag() & ACC_ABSTRACT) == ACC_ABSTRACT;
  }

  bool isNative() const {
    return (getAccessFlag() & ACC_NATIVE) == ACC_NATIVE;
  }

  int getMaxLocals() const {
    return _codeAttr != nullptr ? _codeAttr->max_locals : 0;
  }

  int getMaxStack() const {
    return _codeAttr != nullptr ? _codeAttr->max_stack : 0;
  }

  inline void hackAsNative() {
    this->_accessFlag |= ACC_NATIVE;
  }
};

/**
 * The implementation of Method Area
 */
class MethodPool {
 private:
  static std::list<Method *> &getEntriesInternal();

 public:
  static void add(Method *method);

  static const std::list<Method *> &getEntries();
};

std::vector<mirrorOop> parseArguments(const String &descriptor);

mirrorOop parseReturnType(const String &descriptor);

std::vector<ValueType> parseArgumentValueTypes(const String &descriptor);

ValueType parseReturnValueType(const String &descriptor);
}
