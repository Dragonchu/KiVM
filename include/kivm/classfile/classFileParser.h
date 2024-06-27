//
// Created by kiva on 2018/2/25.
//

#pragma once

#include <kivm/classfile/classFile.h>
#include <kivm/classfile/classFileStream.h>

namespace kivm {
class ClassFileParser final {
 public:
  static ClassFile *alloc();

  static void dealloc(ClassFile *class_file);

 private:
  ClassFile *_classFile = nullptr;
  ClassFileStream _classFileStream;
  u1 *_content = nullptr;
  size_t _size;

  ClassFile *parse(const String &className);

  void parseConstantPool(ClassFile *classFile, const String &className);

  void parseInterfaces(ClassFile *classFile);

  void parseFields(ClassFile *classFile);

  void parseMethods(ClassFile *classFile);

  void parseAttributes(ClassFile *classFile);

 public:
  ClassFileParser(const String &filePath, u1 *buffer, size_t size);

  ~ClassFileParser();

  ClassFile *getParsedClassFile(const String &className);
};
}
