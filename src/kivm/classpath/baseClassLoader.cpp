//
// Created by kiva on 2018/3/1.
//

#include <kivm/classpath/classLoader.h>
#include <kivm/oop/klass.h>
#include <kivm/oop/instanceKlass.h>
#include <kivm/oop/arrayKlass.h>
#include <kivm/classfile/classFileParser.h>
#include <kivm/classpath/classPathManager.h>

namespace kivm {
    Klass *BaseClassLoader::loadClass(const String &className) {
        // Load array class
        if (className[0] == L'[') {
            // [I
            int dimension = 0;
            while (className[++dimension] == L'[') continue;

            // Load 1-dimension array directly
            if (dimension == 1) {
                // for example: [Ljava/lang/Object;
                if (className[1] == L'L') {
                    // java/lang/Object
                    const String &component = className.substr(2, className.size() - 3);
                    auto *component_class = (InstanceKlass *) loadClass(component);
                    return component_class != nullptr
                           ? new ObjectArrayKlass(this, nullptr, dimension, component_class)
                           : nullptr;
                }

                // for example: LI -> I, LB -> B
                // use primitiveTypeToValueTypeNoWrap() instead of primitiveTypeToValueType()
                // because in Java, booleans, chars, shorts and bytes are not ints
                ValueType component_type = primitiveTypeToValueTypeNoWrap(className[1]);
                return new TypeArrayKlass(this, nullptr, dimension, component_type);
            }

            // Load multi-dimension array recursively
            // remove the first '['
            const String &down_type_name = className.substr(1);
            auto *down_type = (ArrayKlass *) loadClass(down_type_name);
            if (down_type == nullptr) {
                return nullptr;
            }

            if (down_type->isObjectArray()) {
                return new ObjectArrayKlass(this, (ObjectArrayKlass *) down_type);
            } else {
                return new TypeArrayKlass(this, (TypeArrayKlass *) down_type);
            }
        }

        if (className.substr(0, 3) == L"com") {
            EXPLORE("Class is instance class %S", className.c_str());
        }
        // Load instance class
        ClassPathManager *cpm = ClassPathManager::get();
        const auto &result = cpm->searchClass(className);
        if (result._source == ClassSource::NOT_FOUND) {
            return nullptr;
        }
        if (className.substr(0, 3) == L"com") {
            EXPLORE("Parsing class %S", className.c_str());
        }
        ClassFileParser fileParser(result._file, result._buffer, result._bufferSize);
        ClassFile *classFile = fileParser.getParsedClassFile();
        Klass *klass = classFile != nullptr
                       ? new InstanceKlass(classFile, this, nullptr, ClassType::INSTANCE_CLASS)
                       : nullptr;
        result.closeResource();
        if (className.substr(0, 3) == L"com") {
            EXPLORE("Class %S parsed", className.c_str());
        }
        return klass;
    }

    Klass *BaseClassLoader::loadClass(u1 *classBytes, size_t classSize) {
        if (classBytes == nullptr) {
            return nullptr;
        }

        ClassFileParser fileParser(L"<stream>", classBytes, classSize);
        ClassFile *classFile = fileParser.getParsedClassFile();
        return classFile != nullptr
               ? new InstanceKlass(classFile, this, nullptr, ClassType::INSTANCE_CLASS)
               : nullptr;
    }
}
