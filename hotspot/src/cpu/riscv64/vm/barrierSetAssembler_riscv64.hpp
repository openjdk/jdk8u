/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef CPU_RISCV64_VM_BARRIERSETASSEMBLER_RISCV64_HPP
#define CPU_RISCV64_VM_BARRIERSETASSEMBLER_RISCV64_HPP
#include "asm/macroAssembler.hpp"
#include "memory/allocation.hpp"
#include "register_riscv64.hpp"
#include "assembler_riscv64.hpp"
#include "accessDecorators_riscv64.hpp"
#include "runtime/jniHandles.hpp"
#include "runtime/thread.hpp"
#include "asm/macroAssembler.inline.hpp"


class BarrierSetAssembler: public CHeapObj<mtGC> {
private:
  void incr_allocated_bytes(MacroAssembler* masm,
                            Register var_size_in_bytes, int con_size_in_bytes,
                            Register t1 = noreg);

public:
  virtual void arraycopy_prologue(MacroAssembler* masm, DecoratorSet decorators, bool is_oop,
                                  Register src, Register dst, Register count, RegSet saved_regs) {}
  virtual void arraycopy_epilogue(MacroAssembler* masm, DecoratorSet decorators, bool is_oop,
                                  Register start, Register end, Register tmp, RegSet saved_regs) {}
  virtual void load_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                       Register dst, Address src, Register tmp1, Register tmp_thread);
  virtual void store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                        Address dst, Register val, Register tmp1, Register tmp2);
  virtual void obj_equals(MacroAssembler* masm, Register obj1, Register obj2, Label& equal, bool is_far = false);
  virtual void obj_nequals(MacroAssembler* masm, Register obj1, Register obj2, Label& nequal, bool is_far = false);
  virtual void try_resolve_jobject_in_native(MacroAssembler* masm, Register jni_env,
                                             Register obj, Register tmp, Label& slowpath);

  virtual void tlab_allocate(MacroAssembler* masm,
    Register obj,                      // result: pointer to object after successful allocation
    Register var_size_in_bytes,        // object size in bytes if unknown at compile time; invalid otherwise
    int      con_size_in_bytes,        // object size in bytes if   known at compile time
    Register tmp1,                     // temp register
    Register tmp2,                     // temp register
    Label&   slow_case,                // continuation point if fast allocation fails
    bool is_far = false                // the distance of label slowcase could be more than 12KiB in C1
  );

  void eden_allocate(MacroAssembler* masm,
    Register obj,                      // result: pointer to object after successful allocation
    Register var_size_in_bytes,        // object size in bytes if unknown at compile time; invalid otherwise
    int      con_size_in_bytes,        // object size in bytes if   known at compile time
    Register tmp1,                     // temp register
    Label&   slow_case,                // continuation point if fast allocation fails
    bool is_far = false                // the distance of label slowcase could be more than 12KiB in C1
  );
  virtual void barrier_stubs_init() {}
  virtual ~BarrierSetAssembler() {}
};
class BarrierSetAssembler;
// This class provides the interface between a barrier implementation and
// the rest of the system.

class BarrierSetRv: public CHeapObj<mtGC> {
  friend class VMStructs;

  static BarrierSetRv* _barrier_set;

public:
 /* enum Name {
#define BARRIER_SET_DECLARE_BS_ENUM(bs_name) bs_name ,
    FOR_EACH_BARRIER_SET_DO(BARRIER_SET_DECLARE_BS_ENUM)
#undef BARRIER_SET_DECLARE_BS_ENUM
    UnknownBS
  };
*/
private:

  BarrierSetAssembler* _barrier_set_assembler;

public:

protected:
  BarrierSetRv(BarrierSetAssembler* barrier_set_assembler) :
    _barrier_set_assembler(barrier_set_assembler){}
  ~BarrierSetRv() { }

  template <class BarrierSetAssemblerT>
  static BarrierSetAssembler* make_barrier_set_assembler() {
    return NOT_ZERO(new BarrierSetAssemblerT()) ZERO_ONLY(NULL);
  }


public:
  // Print a description of the memory for the barrier set
  virtual void print_on(outputStream* st) const = 0;

