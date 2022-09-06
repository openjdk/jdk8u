/*
 * Copyright (c) 1999, 2018, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020, Huawei Technologies Co., Ltd. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

// no precompiled headers
#include "asm/macroAssembler.hpp"
#include "classfile/classLoader.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/vmSymbols.hpp"
#include "code/codeCache.hpp"
#include "code/icBuffer.hpp"
//#include "code/nativeInst.hpp"
#include "code/vtableStubs.hpp"
#include "interpreter/interpreter.hpp"
#include "jvm.h"
#include "memory/allocation.inline.hpp"
#include "os_share_linux.hpp"
#include "prims/jniFastGetField.hpp"
#include "prims/jvm_misc.hpp"
#include "runtime/arguments.hpp"
#include "runtime/extendedPC.hpp"
#include "runtime/frame.inline.hpp"
//#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/java.hpp"
#include "runtime/javaCalls.hpp"
#include "runtime/mutexLocker.hpp"
#include "runtime/osThread.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"
#include "runtime/thread.inline.hpp"
#include "runtime/timer.hpp"
#include "utilities/debug.hpp"
#include "utilities/events.hpp"
#include "utilities/vmError.hpp"
#include "prims/jvm.h"
#include "jvm_linux.h"
#include "mutex_linux.inline.hpp"

// put OS-includes here
# include <dlfcn.h>
# include <fpu_control.h>
# include <errno.h>
# include <pthread.h>
# include <signal.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/mman.h>
# include <sys/resource.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/utsname.h>
# include <sys/wait.h>
# include <poll.h>
# include <pwd.h>
# include <ucontext.h>
# include <unistd.h>

#define REG_LR       1
#define REG_FP       8

NOINLINE address os::current_stack_pointer() {
  return (address)__builtin_frame_address(0);
}

char* os::non_memory_address_word() {
  // Must never look like an address returned by reserve_memory,
  return (char*) -1;
}

address os::Linux::ucontext_get_pc( ucontext_t * uc) {
  return (address)uc->uc_mcontext.__gregs[REG_PC];
}
void os::initialize_thread(Thread *thr) {
}

void os::Linux::ucontext_set_pc(ucontext_t * uc, address pc) {
  uc->uc_mcontext.__gregs[REG_PC] = (intptr_t)pc;
}

intptr_t* os::Linux::ucontext_get_sp( ucontext_t * uc) {
  return (intptr_t*)uc->uc_mcontext.__gregs[REG_SP];
}

intptr_t* os::Linux::ucontext_get_fp( ucontext_t * uc) {
  return (intptr_t*)uc->uc_mcontext.__gregs[REG_FP];
}

static address handle_unsafe_access(JavaThread* thread, address pc) {
  // pc is the instruction which we must emulate
  // doing a no-op is fine:  return garbage from the load
  // therefore, compute npc
  address npc = pc + NativeCall::instruction_size;

  // request an async exception
  thread->set_pending_unsafe_access_error();
 
  // return address of next instruction to execute
  return npc;
}
// For Forte Analyzer AsyncGetCallTrace profiling support - thread
// is currently interrupted by SIGPROF.
// os::Solaris::fetch_frame_from_ucontext() tries to skip nested signal
// frames. Currently we don't do that on Linux, so it's the same as
// os::fetch_frame_from_context().
ExtendedPC os::Linux::fetch_frame_from_ucontext(Thread* thread,
   ucontext_t* uc, intptr_t** ret_sp, intptr_t** ret_fp) {

  assert(thread != NULL, "just checking");
  assert(ret_sp != NULL, "just checking");
  assert(ret_fp != NULL, "just checking");

  return os::fetch_frame_from_context(uc, ret_sp, ret_fp);
}

ExtendedPC os::fetch_frame_from_context( void* ucVoid,
                                        intptr_t** ret_sp, intptr_t** ret_fp) {

  ExtendedPC  epc;
   ucontext_t* uc = ( ucontext_t*)ucVoid;

  if (uc != NULL) {
    epc = ExtendedPC(os::Linux::ucontext_get_pc(uc));
    if (ret_sp != NULL) {
      *ret_sp = os::Linux::ucontext_get_sp(uc);
    }
    if (ret_fp != NULL) {
      *ret_fp = os::Linux::ucontext_get_fp(uc);
    }
  } else {
    // construct empty ExtendedPC for return value checking
    epc = ExtendedPC(NULL);
    if (ret_sp != NULL) {
      *ret_sp = (intptr_t *)NULL;
    }
    if (ret_fp != NULL) {
      *ret_fp = (intptr_t *)NULL;
    }
  }

  return epc;
}

frame os::fetch_frame_from_context( void* ucVoid) {
  intptr_t* frame_sp = NULL;
  intptr_t* frame_fp = NULL;
  ExtendedPC epc = fetch_frame_from_context(ucVoid, &frame_sp, &frame_fp);
  return frame(frame_sp, frame_fp, epc.pc());
}

bool os::Linux::get_frame_at_stack_banging_point(JavaThread* thread, ucontext_t* uc, frame* fr) {
  address pc = (address) os::Linux::ucontext_get_pc(uc);
  if (Interpreter::contains(pc)) {
    // interpreter performs stack banging after the fixed frame header has
    // been generated while the compilers perform it before. To maintain
    // semantic consistency between interpreted and compiled frames, the
    // method returns the Java sender of the current frame.
    *fr = os::fetch_frame_from_context(uc);
    if (!fr->is_first_java_frame()) {
      assert(fr->safe_for_sender(thread), "Safety check");
      *fr = fr->java_sender();
    }
  } else {
    // more complex code with compiled code
    assert(!Interpreter::contains(pc), "Interpreted methods should have been handled above");
    CodeBlob* cb = CodeCache::find_blob(pc);
    if (cb == NULL || !cb->is_nmethod() || cb->is_frame_complete_at(pc)) {
      // Not sure where the pc points to, fallback to default
      // stack overflow handling
      return false;
    } else {
      // In compiled code, the stack banging is performed before LR
      // has been saved in the frame.  LR is live, and SP and FP
      // belong to the caller.
      intptr_t* frame_fp = os::Linux::ucontext_get_fp(uc);
      intptr_t* frame_sp = os::Linux::ucontext_get_sp(uc);
      address frame_pc = (address)(uintptr_t)(uc->uc_mcontext.__gregs[REG_LR] -
                         NativeInstruction::instruction_size);
      *fr = frame(frame_sp, frame_fp, frame_pc);
      if (!fr->is_java_frame()) {
        assert(fr->safe_for_sender(thread), "Safety check");
        assert(!fr->is_first_frame(), "Safety check");
        *fr = fr->java_sender();
      }
    }
  }
  assert(fr->is_java_frame(), "Safety check");
  return true;
}

// By default, gcc always saves frame pointer rfp on this stack. This
// may get turned off by -fomit-frame-pointer.
frame os::get_sender_for_C_frame(frame* fr) {
  return frame(fr->c_frame_sender_sp(), fr->c_frame_link(), fr->c_frame_sender_pc());
}

NOINLINE frame os::current_frame() {
  intptr_t **sender_sp = (intptr_t **)__builtin_frame_address(0);
  if(sender_sp != NULL) {
    frame myframe((intptr_t*)os::current_stack_pointer(),
                  sender_sp[frame::c_frame_link_offset],
                  CAST_FROM_FN_PTR(address, os::current_frame));
    if (os::is_first_C_frame(&myframe)) {
      // stack is not walkable
      return frame();
    } else {
      return os::get_sender_for_C_frame(&myframe);
    }
  } else {
    ShouldNotReachHere();
    return frame();
  }
}

/*bool os::is_first_C_frame(frame* fr) {
  // Load up sp, fp, sender sp and sender fp, check for reasonable values.
  // Check usp first, because if that's bad the other accessors may fault
  // on some architectures.  Ditto ufp second, etc.
  uintptr_t fp_align_mask = (uintptr_t)(sizeof(address) - 1);
  // sp on amd can be 32 bit aligned.
  uintptr_t sp_align_mask = (uintptr_t)(sizeof(int) - 1);

  uintptr_t usp    = (uintptr_t)fr->sp();
  if ((usp & sp_align_mask) != 0) {
    return true;
  }

  uintptr_t ufp    = (uintptr_t)fr->fp();
  if ((ufp & fp_align_mask) != 0) {
    return true;
  }

  uintptr_t old_sp = (uintptr_t)fr->c_frame_sender_sp();
  if ((old_sp & sp_align_mask) != 0) {
    return true;
  }
  if (old_sp == 0 || old_sp == (uintptr_t)-1) {
    return true;
  }

  uintptr_t old_fp = (uintptr_t)fr->c_frame_link();
  if ((old_fp & fp_align_mask) != 0) {
    return true;
  }
  if (old_fp == 0 || old_fp == (uintptr_t)-1 || old_fp == ufp) {
    return true;
  }

  // stack grows downwards; if old_fp is below current fp or if the stack
  // frame is too large, either the stack is corrupted or fp is not saved
  // on stack (i.e. on x86, ebp may be used as general register). The stack
  // is not walkable beyond current frame.
  if (old_fp < ufp) {
    return true;
  }
  if (old_fp - ufp > 64 * K) {
    return true;
  }

  return false;
}*/

