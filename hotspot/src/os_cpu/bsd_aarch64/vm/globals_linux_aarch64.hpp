/*
*
*/

#ifndef OS_CPU_BSD_AARCH64_VM_GLOBALS_BSD_AARCH64_HPP
#define OS_CPU_BSD_AARCH64_VM_GLOBALS_BSD_AARCH64_HPP

// Sets the default values for platform dependent flags used by the runtime system.
// (see globals.hpp)

define_pd_global(bool, DontYieldALot,            false);
define_pd_global(intx, ThreadStackSize,          2048); // 0 => use system default
define_pd_global(intx, VMThreadStackSize,        2048);

define_pd_global(intx, CompilerThreadStackSize,  0);

define_pd_global(uintx,JVMInvokeMethodSlack,     8192);

// Used on 64 bit platforms for UseCompressedOops base address
define_pd_global(uintx,HeapBaseMinAddress,       2*G);

extern __thread Thread *aarch64_currentThread;

#endif // OS_CPU_BSD_AARCH64_VM_GLOBALS_BSD_AARCH64_HPP
