/*
*
*/

#ifndef OS_CPU_BSD_AARCH64_VM_PREFETCH_BSD_AARCH64_INLINE_HPP
#define OS_CPU_BSD_AARCH64_VM_PREFETCH_BSD_AARCH64_INLINE_HPP

#include "runtime/prefetch.hpp"


inline void Prefetch::read (void *loc, intx interval) {
  if (interval >= 0)
    asm("prfm PLDL1KEEP, [%0, %1]" : : "r"(loc), "r"(interval));
}

inline void Prefetch::write(void *loc, intx interval) {
  if (interval >= 0)
    asm("prfm PSTL1KEEP, [%0, %1]" : : "r"(loc), "r"(interval));
}

#endif // OS_CPU_BSD_AARCH64_VM_PREFETCH_BSD_AARCH64_INLINE_HPP