int os::get_native_stack(address* stack, int frames, int toSkip) {
  int frame_idx = 0;
  int num_of_frames = 0;  // number of frames captured
  frame fr = os::current_frame();
  while (fr.pc() && frame_idx < frames) {
    if (toSkip > 0) {
      toSkip --;
    } else {
      stack[frame_idx ++] = fr.pc();
    }
    if (fr.fp() == NULL || fr.cb() != NULL ||
        fr.c_frame_sender_pc() == NULL || os::is_first_C_frame(&fr)) {
      break;
    }

    if (fr.c_frame_sender_pc() && !os::is_first_C_frame(&fr)) {
      fr = os::get_sender_for_C_frame(&fr);
    } else {
      break;
    }
  }
  num_of_frames = frame_idx;
  for (; frame_idx < frames; frame_idx ++) {
    stack[frame_idx] = NULL;
  }

  return num_of_frames;
}

// Utility functions
extern "C" JNIEXPORT int
JVM_handle_linux_signal(int sig,
                        siginfo_t* info,
                        void* ucVoid,
                        int abort_if_unrecognized) {
  ucontext_t* uc = (ucontext_t*) ucVoid;

  Thread* t = ThreadLocalStorage::get_thread_slow();

  // Must do this before SignalHandlerMark, if crash protection installed we will longjmp away
  // (no destructors can be run)
  os::ThreadCrashProtection::check_crash_protection(sig, t);

  SignalHandlerMark shm(t);

  // Note: it's not uncommon that JNI code uses signal/sigset to install
  // then restore certain signal handler (e.g. to temporarily block SIGPIPE,
  // or have a SIGILL handler when detecting CPU type). When that happens,
  // JVM_handle_linux_signal() might be invoked with junk info/ucVoid. To
  // avoid unnecessary crash when libjsig is not preloaded, try handle signals
  // that do not require siginfo/ucontext first.

  if (sig == SIGPIPE || sig == SIGXFSZ) {
    // allow chained handler to go first
    if (os::Linux::chained_handler(sig, info, ucVoid)) {
      return true;
    } else {
      // Ignoring SIGPIPE/SIGXFSZ - see bugs 4229104 or 6499219
      return true;
    }
  }

#ifdef CAN_SHOW_REGISTERS_ON_ASSERT
  if ((sig == SIGSEGV || sig == SIGBUS) && info != NULL && info->si_addr == g_assert_poison) {
    handle_assert_poison_fault(ucVoid, info->si_addr);
    return 1;
  }
#endif

  JavaThread* thread = NULL;
  VMThread* vmthread = NULL;
  if (os::Linux::signal_handlers_are_installed) {
    if (t != NULL ) {
      if(t->is_Java_thread()) {
        thread = (JavaThread*)t;
      } else if(t->is_VM_thread()) {
        vmthread = (VMThread *)t;
      }
    }
  }

  // decide if this trap can be handled by a stub
  address stub = NULL;

  address pc = NULL;

  //%note os_trap_1
  if (info != NULL && uc != NULL && thread != NULL) {
    pc = (address) os::Linux::ucontext_get_pc(uc);

    if (StubRoutines::is_safefetch_fault(pc)) {
      os::Linux::ucontext_set_pc(uc, StubRoutines::continuation_for_safefetch_fault(pc));
      return 1;
    }

    // Handle ALL stack overflow variations here
    if (sig == SIGSEGV) {
      address addr = (address) info->si_addr;

      // check if fault address is within thread stack
 if (sig == SIGSEGV) {
      address addr = (address) info->si_addr;

      // check if fault address is within thread stack
      if (addr < thread->stack_base() &&
          addr >= thread->stack_base() - thread->stack_size()) {
        // stack overflow
        if (thread->in_stack_yellow_zone(addr)) {
          thread->disable_stack_yellow_zone();
          if (thread->thread_state() == _thread_in_Java) {
            // Throw a stack overflow exception.  Guard pages will be reenabled
            // while unwinding the stack.
            stub = SharedRuntime::continuation_for_implicit_exception(thread, pc, SharedRuntime::STACK_OVERFLOW);
          } else {
            // Thread was in the vm or native code.  Return and try to finish.
            return 1;
          }
        } else if (thread->in_stack_red_zone(addr)) {
          // Fatal red zone violation.  Disable the guard pages and fall through
          // to handle_unexpected_exception way down below.
          thread->disable_stack_red_zone();
          tty->print_raw_cr("An irrecoverable stack overflow has occurred.");

          // This is a likely cause, but hard to verify. Let's just print
          // it as a hint.
          tty->print_raw_cr("Please check if any of your loaded .so files has "
                            "enabled executable stack (see man page execstack(8))");
        } else {
          // Accessing stack address below sp may cause SEGV if current
          // thread has MAP_GROWSDOWN stack. This should only happen when
          // current thread was created by user code with MAP_GROWSDOWN flag
          // and then attached to VM. See notes in os_linux.cpp.
          if (thread->osthread()->expanding_stack() == 0) {
             thread->osthread()->set_expanding_stack();
             if (os::Linux::manually_expand_stack(thread, addr)) {
               thread->osthread()->clear_expanding_stack();
               return 1;
             }
             thread->osthread()->clear_expanding_stack();
          } else {
             fatal("recursive segv. expanding stack.");
          }
        }
      }
    }

    if (thread->thread_state() == _thread_in_Java) {
      // Java thread running in Java code => find exception handler if any
      // a fault inside compiled code, the interpreter, or a stub

      // Handle signal from NativeJump::patch_verified_entry().
      if ((sig == SIGILL || sig == SIGTRAP)
          && nativeInstruction_at(pc)->is_sigill_zombie_not_entrant()) {
        if (TraceTraps) {
          tty->print_cr("trap: zombie_not_entrant (%s)", (sig == SIGTRAP) ? "SIGTRAP" : "SIGILL");
        }
        stub = SharedRuntime::get_handle_wrong_method_stub();
      } else if (sig == SIGSEGV && os::is_poll_address((address)info->si_addr)) {
        stub = SharedRuntime::get_poll_stub(pc);
      } else if (sig == SIGBUS /* && info->si_code == BUS_OBJERR */) {
        // BugId 4454115: A read from a MappedByteBuffer can fault
        // here if the underlying file has been truncated.
        // Do not crash the VM in such a case.
        CodeBlob* cb = CodeCache::find_blob_unsafe(pc);
        nmethod* nm = (cb != NULL && cb->is_nmethod()) ? (nmethod*)cb : NULL;
        if (nm != NULL && nm->has_unsafe_access()) {
          address next_pc = pc + NativeCall::instruction_size;
          stub = handle_unsafe_access(thread, next_pc);
        }
      } else if (sig == SIGFPE  &&
          (info->si_code == FPE_INTDIV || info->si_code == FPE_FLTDIV)) {
        stub =
          SharedRuntime::
          continuation_for_implicit_exception(thread,
                                              pc,
                                              SharedRuntime::
                                              IMPLICIT_DIVIDE_BY_ZERO);
      } else if (sig == SIGSEGV &&
                 !MacroAssembler::needs_explicit_null_check((intptr_t)info->si_addr)) {
          // Determination of interpreter/vtable stub/compiled code null exception
          stub = SharedRuntime::continuation_for_implicit_exception(thread, pc, SharedRuntime::IMPLICIT_NULL);
      }
    } else if (thread->thread_state() == _thread_in_vm &&
               sig == SIGBUS && /* info->si_code == BUS_OBJERR && */
               thread->doing_unsafe_access()) {
      address next_pc = pc + NativeCall::instruction_size;
      stub = handle_unsafe_access(thread, next_pc);
    }

    // jni_fast_Get<Primitive>Field can trap at certain pc's if a GC kicks in
    // and the heap gets shrunk before the field access.
    if ((sig == SIGSEGV) || (sig == SIGBUS)) {
      address addr = JNI_FastGetField::find_slowcase_pc(pc);
      if (addr != (address)-1) {
        stub = addr;
      }
    }

    // Check to see if we caught the safepoint code in the
    // process of write protecting the memory serialization page.
    // It write enables the page immediately after protecting it
    // so we can just return to retry the write.
    if ((sig == SIGSEGV) &&
        os::is_memory_serialize_page(thread, (address) info->si_addr)) {
      // Block current thread until the memory serialize page permission restored.
      os::block_on_serialize_page_trap();
      return true;
    }
  }

  if (stub != NULL) {
    // save all thread context in case we need to restore it
    if (thread != NULL) {
      thread->set_saved_exception_pc(pc);
    }

    os::Linux::ucontext_set_pc(uc, stub);
    return true;
  }

  // signal-chaining
  if (os::Linux::chained_handler(sig, info, ucVoid)) {
     return true;
  }

  if (!abort_if_unrecognized) {
    // caller wants another chance, so give it to him
    return false;
  }

  if (pc == NULL && uc != NULL) {
    pc = os::Linux::ucontext_get_pc(uc);
  }

  // unmask current signal
  sigset_t newset;
  sigemptyset(&newset);
  sigaddset(&newset, sig);
  sigprocmask(SIG_UNBLOCK, &newset, NULL);

  VMError err(t, sig, pc, info, ucVoid);
  err.report_and_die();

  ShouldNotReachHere();
  return true; // Mute compiler
}
}

