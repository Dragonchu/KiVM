//
// Created by kiva on 2018/3/23.
//
#pragma once

#include <vector>
#include <kivm/oop/primitiveOop.h>
#include <kivm/oop/field.h>
#include <kivm/native/java_lang_String.h>

namespace kivm {
inline void helperInitField(std::vector<oop> &values, int offset, Field *field) {
  if (values.size() <= offset) {
    values.resize((unsigned long) offset + 1);
  }
  switch (field->getValueType()) {
    case ValueType::INT:
    case ValueType::SHORT:
    case ValueType::CHAR:
    case ValueType::BOOLEAN:
    case ValueType::BYTE:values[offset] = new intOopDesc(0);
      break;
    case ValueType::LONG:values[offset] = new longOopDesc(0L);
      break;
    case ValueType::FLOAT:values[offset] = new floatOopDesc(0.0f);
      break;
    case ValueType::DOUBLE:values[offset] = new doubleOopDesc(0.0);
      break;
    case ValueType::OBJECT:
    case ValueType::ARRAY:values[offset] = nullptr;
      break;

    case ValueType::VOID:SHOULD_NOT_REACH_HERE_M("Field cannot be typed void");
      break;
    default:SHOULD_NOT_REACH_HERE_M("Unrecognized field value type");
      break;
  }
}

inline bool helperInitConstantField(std::vector<oop> &values,
                                    int offset,
                                    cp_info **pool,
                                    Field *field) {
  if (values.size() <= offset) {
    values.resize((unsigned long) offset + 1);
  }

  ConstantValue_attribute *attr = field->getConstantAttribute();
  if (attr != nullptr) {
    cp_info *constant_info = pool[attr->constant_index];
    switch (constant_info->tag) {
      case CONSTANT_Long: {
        auto *info = (CONSTANT_Long_info *) constant_info;
        values[offset] = new longOopDesc(info->getConstant());
        break;
      }
      case CONSTANT_Float: {
        auto *info = (CONSTANT_Float_info *) constant_info;
        values[offset] = new floatOopDesc(info->getConstant());
        break;
      }
      case CONSTANT_Double: {
        auto *info = (CONSTANT_Double_info *) constant_info;
        values[offset] = new doubleOopDesc(info->getConstant());
        break;
      }
      case CONSTANT_Integer: {
        auto *info = (CONSTANT_Integer_info *) constant_info;
        values[offset] = new intOopDesc(info->getConstant());
        break;
      }
      case CONSTANT_String: {
        auto *info = (CONSTANT_String_info *) constant_info;
        auto *utf8 = (CONSTANT_Utf8_info *) pool[info->string_index];
        values[offset] = java::lang::String::intern(utf8->getConstant());
        break;
      }
      default: {
        SHOULD_NOT_REACH_HERE();
      }
    }
    return true;
  }

  return false;
}
}

