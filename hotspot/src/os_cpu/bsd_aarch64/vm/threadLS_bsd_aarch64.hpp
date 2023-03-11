/*
*
*/

#ifndef OS_CPU_BSD_AARCH64_VM_THREADLS_BSD_AARCH64_HPP
#define OS_CPU_BSD_AARCH64_VM_THREADLS_BSD_AARCH64_HPP

  // Processor dependent parts of ThreadLocalStorage

public:

  static Thread *thread() {
    return aarch64_currentThread;
  }

#endif // OS_CPU_BSD_AARCH64_VM_THREADLS_BSD_AARCH64_HPP