void os::Linux::init_thread_fpu_state(void) {
}

int os::Linux::get_fpu_control_word(void) {
  return 0;
}

void os::Linux::set_fpu_control_word(int fpu_control) {
}


////////////////////////////////////////////////////////////////////////////////
// thread stack

// Minimum usable stack sizes required to get to user code. Space for
// HotSpot guard pages is added later.
//size_t os::Posix::_compiler_thread_min_stack_allowed = 72 * K;
//size_t os::Posix::_java_thread_min_stack_allowed = 72 * K;
//size_t os::Posix::_vm_internal_thread_min_stack_allowed = 72 * K;
size_t os::Linux::min_stack_allowed  = 64 * K;
// return default stack size for thr_type
size_t os::Linux::default_stack_size(os::ThreadType thr_type) {
   // default stack size (compiler thread needs larger stack)
   size_t s = (thr_type == os::compiler_thread ? 4 * M : 1 * M);
   return s;
 }
// amd64: pthread on amd64 is always in floating stack mode
bool os::Linux::supports_variable_stack_size() {  return true; }

// return default stack size for thr_type
size_t os::Linux::default_guard_size(os::ThreadType thr_type) {
  // Creating guard page is very expensive. Java thread has HotSpot
  // guard page, only enable glibc guard page for non-Java threads.
  return (thr_type == java_thread ? 0 : page_size());
}
static void current_stack_region(address * bottom, size_t * size) {
  if (os::is_primordial_thread()) {
     // primordial thread needs special handling because pthread_getattr_np()
     // may return bogus value.
     *bottom = os::Linux::initial_thread_stack_bottom();
     *size   = os::Linux::initial_thread_stack_size();
  } else {
     pthread_attr_t attr;

     int rslt = pthread_getattr_np(pthread_self(), &attr);

     // JVM needs to know exact stack location, abort if it fails
     if (rslt != 0) {
       if (rslt == ENOMEM) {
         vm_exit_out_of_memory(0, OOM_MMAP_ERROR, "pthread_getattr_np");
       } else {
         fatal(err_msg("pthread_getattr_np failed with errno = %d", rslt));
       }
     }

     if (pthread_attr_getstack(&attr, (void **)bottom, size) != 0) {
         fatal("Can not locate current stack attributes!");
     }

     pthread_attr_destroy(&attr);

  }
  assert(os::current_stack_pointer() >= *bottom &&
         os::current_stack_pointer() < *bottom + *size, "just checking");
}
size_t os::current_stack_size() {
  // stack size includes normal stack and HotSpot guard pages
  address bottom;
  size_t size;
  current_stack_region(&bottom, &size);
  return size;
}
address os::current_stack_base() {
  address bottom;
  size_t size;
  current_stack_region(&bottom, &size);
  return (bottom + size);
}
/////////////////////////////////////////////////////////////////////////////
// helper functions for fatal error handler

