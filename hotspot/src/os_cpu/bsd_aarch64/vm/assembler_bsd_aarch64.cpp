/*
*
*/

#include "precompiled.hpp"
#include "asm/macroAssembler.hpp"
#include "asm/macroAssembler.inline.hpp"
#include "runtime/os.hpp"
#include "runtime/threadLocalStorage.hpp"

void MacroAssembler::get_thread(Register dst) {
  // call pthread_getspecific
  // void * pthread_getspecific(pthread_key_t key);

  // Save all call-clobbered regs except dst, plus r19 and r20.
  RegSet saved_regs = RegSet::range(r0, r20) + lr - dst;
  push(saved_regs, sp);
  mov(c_rarg0, ThreadLocalStorage::thread_index());
  mov(r19, CAST_FROM_FN_PTR(address, pthread_getspecific));
  blr(r19);
  if (dst != c_rarg0) {
    mov(dst, c_rarg0);
  }
  // restore pushed registers
  pop(saved_regs, sp);
}
