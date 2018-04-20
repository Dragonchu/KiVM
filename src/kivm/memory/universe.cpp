//
// Created by kiva on 2018/4/20.
//
#include <kivm/memory/universe.h>
#include <kivm/memory/markSweep.h>
#include <sys/mman.h>
#include <cerrno>

namespace kivm {
    CollectedHeap *Universe::sCollectedHeapInstance = nullptr;

    struct VirtualMemoryInfo {
        size_t memorySize;
    };

    void Universe::initialize() {
        Universe::sCollectedHeapInstance = new MarkSweepHeap;
        if (!Universe::sCollectedHeapInstance->initializeAll()) {
            PANIC("CollectedHeap initialization failed");
        }
    }

    void *Universe::allocVirtual(size_t size) {
        auto m = (jbyte *) mmap(nullptr, size + sizeof(VirtualMemoryInfo),
                                  PROT_READ | PROT_WRITE,
                                  MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (m == MAP_FAILED) {
            PANIC("mmap() failed: %s", strerror(errno));
            return nullptr;
        }

        auto memoryInfo = (VirtualMemoryInfo *) m;
        memoryInfo->memorySize = size;
        return m + sizeof(VirtualMemoryInfo);
    }

    void Universe::deallocVirtual(void *memory) {
        jbyte *m = ((jbyte *) memory) - sizeof(VirtualMemoryInfo);
        auto memoryInfo = (VirtualMemoryInfo *) m;
        munmap(m, memoryInfo->memorySize);
    }

    void *Universe::allocHeap(size_t size) {
        if (sCollectedHeapInstance == nullptr) {
            PANIC("heap not initialized");
        }
        return sCollectedHeapInstance->allocate(size);
    }
}