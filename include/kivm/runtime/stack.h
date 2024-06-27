//
// Created by kiva on 2018/3/21.
//
#pragma once

#include <kivm/kivm.h>
#include <kivm/oop/oopfwd.h>
#include <kivm/runtime/slot.h>

namespace kivm {
class SlotArray final {
  friend class CopyingHeap;

 protected:
  Slot *_elements = nullptr;
  int _size;

 public:
  explicit SlotArray(int size);

  inline void setInt(int position, jint i) {
    assert(position >= 0 && position < _size);
    _elements[position].i32 = i;
    _elements[position].isObject = false;
  }

  inline jint getInt(int position) {
    assert(position >= 0 && position < _size);
    return _elements[position].i32;
  }

  inline void setLong(int position, jlong j) {
    union _Cvt {
      jlong j;
      struct {
        jint h;
        jint l;
      };
    };
    _Cvt X{};
    X.j = j;
    setInt(position, X.l);
    setInt(position + 1, X.h);
  }

  inline jlong getLong(int position) {
    union _Cvt {
      jlong j;
      struct {
        jint h;
        jint l;
      };
    };
    _Cvt X{};
    X.l = getInt(position);
    X.h = getInt(position + 1);
    return X.j;
  }

  inline void setFloat(int position, jfloat f) {
    union _Cvt {
      jfloat f;
      jint i32;
    };
    _Cvt X{};
    X.f = f;
    setInt(position, X.i32);
  }

  inline jfloat getFloat(int position) {
    union _Cvt {
      jfloat f;
      jint i32;
    };
    _Cvt X{};
    X.i32 = getInt(position);
    return X.f;
  }

  inline void setDouble(int position, jdouble d) {
    union _Cvt {
      jdouble d;
      jlong j;
    };
    _Cvt X{};
    X.d = d;
    setLong(position, X.j);
  }

  inline jdouble getDouble(int position) {
    union _Cvt {
      jdouble d;
      jlong j;
    };
    _Cvt X{};
    X.j = getLong(position);
    return X.d;
  }

  inline void setReference(int position, jobject l) {
    assert(position >= 0 && position < _size);
    _elements[position].ref = l;
    _elements[position].isObject = true;
  }

  inline jobject getReference(int position) {
    assert(position >= 0 && position < _size);
    return _elements[position].ref;
  }

 public:
  virtual ~SlotArray();
};

class Stack final {
  friend class CopyingHeap;

 private:
  SlotArray _array;
  int _sp;

 public:
  explicit Stack(int size);

  ~Stack() = default;

  inline void pushInt(jint v) {
    _array.setInt(_sp++, v);
  }

  inline void pushFloat(jfloat v) {
    _array.setFloat(_sp++, v);
  }

  inline void pushReference(jobject v) {
    _array.setReference(_sp++, v);
  }

  inline void pushDouble(jdouble v) {
    _array.setDouble(_sp, v);
    _sp += 2;
  }

  inline void pushLong(jlong v) {
    _array.setLong(_sp, v);
    _sp += 2;
  }

  inline jint popInt() {
    return _array.getInt(--_sp);
  }

  inline jfloat popFloat() {
    return _array.getFloat(--_sp);
  }

  inline jobject popReference() {
    return _array.getReference(--_sp);
  }

  inline jdouble popDouble() {
    _sp -= 2;
    return _array.getDouble(_sp);
  }

  inline jlong popLong() {
    _sp -= 2;
    return _array.getLong(_sp);
  }

  inline void dropTop() {
    --_sp;
  }

  inline void clear() {
    this->_sp = 0;
  }
};

class Locals final {
  friend class CopyingHeap;

 private:
  SlotArray _array;

 public:
  explicit Locals(int size);

  ~Locals() = default;

  inline void setInt(int position, jint i) {
    _array.setInt(position, i);
  }

  inline jint getInt(int position) {
    return _array.getInt(position);
  }

  inline void setLong(int position, jlong j) {
    _array.setLong(position, j);
  }

  inline jlong getLong(int position) {
    return _array.getLong(position);
  }

  inline void setFloat(int position, jfloat f) {
    _array.setFloat(position, f);
  }

  inline jfloat getFloat(int position) {
    return _array.getFloat(position);
  }

  inline void setDouble(int position, jdouble d) {
    _array.setDouble(position, d);
  }

  inline jdouble getDouble(int position) {
    return _array.getDouble(position);
  }

  inline void setReference(int position, jobject l) {
    _array.setReference(position, l);
  }

  inline jobject getReference(int position) {
    return _array.getReference(position);
  }
};
}

