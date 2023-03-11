/*
*
*/

#ifndef OS_CPU_BSD_AARCH64_VM_ATOMIC_BSD_AARCH64_INLINE_HPP
#define OS_CPU_BSD_AARCH64_VM_ATOMIC_BSD_AARCH64_INLINE_HPP

#include "runtime/atomic.hpp"
#include "runtime/os.hpp"
#include "vm_version_aarch64.hpp"

// Implementation of class atomic

#define FULL_MEM_BARRIER  __sync_synchronize()
#define READ_MEM_BARRIER  __atomic_thread_fence(__ATOMIC_ACQUIRE);
#define WRITE_MEM_BARRIER __atomic_thread_fence(__ATOMIC_RELEASE);

inline void Atomic::store    (jbyte    store_value, jbyte*    dest) { *dest = store_value; }
inline void Atomic::store    (jshort   store_value, jshort*   dest) { *dest = store_value; }
inline void Atomic::store    (jint     store_value, jint*     dest) { *dest = store_value; }
inline void Atomic::store_ptr(intptr_t store_value, intptr_t* dest) { *dest = store_value; }
inline void Atomic::store_ptr(void*    store_value, void*     dest) { *(void**)dest = store_value; }

inline void Atomic::store    (jbyte    store_value, volatile jbyte*    dest) { *dest = store_value; }
inline void Atomic::store    (jshort   store_value, volatile jshort*   dest) { *dest = store_value; }
inline void Atomic::store    (jint     store_value, volatile jint*     dest) { *dest = store_value; }
inline void Atomic::store_ptr(intptr_t store_value, volatile intptr_t* dest) { *dest = store_value; }
inline void Atomic::store_ptr(void*    store_value, volatile void*     dest) { *(void* volatile *)dest = store_value; }


inline jint Atomic::add(jint add_value, volatile jint* dest)
{
 return __sync_add_and_fetch(dest, add_value);
}

inline void Atomic::inc(volatile jint* dest)
{
 add(1, dest);
}

inline void Atomic::inc_ptr(volatile void* dest)
{
 add_ptr(1, dest);
}

inline void Atomic::dec (volatile jint* dest)
{
 add(-1, dest);
}

inline void Atomic::dec_ptr(volatile void* dest)
{
 add_ptr(-1, dest);
}

inline jint Atomic::xchg (jint exchange_value, volatile jint* dest)
{
  jint res = __sync_lock_test_and_set (dest, exchange_value);
  FULL_MEM_BARRIER;
  return res;
}

inline void* Atomic::xchg_ptr(void* exchange_value, volatile void* dest)
{
  return (void *) xchg_ptr((intptr_t) exchange_value,
                           (volatile intptr_t*) dest);
}

inline jint Atomic::cmpxchg (jint exchange_value, volatile jint* dest, jint compare_value)
{
 return __sync_val_compare_and_swap(dest, compare_value, exchange_value);
}

inline void Atomic::store (jlong store_value, jlong* dest) { *dest = store_value; }
inline void Atomic::store (jlong store_value, volatile jlong* dest) { *dest = store_value; }

inline intptr_t Atomic::add_ptr(intptr_t add_value, volatile intptr_t* dest)
{
 return __sync_add_and_fetch(dest, add_value);
}

inline void* Atomic::add_ptr(intptr_t add_value, volatile void* dest)
{
  return (void *) add_ptr(add_value, (volatile intptr_t *) dest);
}

inline void Atomic::inc_ptr(volatile intptr_t* dest)
{
 add_ptr(1, dest);
}

inline void Atomic::dec_ptr(volatile intptr_t* dest)
{
 add_ptr(-1, dest);
}

inline intptr_t Atomic::xchg_ptr(intptr_t exchange_value, volatile intptr_t* dest)
{
  intptr_t res = __sync_lock_test_and_set (dest, exchange_value);
  FULL_MEM_BARRIER;
  return res;
}

inline jlong Atomic::cmpxchg (jlong exchange_value, volatile jlong* dest, jlong compare_value)
{
 return __sync_val_compare_and_swap(dest, compare_value, exchange_value);
}

inline intptr_t Atomic::cmpxchg_ptr(intptr_t exchange_value, volatile intptr_t* dest, intptr_t compare_value)
{
 return __sync_val_compare_and_swap(dest, compare_value, exchange_value);
}

inline void* Atomic::cmpxchg_ptr(void* exchange_value, volatile void* dest, void* compare_value)
{
  return (void *) cmpxchg_ptr((intptr_t) exchange_value,
                              (volatile intptr_t*) dest,
                              (intptr_t) compare_value);
}

inline jlong Atomic::load(volatile jlong* src) { return *src; }

#endif // OS_CPU_BSD_AARCH64_VM_ATOMIC_BSD_AARCH64_INLINE_HPP