static const char* reg_abi_names[] = {
  "pc",
  "x1(ra)", "x2(sp)", "x3(gp)", "x4(tp)",
  "x5(t0)", "x6(t1)", "x7(t2)",
  "x8(s0)", "x9(s1)",
  "x10(a0)", "x11(a1)", "x12(a2)", "x13(a3)", "x14(a4)", "x15(a5)", "x16(a6)", "x17(a7)",
  "x18(s2)", "x19(s3)", "x20(s4)", "x21(s5)", "x22(s6)", "x23(s7)", "x24(s8)", "x25(s9)", "x26(s10)", "x27(s11)",
  "x28(t3)", "x29(t4)","x30(t5)", "x31(t6)"
};

void os::print_context(outputStream *st, void *context) {
  if (context == NULL) {
    return;
  }

   ucontext_t *uc = ( ucontext_t*)context;
  st->print_cr("Registers:");
  for (int r = 0; r < 32; r++) {
    st->print("%-*.*s=", 8, 8, reg_abi_names[r]);
    print_location(st, uc->uc_mcontext.__gregs[r]);
  }
  st->cr();

  intptr_t *frame_sp = (intptr_t *)os::Linux::ucontext_get_sp(uc);
  st->print_cr("Top of Stack: (sp=" PTR_FORMAT ")", p2i(frame_sp));
  print_hex_dump(st, (address)frame_sp, (address)(frame_sp + 8 * sizeof(intptr_t)), sizeof(intptr_t));
  st->cr();

  // Note: it may be unsafe to inspect memory near pc. For example, pc may
  // point to garbage if entry point in an nmethod is corrupted. Leave
  // this at the end, and hope for the best.
  address pc = os::Linux::ucontext_get_pc(uc);
  print_instructions(st, pc, sizeof(char));
  st->cr();
}

