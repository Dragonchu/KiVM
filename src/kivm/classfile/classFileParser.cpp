//
// Created by kiva on 2018/2/25.
//

#include <cstdio>
#include <kivm/classfile/classFileParser.h>
#include <cassert>
#include <kivm/memory/universe.h>

namespace kivm {
ClassFile *ClassFileParser::alloc() {
  return new ClassFile;
}

void ClassFileParser::dealloc(ClassFile *class_file) {
  AttributeParser::deallocAttributes(&class_file->attributes, class_file->attributes_count);
  for (int i = 1; i < class_file->constant_pool_count; ++i) {
    u1 tag = class_file->constant_pool[i]->tag;
    delete class_file->constant_pool[i];
    if (tag == CONSTANT_Long || tag == CONSTANT_Double) {
      ++i;
    }
  }
  delete[] class_file->methods;
  delete[] class_file->fields;
  delete[] class_file->interfaces;
  delete[] class_file->attributes;
  delete[] class_file->constant_pool;
  delete class_file;
}

ClassFileParser::ClassFileParser(const String &filePath, u1 *buffer, size_t size) {
  _content = buffer;
  _size = size;
  _classFileStream.setSource(filePath);
}

ClassFileParser::~ClassFileParser() = default;

ClassFile *ClassFileParser::getParsedClassFile(const String &className) {
  if (_classFile == nullptr) {
    _classFile = parse(className);
  }

  return _classFile;
}

ClassFile *ClassFileParser::parse(const String &className) {
  if (_content == nullptr) {
    return nullptr;
  }
  if (className.substr(0, 3) == L"com") {
    EXPLORE("Alloc classFile %S", className.c_str());
  }
  ClassFile *classFile = ClassFileParser::alloc();
  if (classFile == nullptr) {
    return nullptr;
  }

  _classFileStream.init(_content, _size);

  if (className.substr(0, 3) == L"com") {
    EXPLORE("Read magic %S", className.c_str());
  }
  classFile->magic = _classFileStream.get4();
  if (classFile->magic != 0xCAFEBABE) {
    ClassFileParser::dealloc(classFile);
    return nullptr;
  }

  if (className.substr(0, 3) == L"com") {
    EXPLORE("Read major and minor version %S", className.c_str());
  }
  classFile->major_version = _classFileStream.get2();
  classFile->minor_version = _classFileStream.get2();

  if (className.substr(0, 3) == L"com") {
    EXPLORE("Parsing Constant Pool %S", className.c_str());
  }
  parseConstantPool(classFile, className);
  if (className.substr(0, 3) == L"com") {
    EXPLORE("Constant Pool parsed %S", className.c_str());
  }

  classFile->access_flags = _classFileStream.get2();
  classFile->this_class = _classFileStream.get2();
  classFile->super_class = _classFileStream.get2();

  parseInterfaces(classFile);
  parseFields(classFile);
  parseMethods(classFile);
  parseAttributes(classFile);
  return classFile;
}

template<typename T>
static void readPoolEntry(cp_info **pool, int index, ClassFileStream &stream) {
  pool[index] = new T;
  stream >> *(T *) pool[index];
}

void ClassFileParser::parseConstantPool(ClassFile *classFile, const String &className) {
  u2 count = classFile->constant_pool_count = _classFileStream.get2();
  if (className.substr(0, 3) == L"com") {
    EXPLORE("Total %d constant in %S", count, className.c_str());
  }

  if (className.substr(0, 3) == L"com") {
    EXPLORE("Allocate constant_pool according count %S", className.c_str());
  }
  classFile->constant_pool = (cp_info **) Universe::allocCObject(sizeof(cp_info *) * count);
  cp_info **pool = classFile->constant_pool;

  // The constant_pool table is indexed
  // from 1 to count - 1
  for (int i = 1; i < count; ++i) {
    u1 tag = _classFileStream.peek1();
    switch (tag) {
      case CONSTANT_Utf8:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Utf8 (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Utf8_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_Integer:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Integer (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Integer_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_Float:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Float (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Float_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_Long:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Long (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Long_info>(pool, i, _classFileStream);
        ++i;
        break;
      case CONSTANT_Double:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Double (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Double_info>(pool, i, _classFileStream);
        ++i;
        break;
      case CONSTANT_Class:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Class (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Class_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_String:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_String (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_String_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_Fieldref:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Fieldref (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Fieldref_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_Methodref:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_Methodref (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_Methodref_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_InterfaceMethodref:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_InterfaceMethodref (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_InterfaceMethodref_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_NameAndType:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_NameAndType (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_NameAndType_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_MethodHandle:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_MethodHandle (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_MethodHandle_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_MethodType:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_MethodType (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_MethodType_info>(pool, i, _classFileStream);
        break;
      case CONSTANT_InvokeDynamic:
        if (className.substr(0, 3) == L"com") {
          EXPLORE("%d tag is CONSTANT_InvokeDynamic (%S)", i, className.c_str());
        }
        readPoolEntry<CONSTANT_InvokeDynamic_info>(pool, i, _classFileStream);
        break;
      default:assert(false);
        break;
    }
  }
}

void ClassFileParser::parseInterfaces(ClassFile *classFile) {
  u2 count = classFile->interfaces_count = _classFileStream.get2();
  classFile->interfaces = new u2[count];
  for (int i = 0; i < count; i++) {
    classFile->interfaces[i] = _classFileStream.get2();
  }
}

void ClassFileParser::parseFields(ClassFile *classFile) {
  u2 count = classFile->fields_count = _classFileStream.get2();
  classFile->fields = new field_info[count];
  for (int i = 0; i < count; ++i) {
    classFile->fields[i].init(_classFileStream, classFile->constant_pool);
  }
}

void ClassFileParser::parseMethods(ClassFile *classFile) {
  u2 count = classFile->methods_count = _classFileStream.get2();
  classFile->methods = new method_info[count];
  for (int i = 0; i < count; ++i) {
    classFile->methods[i].init(_classFileStream, classFile->constant_pool);
  }
}

void ClassFileParser::parseAttributes(ClassFile *classFile) {
  classFile->attributes_count = _classFileStream.get2();
  AttributeParser::readAttributes(&classFile->attributes, classFile->attributes_count,
                                  _classFileStream, classFile->constant_pool);
}
}