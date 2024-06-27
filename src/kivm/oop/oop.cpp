//
// Created by kiva on 2018/2/25.
//

#include <kivm/oop/oop.h>

namespace kivm {
markOopDesc::markOopDesc(oopType type)
    : _type(type), _hash(0) {
}

oopDesc::oopDesc(Klass *klass, oopType type) {
  this->_klass = klass;
  this->_mark = new markOopDesc(type);
}

oopDesc::~oopDesc() {
  delete this->_mark;
}
}
