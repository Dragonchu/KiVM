//
// Created by kiva on 2018/2/25.
//

#pragma once

#include <kivm/kivm.h>
#include <kivm/classfile/constantPool.h>
#include <kivm/classfile/classFile.h>

/**
 * Ugly but useful
 */
#define READ_U2(v, p)  v = ((p)[0]<<8)|(p)[1];
#define READ_U4(v, p)  v = ((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3];
#define READ_U8(v, p)  v = ((u8)(p)[0]<<56)|((u8)(p)[1]<<48)|((u8)(p)[2]<<40) \
                            |((u8)(p)[3]<<32)|((u8)(p)[4]<<24)|((u8)(p)[5]<<16) \
                            |((u8)(p)[6]<<8)|(u8)(p)[7];

namespace kivm {
/**
 *  Input stream for reading .class file
 *  The entire input stream is present in a buffer allocated by the caller.
 *  The caller is responsible for deallocating the buffer.
 */
class ClassFileStream final {
 private:
  u1 *_bufferStart = nullptr; // Buffer bottom
  u1 *_bufferEnd = nullptr;   // Buffer top (one past last element)
  u1 *_current = nullptr;      // Current buffer position
  String _source;    // Source of stream (directory name, ZIP/JAR archive name)
  bool _needVerify{};  // True if verification is on for the class file

  void guaranteeMore(int size) {
    auto remaining = (size_t) (_bufferEnd - _current);
    auto usize = (unsigned int) size;
    if (usize > remaining) {
      PANIC("Unexpected EOF");
    }
  }

 public:
  ClassFileStream() = default;

  void init(u1 *buffer, size_t length);

  // Buffer access
  u1 *getBufferStart() const { return _bufferStart; }

  size_t getLength() const { return static_cast<size_t>(_bufferEnd - _bufferStart); }

  u1 *getCurrent() const { return _current; }

  void setCurrent(u1 *pos) { _current = pos; }

  const String &getSource() const { return _source; }

  void setNeedVerify(bool flag) { _needVerify = flag; }

  void setSource(const String &source) { _source = source; }

  // Peek u1
  u1 peek1() const {
    return *_current;
  }

  u2 peek2() const {
    u2 res;
    READ_U2(res, _current);
    return res;
  }

  // Read u1 from stream
  u1 get1();

  u1 get1Fast() {
    return *_current++;
  }

  // Read u2 from stream
  u2 get2();

  u2 get2Fast() {
    u2 res;
    READ_U2(res, _current);
    _current += 2;
    return res;
  }

  // Read u4 from stream
  u4 get4();

  u4 get4Fast() {
    u4 res;
    READ_U4(res, _current);
    _current += 4;
    return res;
  }

  // Read u8 from stream
  u8 get8();

  u8 get8Fast() {
    u8 res;
    READ_U8(res, _current);
    _current += 8;
    return res;
  }

  // Copy `count` u1 bytes from current position to `to`
  void getBytes(u1 *to, int count);

  // Get direct pointer into stream at current position.
  // Returns NULL if length elements are not remaining. The caller is
  // responsible for calling skip below if buffer contents is used.
  u1 *asU1Buffer() {
    return _current;
  }

  u2 *asU2Buffer() {
    return (u2 *) _current;
  }

  // Skip length u1 or u2 elements from stream
  void skip1(int length);

  void skip1Fast(int length) {
    _current += length;
  }

  void skip2(int length);

  void skip2Fast(int length) {
    _current += 2 * length;
  }

  void skip4(int length);

  void skip4Fast(int length) {
    _current += 4 * length;
  }

  // Tells whether eos is reached
  bool isEnded() const { return _current == _bufferEnd; }

  // Yeah...
  ClassFileStream &operator>>(CONSTANT_Utf8_info &info);

  ClassFileStream &operator>>(CONSTANT_Class_info &info);

  ClassFileStream &operator>>(CONSTANT_Double_info &info);

  ClassFileStream &operator>>(CONSTANT_Float_info &info);

  ClassFileStream &operator>>(CONSTANT_Long_info &info);

  ClassFileStream &operator>>(CONSTANT_Integer_info &info);

  ClassFileStream &operator>>(CONSTANT_String_info &info);

  ClassFileStream &operator>>(CONSTANT_Fieldref_info &info);

  ClassFileStream &operator>>(CONSTANT_Methodref_info &info);

  ClassFileStream &operator>>(CONSTANT_InterfaceMethodref_info &info);

  ClassFileStream &operator>>(CONSTANT_MethodHandle_info &info);

  ClassFileStream &operator>>(CONSTANT_MethodType_info &info);

  ClassFileStream &operator>>(CONSTANT_NameAndType_info &info);

  ClassFileStream &operator>>(CONSTANT_InvokeDynamic_info &info);
};
}
