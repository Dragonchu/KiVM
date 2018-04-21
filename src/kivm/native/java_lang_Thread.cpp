//
// Created by kiva on 2018/4/15.
//

#include <kivm/native/java_lang_Thread.h>
#include <kivm/kivm.h>
#include <kivm/bytecode/execution.h>
#include <kivm/native/classNames.h>

using namespace kivm;

extern "C" jobject Java_java_lang_Thread_currentThread(JNIEnv *env, jclass java_lang_Thread) {
    auto currentThread = Threads::currentThread();
    if (currentThread == nullptr) {
        PANIC("currentThread cannot be null");
    }

    return currentThread->getJavaThreadObject();
}

extern "C" void Java_java_lang_Thread_setPriority0(JNIEnv *env, jobject threadObject, jint priority) {
    auto instanceOop = Resolver::resolveInstance(threadObject);
    // TODO: set native thread's priority
}