  static BarrierSetRv* barrier_set() { return _barrier_set; }
  static void set_barrier_set(BarrierSetRv* barrier_set);

  BarrierSetAssembler* barrier_set_assembler() {
    assert(_barrier_set_assembler != NULL, "should be set");
    return _barrier_set_assembler;
  }

};

/*
void BarrierSetAssembler::load_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                  Register dst, Address src, Register tmp1, Register tmp_thread) {
  assert_cond(masm != NULL);

  // LR is live. It must be saved around calls.

  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  bool is_not_null = (decorators & IS_NOT_NULL) != 0;
  switch (type) {
  case T_OBJECT:
  case T_ARRAY: {
    if (in_heap) {
      if (UseCompressedOops) {
        __ lwu(dst, src);
        if (is_not_null) {
          __ decode_heap_oop_not_null(dst);
        } else {
          __ decode_heap_oop(dst);
        }
      } else {
        __ ld(dst, src);
      }
    } else {
      assert(in_native, "why else?");
      __ ld(dst, src);
    }
    break;
  }
  case T_BOOLEAN: __ load_unsigned_byte (dst, src); break;
  case T_BYTE:    __ load_signed_byte   (dst, src); break;
  case T_CHAR:    __ load_unsigned_short(dst, src); break;
  case T_SHORT:   __ load_signed_short  (dst, src); break;
  case T_INT:     __ lw                 (dst, src); break;
  case T_LONG:    __ ld                 (dst, src); break;
  case T_ADDRESS: __ ld                 (dst, src); break;
  case T_FLOAT:   __ flw                (f10, src); break;
  case T_DOUBLE:  __ fld                (f10, src); break;
  default: Unimplemented();
  }
}

void BarrierSetAssembler::store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                   Address dst, Register val, Register tmp1, Register tmp2) {
  assert_cond(masm != NULL);
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  switch (type) {
  case T_OBJECT:
  case T_ARRAY: {
    val = val == noreg ? zr : val;
    if (in_heap) {
      if (UseCompressedOops) {
        assert(!dst.uses(val), "not enough registers");
        if (val != zr) {
          __ encode_heap_oop(val);
        }
        __ sw(val, dst);
      } else {
        __ sd(val, dst);
      }
    } else {
      assert(in_native, "why else?");
      __ sd(val, dst);
    }
    break;
  }
  case T_BOOLEAN:
    __ andi(val, val, 0x1);  // boolean is true if LSB is 1
    __ sb(val, dst);
    break;
  case T_BYTE:    __ sb(val, dst); break;
  case T_CHAR:    __ sh(val, dst); break;
  case T_SHORT:   __ sh(val, dst); break;
  case T_INT:     __ sw(val, dst); break;
  case T_LONG:    __ sd(val, dst); break;
  case T_ADDRESS: __ sd(val, dst); break;
  case T_FLOAT:   __ fsw(f10,  dst); break;
  case T_DOUBLE:  __ fsd(f10,  dst); break;
  default: Unimplemented();
  }

}

void BarrierSetAssembler::obj_equals(MacroAssembler* masm, Register obj1, Register obj2, Label& equal, bool is_far) {
  assert_cond(masm != NULL);
  __ beq(obj1, obj2, equal, is_far);
}

void BarrierSetAssembler::obj_nequals(MacroAssembler* masm, Register obj1, Register obj2, Label& nequal, bool is_far) {
  assert_cond(masm != NULL);
  __ bne(obj1, obj2, nequal, is_far);
}

void BarrierSetAssembler::try_resolve_jobject_in_native(MacroAssembler* masm, Register jni_env,
                                                        Register obj, Register tmp, Label& slowpath) {
  assert_cond(masm != NULL);
  // If mask changes we need to ensure that the inverse is still encodable as an immediate
  STATIC_ASSERT(JNIHandles::weak_tag_mask == 1);
  __ andi(obj, obj, ~JNIHandles::weak_tag_mask);
  __ ld(obj, Address(obj, 0));             // *obj
}

// Defines obj, preserves var_size_in_bytes, okay for tmp2 == var_size_in_bytes.
void BarrierSetAssembler::tlab_allocate(MacroAssembler* masm, Register obj,
                                        Register var_size_in_bytes,
                                        int con_size_in_bytes,
                                        Register tmp1,
                                        Register tmp2,
                                        Label& slow_case,
                                        bool is_far) {
  assert_cond(masm != NULL);
  assert_different_registers(obj, tmp2);
  assert_different_registers(obj, var_size_in_bytes);
  Register end = tmp2;

  __ ld(obj, Address(xthread, JavaThread::tlab_top_offset()));
  if (var_size_in_bytes == noreg) {
    __ la(end, Address(obj, con_size_in_bytes));
  } else {
    __ add(end, obj, var_size_in_bytes);
  }
  __ ld(t0, Address(xthread, JavaThread::tlab_end_offset()));
  __ bgtu(end, t0, slow_case, is_far);

  // update the tlab top pointer
  __ sd(end, Address(xthread, JavaThread::tlab_top_offset()));

  // recover var_size_in_bytes if necessary
  if (var_size_in_bytes == end) {
    __ sub(var_size_in_bytes, var_size_in_bytes, obj);
  }
}

// Defines obj, preserves var_size_in_bytes
void BarrierSetAssembler::eden_allocate(MacroAssembler* masm, Register obj,
                                        Register var_size_in_bytes,
                                        int con_size_in_bytes,
                                        Register tmp1,
                                        Label& slow_case,
                                        bool is_far) {
  assert_cond(masm != NULL);
  assert_different_registers(obj, var_size_in_bytes, tmp1);
  if (!Universe::heap()->supports_inline_contig_alloc()) {
    __ j(slow_case);
  } else {
    Register end = tmp1;
    Label retry;
    int32_t offset = 0;
    __ bind(retry);

    // Get the current top of the heap
    ExternalAddress address_top((address) Universe::heap()->top_addr());
    __ la_patchable(t2, address_top, offset);
    __ addi(t2, t2, offset);
    __ lr_d(obj, t2, Assembler::aqrl);

    // Adjust it my the size of our new object
    if (var_size_in_bytes == noreg) {
      __ la(end, Address(obj, con_size_in_bytes));
    } else {
      __ add(end, obj, var_size_in_bytes);
    }

    // if end < obj then we wrapped around high memory
    __ bltu(end, obj, slow_case, is_far);

    Register heap_end = t1;
    // Get the current end of the heap
    ExternalAddress address_end((address) Universe::heap()->end_addr());
    offset = 0;
    __ la_patchable(heap_end, address_end, offset);
    __ ld(heap_end, Address(heap_end, offset));

    __ bgtu(end, heap_end, slow_case, is_far);

    // If heap_top hasn't been changed by some other thread, update it.
    __ sc_d(t1, end, t2, Assembler::rl);
    __ bnez(t1, retry);
    incr_allocated_bytes(masm, var_size_in_bytes, con_size_in_bytes, tmp1);
  }
}

void BarrierSetAssembler::incr_allocated_bytes(MacroAssembler* masm,
                                               Register var_size_in_bytes,
                                               int con_size_in_bytes,
                                               Register tmp1) {
  assert_cond(masm != NULL);
  assert(tmp1->is_valid(), "need temp reg");

  __ ld(tmp1, Address(xthread, in_bytes(JavaThread::allocated_bytes_offset())));
  if (var_size_in_bytes->is_valid()) {
    __ add(tmp1, tmp1, var_size_in_bytes);
  } else {
    __ add(tmp1, tmp1, con_size_in_bytes);
  }
  __ sd(tmp1, Address(xthread, in_bytes(JavaThread::allocated_bytes_offset())));
}*/
#endif // CPU_RISCV64_BARRIERSETASSEMBLER_RISCV64_HPP