void os::print_register_info(outputStream *st,  void *context) {
  if (context == NULL) {
    return;
  }

   ucontext_t *uc = ( ucontext_t*)context;

  st->print_cr("Register to memory mapping:");
  st->cr();

  // this is horrendously verbose but the layout of the registers in the
  // context does not match how we defined our abstract Register set, so
  // we can't just iterate through the gregs area

  // this is only for the "general purpose" registers

  for (int r = 0; r < 32; r++)
    st->print_cr("%-*.*s=" INTPTR_FORMAT, 8, 8, reg_abi_names[r], (uintptr_t)uc->uc_mcontext.__gregs[r]);
  st->cr();
}

void os::setup_fpu() {
}

#ifndef PRODUCT
void os::verify_stack_alignment() {
  assert(((intptr_t)os::current_stack_pointer() & (StackAlignmentInBytes-1)) == 0, "incorrect stack alignment");
}
#endif

int os::extra_bang_size_in_bytes() {
  return 0;
}

extern "C" {
  int SpinPause() {
    return 0;
  }

  void _Copy_conjoint_jshorts_atomic(jshort* from, jshort* to, size_t count) {
    if (from > to) {
      const jshort *end = from + count;
      while (from < end) {
        *(to++) = *(from++);
      }
    } else if (from < to) {
      const jshort *end = from;
      from += count - 1;
      to   += count - 1;
      while (from >= end) {
        *(to--) = *(from--);
      }
    }
  }
  void _Copy_conjoint_jints_atomic(jint* from, jint* to, size_t count) {
    if (from > to) {
      const jint *end = from + count;
      while (from < end) {
        *(to++) = *(from++);
      }
    } else if (from < to) {
      const jint *end = from;
      from += count - 1;
      to   += count - 1;
      while (from >= end) {
        *(to--) = *(from--);
      }
    }
  }
  void _Copy_conjoint_jlongs_atomic(jlong* from, jlong* to, size_t count) {
    if (from > to) {
      const jlong *end = from + count;
      while (from < end) {
        os::atomic_copy64(from++, to++);
      }
    } else if (from < to) {
      const jlong *end = from;
      from += count - 1;
      to   += count - 1;
      while (from >= end) {
        os::atomic_copy64(from--, to--);
      }
    }
  }

  void _Copy_arrayof_conjoint_bytes(HeapWord* from,
                                    HeapWord* to,
                                    size_t    count) {
    memmove(to, from, count);
  }
  void _Copy_arrayof_conjoint_jshorts(HeapWord* from,
                                      HeapWord* to,
                                      size_t    count) {
    memmove(to, from, count * 2);
  }
  void _Copy_arrayof_conjoint_jints(HeapWord* from,
                                    HeapWord* to,
                                    size_t    count) {
    memmove(to, from, count * 4);
  }
  void _Copy_arrayof_conjoint_jlongs(HeapWord* from,
                                     HeapWord* to,
                                     size_t    count) {
    memmove(to, from, count * 8);
  }
};
