/*
*
*/

#ifndef OS_CPU_BSD_AARCH64_VM_OS_BSD_AARCH64_HPP
#define OS_CPU_BSD_AARCH64_VM_OS_BSD_AARCH64_HPP

  static void setup_fpu();
  static bool supports_sse();

  static jlong rdtsc();

  static bool is_allocatable(size_t bytes);

  // Used to register dynamic code cache area with the OS
  // Note: Currently only used in 64 bit Windows implementations
  static bool register_code_area(char *low, char *high) { return true; }

  // Atomically copy 64 bits of data
  static void atomic_copy64(volatile void *src, volatile void *dst) {
#if defined(PPC) && !defined(_LP64)
    double tmp;
    asm volatile ("lfd  %0, 0(%1)\n"
                  "stfd %0, 0(%2)\n"
                  : "=f"(tmp)
                  : "b"(src), "b"(dst));
#elif defined(S390) && !defined(_LP64)
    double tmp;
    asm volatile ("ld  %0, 0(%1)\n"
                  "std %0, 0(%2)\n"
                  : "=r"(tmp)
                  : "a"(src), "a"(dst));
#else
    *(jlong *) dst = *(jlong *) src;
#endif
  }

#endif // OS_CPU_BSD_AARCH64_VM_OS_BSD_AARCH64_HPP
