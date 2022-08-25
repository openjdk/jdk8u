/*
 * Copyright (c) 1997, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2014, 2019, Red Hat Inc. All rights reserved.
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

#ifndef CPU_RISCV64_VM_ASSEMBLER_RISCV64_HPP
#define CPU_RISCV64_VM_ASSEMBLER_RISCV64_HPP

#include "asm/register.hpp"
#include "assembler_riscv64.inline.hpp"

#define registerSize 64

// definitions of various symbolic names for machine registers

// First intercalls between C and Java which use 8 general registers
// and 8 floating registers

class Argument {
 public:
  enum {
    n_int_register_parameters_c   = 8,  // x10, x11, ... x17 (c_rarg0, c_rarg1, ...)
    n_float_register_parameters_c = 8,  // f10, f11, ... f17 (c_farg0, c_farg1, ... )

    n_int_register_parameters_j   = 8, // x11, ... x17, x10 (rj_rarg0, j_rarg1, ...)
    n_float_register_parameters_j = 8  // f10, f11, ... f17 (j_farg0, j_farg1, ...)
  };
};

// function argument(caller-save registers)
REGISTER_DECLARATION(Register, c_rarg0, x10);
REGISTER_DECLARATION(Register, c_rarg1, x11);
REGISTER_DECLARATION(Register, c_rarg2, x12);
REGISTER_DECLARATION(Register, c_rarg3, x13);
REGISTER_DECLARATION(Register, c_rarg4, x14);
REGISTER_DECLARATION(Register, c_rarg5, x15);
REGISTER_DECLARATION(Register, c_rarg6, x16);
REGISTER_DECLARATION(Register, c_rarg7, x17);

REGISTER_DECLARATION(FloatRegister, c_farg0, f10);
REGISTER_DECLARATION(FloatRegister, c_farg1, f11);
REGISTER_DECLARATION(FloatRegister, c_farg2, f12);
REGISTER_DECLARATION(FloatRegister, c_farg3, f13);
REGISTER_DECLARATION(FloatRegister, c_farg4, f14);
REGISTER_DECLARATION(FloatRegister, c_farg5, f15);
REGISTER_DECLARATION(FloatRegister, c_farg6, f16);
REGISTER_DECLARATION(FloatRegister, c_farg7, f17);

// java function register(caller-save registers)
REGISTER_DECLARATION(Register, j_rarg0, c_rarg1);
REGISTER_DECLARATION(Register, j_rarg1, c_rarg2);
REGISTER_DECLARATION(Register, j_rarg2, c_rarg3);
REGISTER_DECLARATION(Register, j_rarg3, c_rarg4);
REGISTER_DECLARATION(Register, j_rarg4, c_rarg5);
REGISTER_DECLARATION(Register, j_rarg5, c_rarg6);
REGISTER_DECLARATION(Register, j_rarg6, c_rarg7);
REGISTER_DECLARATION(Register, j_rarg7, c_rarg0);

REGISTER_DECLARATION(FloatRegister, j_farg0, f10);
REGISTER_DECLARATION(FloatRegister, j_farg1, f11);
REGISTER_DECLARATION(FloatRegister, j_farg2, f12);
REGISTER_DECLARATION(FloatRegister, j_farg3, f13);
REGISTER_DECLARATION(FloatRegister, j_farg4, f14);
REGISTER_DECLARATION(FloatRegister, j_farg5, f15);
REGISTER_DECLARATION(FloatRegister, j_farg6, f16);
REGISTER_DECLARATION(FloatRegister, j_farg7, f17);

// zero rigster
REGISTER_DECLARATION(Register, zr,        x0);
// global pointer
REGISTER_DECLARATION(Register, gp,        x3);
// thread pointer
REGISTER_DECLARATION(Register, tp,        x4);

// volatile (caller-save) registers

// current method -- must be in a call-clobbered register
REGISTER_DECLARATION(Register, xmethod,   x31);
// return address
REGISTER_DECLARATION(Register, ra,        x1);
// link rigster
REGISTER_DECLARATION(Register, lr,        x1);


// non-volatile (callee-save) registers

// stack pointer
REGISTER_DECLARATION(Register, sp,        x2);
// frame pointer
REGISTER_DECLARATION(Register, fp,        x8);
// base of heap
REGISTER_DECLARATION(Register, xheapbase, x27);
// constant pool cache
REGISTER_DECLARATION(Register, xcpool,    x26);
// monitors allocated on stack
REGISTER_DECLARATION(Register, xmonitors, x25);
// locals on stack
REGISTER_DECLARATION(Register, xlocals,   x24);

/* If you use x4(tp) as java thread pointer according to the instruction manual,
 * it overlaps with the register used by c++ thread.
 */
// java thread pointer
REGISTER_DECLARATION(Register, xthread,   x23);
// bytecode pointer
REGISTER_DECLARATION(Register, xbcp,      x22);
// Dispatch table base
REGISTER_DECLARATION(Register, xdispatch, x21);
// Java stack pointer
REGISTER_DECLARATION(Register, esp,       x20);

// tempory register(caller-save registers)
REGISTER_DECLARATION(Register, t0, x5);
REGISTER_DECLARATION(Register, t1, x6);
REGISTER_DECLARATION(Register, t2, x7);

const Register g_INTArgReg[Argument::n_int_register_parameters_c] = {
  c_rarg0, c_rarg1, c_rarg2, c_rarg3, c_rarg4, c_rarg5,  c_rarg6,  c_rarg7
};

const FloatRegister g_FPArgReg[Argument::n_float_register_parameters_c] = {
  c_farg0, c_farg1, c_farg2, c_farg3, c_farg4, c_farg5, c_farg6, c_farg7
};

#define assert_cond(ARG1) assert(ARG1, #ARG1)

// Addressing modes
class Address {
 public:

  enum mode { no_mode, base_plus_offset, pcrel, literal };

 private:
  Register _base;
  int64_t _offset;
  enum mode _mode;

  RelocationHolder _rspec;

  // If the target is far we'll need to load the ea of this to a
  // register to reach it. Otherwise if near we can do PC-relative
  // addressing.
  address          _target;

 public:
  Address()
    : _base(noreg), _offset(0), _mode(no_mode),          _target(NULL) { }
  Address(Register r)
    : _base(r),     _offset(0), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, int o)
    : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, long o)
    : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, long long o)
     : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, unsigned int o)
     : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, unsigned long o)
    : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
  Address(Register r, unsigned long long o)
    : _base(r),     _offset(o), _mode(base_plus_offset), _target(NULL) { }
#ifdef ASSERT
  Address(Register r, ByteSize disp)
    : _base(r), _offset(in_bytes(disp)), _mode(base_plus_offset), _target(NULL) { }
#endif
  Address(address target, RelocationHolder const& rspec)
    : _base(noreg),
      _offset(0),
      _mode(literal),
      _rspec(rspec),
      _target(target)  { }
  Address(address target, relocInfo::relocType rtype = relocInfo::external_word_type);

  const Register base() const {
    guarantee((_mode == base_plus_offset | _mode == pcrel | _mode == literal), "wrong mode");
    return _base;
  }
  long offset() const {
    return _offset;
  }

  mode getMode() const {
    return _mode;
  }

  bool uses(Register reg) const { return _base == reg;}
  const address target() const { return _target; }
  const RelocationHolder& rspec() const { return _rspec; }
  ~Address() {
    _target = NULL;
    _base = NULL;
  }
};

// Convience classes
class RuntimeAddress: public Address {

  public:

  RuntimeAddress(address target) : Address(target, relocInfo::runtime_call_type) {}
  ~RuntimeAddress() {}
};

class OopAddress: public Address {

  public:

  OopAddress(address target) : Address(target, relocInfo::oop_type) {}
  ~OopAddress() {}
};

class ExternalAddress: public Address {
 private:
  static relocInfo::relocType reloc_for_target(address target) {
    // Sometimes ExternalAddress is used for values which aren't
    // exactly addresses, like the card table base.
    // external_word_type can't be used for values in the first page
    // so just skip the reloc in that case.
    return external_word_Relocation::can_be_relocated(target) ? relocInfo::external_word_type : relocInfo::none;
  }

 public:

  ExternalAddress(address target) : Address(target, reloc_for_target(target)) {}
  ~ExternalAddress() {}
};

class InternalAddress: public Address {

  public:

  InternalAddress(address target) : Address(target, relocInfo::internal_word_type) {}
  ~InternalAddress() {}
};

const int FPUStateSizeInWords = 32 * 2;

class Assembler : public AbstractAssembler {
public:

  enum { instruction_size = 4 };

  enum RoundingMode {
    rne = 0b000,     // round to Nearest, ties to Even
    rtz = 0b001,     // round towards Zero
    rdn = 0b010,     // round Down (towards eegative infinity)
    rup = 0b011,     // round Up (towards infinity)
    rmm = 0b100,     // round to Nearest, ties to Max Magnitude
    rdy = 0b111,     // in instruction's rm field, selects dynamic rounding mode.In Rounding Mode register, Invalid.
  };

  void baseOffset32(Register temp, const Address &adr, int32_t &offset) {
    assert(temp != noreg, "temp must not be empty register!");
    guarantee(adr.base() != temp, "should use different registers!");
    if (is_offset_in_range(adr.offset(), 32)) {
      int32_t imm = adr.offset();
      int32_t upper = imm, lower = imm;
      lower = (imm << 20) >> 20;
      upper -= lower;
      lui(temp, upper);
      offset = lower;
    } else {
      movptr_with_offset(temp, (address)(uintptr_t)adr.offset(), offset);
    }
    add(temp, temp, adr.base());
  }

  void baseOffset(Register temp, const Address &adr, int32_t &offset) {
    if (is_offset_in_range(adr.offset(), 12)) {
      assert(temp != noreg, "temp must not be empty register!");
      addi(temp, adr.base(), adr.offset());
      offset = 0;
    } else {
      baseOffset32(temp, adr, offset);
    }
  }

  void li(Register Rd, int64_t imm);  // optimized load immediate
  void li32(Register Rd, int32_t imm);
  void li64(Register Rd, int64_t imm);
  void movptr(Register Rd, address addr);
  void movptr_with_offset(Register Rd, address addr, int32_t &offset);
  void movptr(Register Rd, uintptr_t imm64);
  void ifence();
  void j(const address &dest, Register temp = t0);
  void j(const Address &adr, Register temp = t0) ;
  void j(Label &l, Register temp = t0);
  void jal(Label &l, Register temp = t0);
  void jal(const address &dest, Register temp = t0);
  void jal(const Address &adr, Register temp = t0);
  void jr(Register Rs);
  void jalr(Register Rs);
  void ret();
  void call(const address &dest, Register temp = t0);
  void call(const Address &adr, Register temp = t0);
  void tail(const address &dest, Register temp = t0);
  void tail(const Address &adr, Register temp = t0);
  void call(Label &l, Register temp) {
    call(target(l), temp);
  }
  void tail(Label &l, Register temp) {
    tail(target(l), temp);
  }

  static inline uint32_t extract(uint32_t val, unsigned msb, unsigned lsb) {
    assert_cond(msb >= lsb && msb <= 31);
    unsigned nbits = msb - lsb + 1;
    uint32_t mask = (1U << nbits) - 1;
    uint32_t result = val >> lsb;
    result &= mask;
    return result;
  }

  static inline int32_t sextract(uint32_t val, unsigned msb, unsigned lsb) {
    assert_cond(msb >= lsb && msb <= 31);
    int32_t result = val << (31 - msb);
    result >>= (31 - msb + lsb);
    return result;
  }

  static void patch(address a, unsigned msb, unsigned lsb, unsigned val) {
    assert_cond(a != NULL);
    assert_cond(msb >= lsb && msb <= 31);
    unsigned nbits = msb - lsb + 1;
    guarantee(val < (1U << nbits), "Field too big for insn");
    unsigned mask = (1U << nbits) - 1;
    val <<= lsb;
    mask <<= lsb;
    unsigned target = *(unsigned *)a;
    target &= ~mask;
    target |= val;
    *(unsigned *)a = target;
  }

  static void patch(address a, unsigned bit, unsigned val) {
    patch(a, bit, bit, val);
  }

  static void patch_reg(address a, unsigned lsb, Register reg) {
    patch(a, lsb + 4, lsb, reg->encoding_nocheck());
  }

  static void patch_reg(address a, unsigned lsb, FloatRegister reg) {
    patch(a, lsb + 4, lsb, reg->encoding_nocheck());
  }

  void emit(unsigned insn) {
    emit_int32((jint)insn);
  }

  void halt() {
    emit_int32(0);
  }

// Rigster Instruction
#define INSN(NAME, op, funct3, funct7)                          \
  void NAME(Register Rd, Register Rs1, Register Rs2) {          \
    unsigned insn = 0;                                          \
    patch((address)&insn, 6,  0, op);                           \
    patch((address)&insn, 14, 12, funct3);                      \
    patch((address)&insn, 31, 25, funct7);                      \
    patch_reg((address)&insn, 7, Rd);                           \
    patch_reg((address)&insn, 15, Rs1);                         \
    patch_reg((address)&insn, 20, Rs2);                         \
    emit(insn);                                                 \
  }

  INSN(add,   0b0110011, 0b000, 0b0000000);
  INSN(sub,   0b0110011, 0b000, 0b0100000);
  INSN(andr,  0b0110011, 0b111, 0b0000000);
  INSN(orr,   0b0110011, 0b110, 0b0000000);
  INSN(xorr,  0b0110011, 0b100, 0b0000000);
  INSN(sll,   0b0110011, 0b001, 0b0000000);
  INSN(sra,   0b0110011, 0b101, 0b0100000);
  INSN(srl,   0b0110011, 0b101, 0b0000000);
  INSN(slt,   0b0110011, 0b010, 0b0000000);
  INSN(sltu,  0b0110011, 0b011, 0b0000000);
  INSN(addw,  0b0111011, 0b000, 0b0000000);
  INSN(subw,  0b0111011, 0b000, 0b0100000);
  INSN(sllw,  0b0111011, 0b001, 0b0000000);
  INSN(sraw,  0b0111011, 0b101, 0b0100000);
  INSN(srlw,  0b0111011, 0b101, 0b0000000);
  INSN(mul,   0b0110011, 0b000, 0b0000001);
  INSN(mulh,  0b0110011, 0b001, 0b0000001);
  INSN(mulhsu,0b0110011, 0b010, 0b0000001);
  INSN(mulhu, 0b0110011, 0b011, 0b0000001);
  INSN(mulw,  0b0111011, 0b000, 0b0000001);
  INSN(div,   0b0110011, 0b100, 0b0000001);
  INSN(divu,  0b0110011, 0b101, 0b0000001);
  INSN(divw,  0b0111011, 0b100, 0b0000001);
  INSN(divuw, 0b0111011, 0b101, 0b0000001);
  INSN(rem,   0b0110011, 0b110, 0b0000001);
  INSN(remu,  0b0110011, 0b111, 0b0000001);
  INSN(remw,  0b0111011, 0b110, 0b0000001);
  INSN(remuw, 0b0111011, 0b111, 0b0000001);

#undef INSN

#define INSN_ENTRY_RELOC(result_type, header)                               \
  result_type header {                                                      \
    InstructionMark im(this);                                               \
    guarantee(rtype == relocInfo::internal_word_type,                       \
              "only internal_word_type relocs make sense here");            \
    code_section()->relocate(inst_mark(), InternalAddress(dest).rspec());

  // Load/store register (all modes)
#define INSN(NAME, op, funct3)                                                                     \
  void NAME(Register Rd, Register Rs, const int32_t offset) {                                      \
    unsigned insn = 0;                                                                             \
    guarantee(is_offset_in_range(offset, 12), "offset is invalid.");                               \
    int32_t val = offset & 0xfff;                                                                  \
    patch((address)&insn, 6, 0, op);                                                               \
    patch((address)&insn, 14, 12, funct3);                                                         \
    patch_reg((address)&insn, 15, Rs);                                                             \
    patch_reg((address)&insn, 7, Rd);                                                              \
    patch((address)&insn, 31, 20, val);                                                            \
    emit(insn);                                                                                    \
  }                                                                                                \
  void NAME(Register Rd, address dest) {                                                           \
    assert_cond(dest != NULL);                                                                     \
    int64_t distance = (dest - pc());                                                              \
    if (is_offset_in_range(distance, 32)) {                                                        \
      auipc(Rd, (int32_t)distance + 0x800);                                                        \
      NAME(Rd, Rd, ((int32_t)distance << 20) >> 20);                                               \
    } else {                                                                                       \
      int32_t offset = 0;                                                                          \
      movptr_with_offset(Rd, dest, offset);                                                        \
      NAME(Rd, Rd, offset);                                                                        \
    }                                                                                              \
  }                                                                                                \
  INSN_ENTRY_RELOC(void, NAME(Register Rd, address dest, relocInfo::relocType rtype))              \
    NAME(Rd, dest);                                                                                \
  }                                                                                                \
  void NAME(Register Rd, const Address &adr, Register temp = t0) {                                 \
    switch(adr.getMode()) {                                                                        \
      case Address::literal: {                                                                     \
        code_section()->relocate(pc(), adr.rspec());                                               \
        NAME(Rd, adr.target());                                                                    \
        break;                                                                                     \
      }                                                                                            \
      case Address::base_plus_offset:{                                                             \
        if (is_offset_in_range(adr.offset(), 12)) {                                                \
          NAME(Rd, adr.base(), adr.offset());                                                      \
        } else {                                                                                   \
          int32_t offset = 0;                                                                      \
          if (Rd == adr.base()) {                                                                  \
            baseOffset32(temp, adr, offset);                                                       \
            NAME(Rd, temp, offset);                                                                \
          } else {                                                                                 \
            baseOffset32(Rd, adr, offset);                                                         \
            NAME(Rd, Rd, offset);                                                                  \
          }                                                                                        \
        }                                                                                          \
        break;                                                                                     \
      }                                                                                            \
       default:                                                                                    \
        ShouldNotReachHere();                                                                      \
    }                                                                                              \
  }                                                                                                \
  void NAME(Register Rd, Label &L) {                                                               \
    wrap_label(Rd, L, &Assembler::NAME);                                                           \
  }

  INSN(lb,  0b0000011, 0b000);
  INSN(lbu, 0b0000011, 0b100);
  INSN(ld,  0b0000011, 0b011);
  INSN(lh,  0b0000011, 0b001);
  INSN(lhu, 0b0000011, 0b101);
  INSN(lw,  0b0000011, 0b010);
  INSN(lwu, 0b0000011, 0b110);

#undef INSN

#define INSN(NAME, op, funct3)                                                                     \
  void NAME(FloatRegister Rd, Register Rs, const int32_t offset) {                                 \
    unsigned insn = 0;                                                                             \
    guarantee(is_offset_in_range(offset, 12), "offset is invalid.");                               \
    uint32_t val = offset & 0xfff;                                                                 \
    patch((address)&insn, 6, 0, op);                                                               \
    patch((address)&insn, 14, 12, funct3);                                                         \
    patch_reg((address)&insn, 15, Rs);                                                             \
    patch_reg((address)&insn, 7, Rd);                                                              \
    patch((address)&insn, 31, 20, val);                                                            \
    emit(insn);                                                                                    \
  }                                                                                                \
  void NAME(FloatRegister Rd, address dest, Register temp = t0) {                                  \
    assert_cond(dest != NULL);                                                                     \
    int64_t distance = (dest - pc());                                                              \
    if (is_offset_in_range(distance, 32)) {                                                        \
      auipc(temp, (int32_t)distance + 0x800);                                                      \
      NAME(Rd, temp, ((int32_t)distance << 20) >> 20);                                             \
    } else {                                                                                       \
      int32_t offset = 0;                                                                          \
      movptr_with_offset(temp, dest, offset);                                                      \
      NAME(Rd, temp, offset);                                                                      \
    }                                                                                              \
  }                                                                                                \
  INSN_ENTRY_RELOC(void, NAME(FloatRegister Rd, address dest, relocInfo::relocType rtype, Register temp = t0)) \
    NAME(Rd, dest, temp);                                                                          \
  }                                                                                                \
  void NAME(FloatRegister Rd, const Address &adr, Register temp = t0) {                            \
    switch(adr.getMode()) {                                                                        \
      case Address::literal: {                                                                     \
        code_section()->relocate(pc(), adr.rspec());                                               \
        NAME(Rd, adr.target(), temp);                                                              \
        break;                                                                                     \
      }                                                                                            \
      case Address::base_plus_offset:{                                                             \
        if (is_offset_in_range(adr.offset(), 12)) {                                                \
          NAME(Rd, adr.base(), adr.offset());                                                      \
        } else {                                                                                   \
          int32_t offset = 0;                                                                      \
          baseOffset32(temp, adr, offset);                                                         \
          NAME(Rd, temp, offset);                                                                  \
        }                                                                                          \
        break;                                                                                     \
      }                                                                                            \
       default:                                                                                    \
        ShouldNotReachHere();                                                                      \
    }                                                                                              \
  }

  INSN(flw, 0b0000111, 0b010);
  INSN(fld, 0b0000111, 0b011);
#undef INSN

#define INSN(NAME, op, funct3)                                                                           \
  void NAME(Register Rs1, Register Rs2, const int64_t offset) {                                          \
    unsigned insn = 0;                                                                                   \
    guarantee(is_imm_in_range(offset, 12, 1), "offset is invalid.");                                     \
    uint32_t val  = offset & 0x1fff;                                                                     \
    uint32_t val11 = (val >> 11) & 0x1;                                                                  \
    uint32_t val12 = (val >> 12) & 0x1;                                                                  \
    uint32_t low  = (val >> 1) & 0xf;                                                                    \
    uint32_t high = (val >> 5) & 0x3f;                                                                   \
    patch((address)&insn, 6, 0, op);                                                                     \
    patch((address)&insn, 14, 12, funct3);                                                               \
    patch_reg((address)&insn, 15, Rs1);                                                                  \
    patch_reg((address)&insn, 20, Rs2);                                                                  \
    patch((address)&insn, 7, val11);                                                                     \
    patch((address)&insn, 11, 8, low);                                                                   \
    patch((address)&insn, 30, 25, high);                                                                 \
    patch((address)&insn, 31, val12);                                                                    \
    emit(insn);                                                                                          \
  }                                                                                                      \
  void NAME(Register Rs1, Register Rs2, const address dest) {                                            \
    assert_cond(dest != NULL);                                                                           \
    int64_t offset = (dest - pc());                                                                      \
    guarantee(is_imm_in_range(offset, 12, 1), "offset is invalid.");                                     \
    NAME(Rs1, Rs2, offset);                                                                              \
  }                                                                                                      \
  INSN_ENTRY_RELOC(void, NAME(Register Rs1, Register Rs2, address dest, relocInfo::relocType rtype))     \
    NAME(Rs1, Rs2, dest);                                                                                \
  }

  INSN(beq,  0b1100011, 0b000);
  INSN(bge,  0b1100011, 0b101);
  INSN(bgeu, 0b1100011, 0b111);
  INSN(blt,  0b1100011, 0b100);
  INSN(bltu, 0b1100011, 0b110);
  INSN(bne,  0b1100011, 0b001);

#undef INSN

#define INSN(NAME, NEG_INSN)                                                                \
  void NAME(Register Rs1, Register Rs2, Label &L, bool is_far = false) {                    \
    wrap_label(Rs1, Rs2, L, &Assembler::NAME, &Assembler::NEG_INSN, is_far);                \
  }

  INSN(beq,  bne);
  INSN(bne,  beq);
  INSN(blt,  bge);
  INSN(bge,  blt);
  INSN(bltu, bgeu);
  INSN(bgeu, bltu);

#undef INSN

#define INSN(NAME, REGISTER, op, funct3)                                                                    \
  void NAME(REGISTER Rs1, Register Rs2, const int32_t offset) {                                             \
    unsigned insn = 0;                                                                                      \
    guarantee(is_offset_in_range(offset, 12), "offset is invalid.");                                        \
    uint32_t val  = offset & 0xfff;                                                                         \
    uint32_t low  = val & 0x1f;                                                                             \
    uint32_t high = (val >> 5) & 0x7f;                                                                      \
    patch((address)&insn, 6, 0, op);                                                                        \
    patch((address)&insn, 14, 12, funct3);                                                                  \
    patch_reg((address)&insn, 15, Rs2);                                                                     \
    patch_reg((address)&insn, 20, Rs1);                                                                     \
    patch((address)&insn, 11, 7, low);                                                                      \
    patch((address)&insn, 31, 25, high);                                                                    \
    emit(insn);                                                                                             \
  }                                                                                                         \
  INSN_ENTRY_RELOC(void, NAME(REGISTER Rs, address dest, relocInfo::relocType rtype, Register temp = t0))   \
    NAME(Rs, dest, temp);                                                                                   \
  }

  INSN(sb,  Register,      0b0100011, 0b000);
  INSN(sh,  Register,      0b0100011, 0b001);
  INSN(sw,  Register,      0b0100011, 0b010);
  INSN(sd,  Register,      0b0100011, 0b011);
  INSN(fsw, FloatRegister, 0b0100111, 0b010);
  INSN(fsd, FloatRegister, 0b0100111, 0b011);

#undef INSN

#define INSN(NAME)                                                                                 \
  void NAME(Register Rs, address dest, Register temp = t0) {                                       \
    assert_cond(dest != NULL);                                                                     \
    assert_different_registers(Rs, temp);                                                          \
    int64_t distance = (dest - pc());                                                              \
    if (is_offset_in_range(distance, 32)) {                                                        \
      auipc(temp, (int32_t)distance + 0x800);                                                      \
      NAME(Rs, temp, ((int32_t)distance << 20) >> 20);                                             \
    } else {                                                                                       \
      int32_t offset = 0;                                                                          \
      movptr_with_offset(temp, dest, offset);                                                      \
      NAME(Rs, temp, offset);                                                                      \
    }                                                                                              \
  }                                                                                                \
  void NAME(Register Rs, const Address &adr, Register temp = t0) {                                 \
    switch(adr.getMode()) {                                                                        \
      case Address::literal: {                                                                     \
        assert_different_registers(Rs, temp);                                                      \
        code_section()->relocate(pc(), adr.rspec());                                               \
        NAME(Rs, adr.target(), temp);                                                              \
        break;                                                                                     \
      }                                                                                            \
      case Address::base_plus_offset:{                                                             \
        if (is_offset_in_range(adr.offset(), 12)) {                                                \
          NAME(Rs, adr.base(), adr.offset());                                                      \
        } else {                                                                                   \
          int32_t offset= 0;                                                                       \
          assert_different_registers(Rs, temp);                                                    \
          baseOffset32(temp, adr, offset);                                                         \
          NAME(Rs, temp, offset);                                                                  \
        }                                                                                          \
        break;                                                                                     \
      }                                                                                            \
       default:                                                                                    \
        ShouldNotReachHere();                                                                      \
    }                                                                                              \
  }

  INSN(sb);
  INSN(sh);
  INSN(sw);
  INSN(sd);

#undef INSN

#define INSN(NAME)                                                                                 \
  void NAME(FloatRegister Rs, address dest, Register temp = t0) {                                  \
    assert_cond(dest != NULL);                                                                     \
    int64_t distance = (dest - pc());                                                              \
    if (is_offset_in_range(distance, 32)) {                                                        \
      auipc(temp, (int32_t)distance + 0x800);                                                      \
      NAME(Rs, temp, ((int32_t)distance << 20) >> 20);                                             \
    } else {                                                                                       \
      int32_t offset = 0;                                                                          \
      movptr_with_offset(temp, dest, offset);                                                      \
      NAME(Rs, temp, offset);                                                                      \
    }                                                                                              \
  }                                                                                                \
  void NAME(FloatRegister Rs, const Address &adr, Register temp = t0) {                            \
    switch(adr.getMode()) {                                                                        \
      case Address::literal: {                                                                     \
        code_section()->relocate(pc(), adr.rspec());                                               \
        NAME(Rs, adr.target(), temp);                                                              \
        break;                                                                                     \
      }                                                                                            \
      case Address::base_plus_offset:{                                                             \
        if (is_offset_in_range(adr.offset(), 12)) {                                                \
          NAME(Rs, adr.base(), adr.offset());                                                      \
        } else {                                                                                   \
          int32_t offset = 0;                                                                      \
          baseOffset32(temp, adr, offset);                                                         \
          NAME(Rs, temp, offset);                                                                  \
        }                                                                                          \
        break;                                                                                     \
      }                                                                                            \
       default:                                                                                    \
        ShouldNotReachHere();                                                                      \
    }                                                                                              \
  }

  INSN(fsw);
  INSN(fsd);

#undef INSN

#define INSN(NAME, op, funct3)                                                        \
  void NAME(Register Rd, const uint32_t csr, Register Rs1) {                          \
    guarantee(is_unsigned_imm_in_range(csr, 12, 0), "csr is invalid");                \
    unsigned insn = 0;                                                                \
    patch((address)&insn, 6, 0, op);                                                  \
    patch((address)&insn, 14, 12, funct3);                                            \
    patch_reg((address)&insn, 7, Rd);                                                 \
    patch_reg((address)&insn, 15, Rs1);                                               \
    patch((address)&insn, 31, 20, csr);                                               \
    emit(insn);                                                                       \
  }

  INSN(csrrw, 0b1110011, 0b001);
  INSN(csrrs, 0b1110011, 0b010);
  INSN(csrrc, 0b1110011, 0b011);

#undef INSN

#define INSN(NAME, op, funct3)                                                        \
  void NAME(Register Rd, const uint32_t csr, const uint32_t uimm) {                   \
    guarantee(is_unsigned_imm_in_range(csr, 12, 0), "csr is invalid");                \
    guarantee(is_unsigned_imm_in_range(uimm, 5, 0), "uimm is invalid");               \
    unsigned insn = 0;                                                                \
    uint32_t val  = uimm & 0x1f;                                                      \
    patch((address)&insn, 6, 0, op);                                                  \
    patch((address)&insn, 14, 12, funct3);                                            \
    patch_reg((address)&insn, 7, Rd);                                                 \
    patch((address)&insn, 19, 15, val);                                               \
    patch((address)&insn, 31, 20, csr);                                               \
    emit(insn);                                                                       \
  }

  INSN(csrrwi, 0b1110011, 0b101);
  INSN(csrrsi, 0b1110011, 0b110);
  INSN(csrrci, 0b1110011, 0b111);

#undef INSN

#define INSN(NAME, op)                                                                        \
  void NAME(Register Rd, const int32_t offset) {                                              \
    unsigned insn = 0;                                                                        \
    guarantee(is_imm_in_range(offset, 20, 1), "offset is invalid.");                          \
    patch((address)&insn, 6, 0, op);                                                          \
    patch_reg((address)&insn, 7, Rd);                                                         \
    patch((address)&insn, 19, 12, (uint32_t)((offset >> 12) & 0xff));                         \
    patch((address)&insn, 20, (uint32_t)((offset >> 11) & 0x1));                              \
    patch((address)&insn, 30, 21, (uint32_t)((offset >> 1) & 0x3ff));                         \
    patch((address)&insn, 31, (uint32_t)((offset >> 20) & 0x1));                              \
    emit(insn);                                                                               \
  }                                                                                           \
  void NAME(Register Rd, const address dest, Register temp = t0) {                            \
    assert_cond(dest != NULL);                                                                \
    int64_t offset = dest - pc();                                                             \
    if (is_imm_in_range(offset, 20, 1)) {                                                     \
      NAME(Rd, offset);                                                                       \
    } else {                                                                                  \
      assert_different_registers(Rd, temp);                                                   \
      int32_t off = 0;                                                                        \
      movptr_with_offset(temp, dest, off);                                                    \
      jalr(Rd, temp, off);                                                                    \
    }                                                                                         \
  }                                                                                           \
  void NAME(Register Rd, Label &L, Register temp = t0) {                                      \
    assert_different_registers(Rd, temp);                                                     \
    wrap_label(Rd, L, temp, &Assembler::NAME);                                                \
  }

  INSN(jal, 0b1101111);

#undef INSN

#undef INSN_ENTRY_RELOC

#define INSN(NAME, op, funct)                                                              \
  void NAME(Register Rd, Register Rs, const int32_t offset) {                              \
    unsigned insn = 0;                                                                     \
    guarantee(is_offset_in_range(offset, 12), "offset is invalid.");                       \
    patch((address)&insn, 6, 0, op);                                                       \
    patch_reg((address)&insn, 7, Rd);                                                      \
    patch((address)&insn, 14, 12, funct);                                                  \
    patch_reg((address)&insn, 15, Rs);                                                     \
    int32_t val = offset & 0xfff;                                                          \
    patch((address)&insn, 31, 20, val);                                                    \
    emit(insn);                                                                            \
  }

  INSN(jalr, 0b1100111, 0b000);

#undef INSN

  enum barrier {
    i = 0b1000, o = 0b0100, r = 0b0010, w = 0b0001,
    ir = i | r, ow = o | w, iorw = i | o | r | w
  };

  void fence(const uint32_t predecessor, const uint32_t successor) {
    unsigned insn = 0;
    guarantee(predecessor < 16, "predecessor is invalid");
    guarantee(successor < 16, "successor is invalid");
    patch((address)&insn, 6, 0, 0b001111);
    patch((address)&insn, 11, 7, 0b00000);
    patch((address)&insn, 14, 12, 0b000);
    patch((address)&insn, 19, 15, 0b00000);
    patch((address)&insn, 23, 20, successor);
    patch((address)&insn, 27, 24, predecessor);
    patch((address)&insn, 31, 28, 0b0000);
    emit(insn);
  }

#define INSN(NAME, op, funct3, funct7)                      \
  void NAME() {                                             \
    unsigned insn = 0;                                      \
    patch((address)&insn, 6, 0, op);                        \
    patch((address)&insn, 11, 7, 0b00000);                  \
    patch((address)&insn, 14, 12, funct3);                  \
    patch((address)&insn, 19, 15, 0b00000);                 \
    patch((address)&insn, 31, 20, funct7);                  \
    emit(insn);                                             \
  }

  INSN(fence_i, 0b0001111, 0b001, 0b000000000000);
  INSN(ecall,   0b1110011, 0b000, 0b000000000000);
  INSN(ebreak,  0b1110011, 0b000, 0b000000000001);
#undef INSN

enum Aqrl {relaxed = 0b00, rl = 0b01, aq = 0b10, aqrl = 0b11};

#define INSN(NAME, op, funct3, funct7)                                                  \
  void NAME(Register Rd, Register Rs1, Register Rs2, Aqrl memory_order = aqrl) {        \
    unsigned insn = 0;                                                                  \
    patch((address)&insn, 6, 0, op);                                                    \
    patch((address)&insn, 14, 12, funct3);                                              \
    patch_reg((address)&insn, 7, Rd);                                                   \
    patch_reg((address)&insn, 15, Rs1);                                                 \
    patch_reg((address)&insn, 20, Rs2);                                                 \
    patch((address)&insn, 31, 27, funct7);                                              \
    patch((address)&insn, 26, 25, memory_order);                                        \
    emit(insn);                                                                         \
  }

  INSN(amoswap_w, 0b0101111, 0b010, 0b00001);
  INSN(amoadd_w,  0b0101111, 0b010, 0b00000);
  INSN(amoxor_w,  0b0101111, 0b010, 0b00100);
  INSN(amoand_w,  0b0101111, 0b010, 0b01100);
  INSN(amoor_w,   0b0101111, 0b010, 0b01000);
  INSN(amomin_w,  0b0101111, 0b010, 0b10000);
  INSN(amomax_w,  0b0101111, 0b010, 0b10100);
  INSN(amominu_w, 0b0101111, 0b010, 0b11000);
  INSN(amomaxu_w, 0b0101111, 0b010, 0b11100);
  INSN(amoswap_d, 0b0101111, 0b011, 0b00001);
  INSN(amoadd_d,  0b0101111, 0b011, 0b00000);
  INSN(amoxor_d,  0b0101111, 0b011, 0b00100);
  INSN(amoand_d,  0b0101111, 0b011, 0b01100);
  INSN(amoor_d,   0b0101111, 0b011, 0b01000);
  INSN(amomin_d,  0b0101111, 0b011, 0b10000);
  INSN(amomax_d , 0b0101111, 0b011, 0b10100);
  INSN(amominu_d, 0b0101111, 0b011, 0b11000);
  INSN(amomaxu_d, 0b0101111, 0b011, 0b11100);
#undef INSN

enum operand_size { int8, int16, int32, uint32, int64 };

#define INSN(NAME, op, funct3, funct7)                                              \
  void NAME(Register Rd, Register Rs1, Aqrl memory_order = relaxed) {               \
    unsigned insn = 0;                                                              \
    uint32_t val = memory_order & 0x3;                                              \
    patch((address)&insn, 6, 0, op);                                                \
    patch((address)&insn, 14, 12, funct3);                                          \
    patch_reg((address)&insn, 7, Rd);                                               \
    patch_reg((address)&insn, 15, Rs1);                                             \
    patch((address)&insn, 25, 20, 0b00000);                                         \
    patch((address)&insn, 31, 27, funct7);                                          \
    patch((address)&insn, 26, 25, val);                                             \
    emit(insn);                                                                     \
  }

  INSN(lr_w, 0b0101111, 0b010, 0b00010);
  INSN(lr_d, 0b0101111, 0b011, 0b00010);

#undef INSN

#define INSN(NAME, op, funct3, funct7)                                                      \
  void NAME(Register Rd, Register Rs1, Register Rs2, Aqrl memory_order = relaxed) {         \
    unsigned insn = 0;                                                                      \
    uint32_t val = memory_order & 0x3;                                                      \
    patch((address)&insn, 6, 0, op);                                                        \
    patch((address)&insn, 14, 12, funct3);                                                  \
    patch_reg((address)&insn, 7, Rd);                                                       \
    patch_reg((address)&insn, 15, Rs2);                                                     \
    patch_reg((address)&insn, 20, Rs1);                                                     \
    patch((address)&insn, 31, 27, funct7);                                                  \
    patch((address)&insn, 26, 25, val);                                                     \
    emit(insn);                                                                             \
  }

  INSN(sc_w, 0b0101111, 0b010, 0b00011);
  INSN(sc_d, 0b0101111, 0b011, 0b00011);
#undef INSN

#define INSN(NAME, op, funct5, funct7)                                                      \
  void NAME(FloatRegister Rd, FloatRegister Rs1, RoundingMode rm = rne) {                   \
    unsigned insn = 0;                                                                      \
    patch((address)&insn, 6, 0, op);                                                        \
    patch((address)&insn, 14, 12, rm);                                                      \
    patch((address)&insn, 24, 20, funct5);                                                  \
    patch((address)&insn, 31, 25, funct7);                                                  \
    patch_reg((address)&insn, 7, Rd);                                                       \
    patch_reg((address)&insn, 15, Rs1);                                                     \
    emit(insn);                                                                             \
  }

  INSN(fsqrt_s,   0b1010011, 0b00000, 0b0101100);
  INSN(fsqrt_d,   0b1010011, 0b00000, 0b0101101);
  INSN(fcvt_s_d,  0b1010011, 0b00001, 0b0100000);
  INSN(fcvt_d_s,  0b1010011, 0b00000, 0b0100001);
#undef INSN

// Immediate Instruction
#define INSN(NAME, op, funct3)                                                              \
  void NAME(Register Rd, Register Rs1, int32_t imm) {                                       \
    guarantee(is_imm_in_range(imm, 12, 0), "Immediate is out of validity");                 \
    unsigned insn = 0;                                                                      \
    patch((address)&insn, 6, 0, op);                                                        \
    patch((address)&insn, 14, 12, funct3);                                                  \
    patch((address)&insn, 31, 20, imm & 0x00000fff);                                        \
    patch_reg((address)&insn, 7, Rd);                                                       \
    patch_reg((address)&insn, 15, Rs1);                                                     \
    emit(insn);                                                                             \
  }

  INSN(addi,  0b0010011, 0b000);
  INSN(slti,  0b0010011, 0b010);
  INSN(addiw, 0b0011011, 0b000);
  INSN(and_imm12,  0b0010011, 0b111);
  INSN(ori,   0b0010011, 0b110);
  INSN(xori,  0b0010011, 0b100);

#undef INSN

#define INSN(NAME, op, funct3)                                                              \
  void NAME(Register Rd, Register Rs1, uint32_t imm) {                                      \
    guarantee(is_unsigned_imm_in_range(imm, 12, 0), "Immediate is out of validity");        \
    unsigned insn = 0;                                                                      \
    patch((address)&insn,6, 0,  op);                                                        \
    patch((address)&insn, 14, 12, funct3);                                                  \
    patch((address)&insn, 31, 20, imm & 0x00000fff);                                        \
    patch_reg((address)&insn, 7, Rd);                                                       \
    patch_reg((address)&insn, 15, Rs1);                                                     \
    emit(insn);                                                                             \
  }

  INSN(sltiu, 0b0010011, 0b011);

#undef INSN

// Shift Immediate Instruction
#define INSN(NAME, op, funct3, funct6)                                   \
  void NAME(Register Rd, Register Rs1, unsigned shamt) {                 \
    guarantee(shamt <= 0x3f, "Shamt is invalid");                        \
    unsigned insn = 0;                                                   \
    patch((address)&insn, 6, 0, op);                                     \
    patch((address)&insn, 14, 12, funct3);                               \
    patch((address)&insn, 25, 20, shamt);                                \
    patch((address)&insn, 31, 26, funct6);                               \
    patch_reg((address)&insn, 7, Rd);                                    \
    patch_reg((address)&insn, 15, Rs1);                                  \
    emit(insn);                                                          \
  }

  INSN(slli,  0b0010011, 0b001, 0b000000);
  INSN(srai,  0b0010011, 0b101, 0b010000);
  INSN(srli,  0b0010011, 0b101, 0b000000);

#undef INSN

// Shift Word Immediate Instruction
#define INSN(NAME, op, funct3, funct7)                                  \
  void NAME(Register Rd, Register Rs1, unsigned shamt) {                \
    guarantee(shamt <= 0x1f, "Shamt is invalid");                       \
    unsigned insn = 0;                                                  \
    patch((address)&insn, 6, 0, op);                                    \
    patch((address)&insn, 14, 12, funct3);                              \
    patch((address)&insn, 24, 20, shamt);                               \
    patch((address)&insn, 31, 25, funct7);                              \
    patch_reg((address)&insn, 7, Rd);                                   \
    patch_reg((address)&insn, 15, Rs1);                                 \
    emit(insn);                                                         \
  }

  INSN(slliw, 0b0011011, 0b001, 0b0000000);
  INSN(sraiw, 0b0011011, 0b101, 0b0100000);
  INSN(srliw, 0b0011011, 0b101, 0b0000000);

#undef INSN

// Upper Immediate Instruction
#define INSN(NAME, op)                                                  \
  void NAME(Register Rd, int32_t imm) {                                 \
    int32_t upperImm = imm >> 12;                                       \
    unsigned insn = 0;                                                  \
    patch((address)&insn, 6, 0, op);                                    \
    patch_reg((address)&insn, 7, Rd);                                   \
    upperImm &= 0x000fffff;                                             \
    patch((address)&insn, 31, 12, upperImm);                            \
    emit(insn);                                                         \
  }

  INSN(lui,   0b0110111);
  INSN(auipc, 0b0010111);

#undef INSN

// Float and Double Rigster Instruction
#define INSN(NAME, op, funct2)                                                                                     \
  void NAME(FloatRegister Rd, FloatRegister Rs1, FloatRegister Rs2, FloatRegister Rs3, RoundingMode rm = rne) {    \
    unsigned insn = 0;                                                                                             \
    patch((address)&insn, 6, 0, op);                                                                               \
    patch((address)&insn, 14, 12, rm);                                                                             \
    patch((address)&insn, 26, 25, funct2);                                                                         \
    patch_reg((address)&insn, 7, Rd);                                                                              \
    patch_reg((address)&insn, 15, Rs1);                                                                            \
    patch_reg((address)&insn, 20, Rs2);                                                                            \
    patch_reg((address)&insn, 27, Rs3);                                                                            \
    emit(insn);                                                                                                    \
  }

  INSN(fmadd_s,   0b1000011,  0b00);
  INSN(fmsub_s,   0b1000111,  0b00);
  INSN(fnmsub_s,  0b1001011,  0b00);
  INSN(fnmadd_s,  0b1001111,  0b00);
  INSN(fmadd_d,   0b1000011,  0b01);
  INSN(fmsub_d,   0b1000111,  0b01);
  INSN(fnmsub_d,  0b1001011,  0b01);
  INSN(fnmadd_d,  0b1001111,  0b01);

#undef INSN

// Float and Double Rigster Instruction
#define INSN(NAME, op, funct3, funct7)                                        \
  void NAME(FloatRegister Rd, FloatRegister Rs1, FloatRegister Rs2) {         \
    unsigned insn = 0;                                                        \
    patch((address)&insn, 6, 0, op);                                          \
    patch((address)&insn, 14, 12, funct3);                                    \
    patch((address)&insn, 31, 25, funct7);                                    \
    patch_reg((address)&insn, 7, Rd);                                         \
    patch_reg((address)&insn, 15, Rs1);                                       \
    patch_reg((address)&insn, 20, Rs2);                                       \
    emit(insn);                                                               \
  }

  INSN(fsgnj_s,  0b1010011, 0b000, 0b0010000);
  INSN(fsgnjn_s, 0b1010011, 0b001, 0b0010000);
  INSN(fsgnjx_s, 0b1010011, 0b010, 0b0010000);
  INSN(fmin_s,   0b1010011, 0b000, 0b0010100);
  INSN(fmax_s,   0b1010011, 0b001, 0b0010100);
  INSN(fsgnj_d,  0b1010011, 0b000, 0b0010001);
  INSN(fsgnjn_d, 0b1010011, 0b001, 0b0010001);
  INSN(fsgnjx_d, 0b1010011, 0b010, 0b0010001);
  INSN(fmin_d,   0b1010011, 0b000, 0b0010101);
  INSN(fmax_d,   0b1010011, 0b001, 0b0010101);

#undef INSN

// Float and Double Rigster Arith Instruction
#define INSN(NAME, op, funct3, funct7)                                    \
  void NAME(Register Rd, FloatRegister Rs1, FloatRegister Rs2) {          \
    unsigned insn = 0;                                                    \
    patch((address)&insn, 6, 0, op);                                      \
    patch((address)&insn, 14, 12, funct3);                                \
    patch((address)&insn, 31, 25, funct7);                                \
    patch_reg((address)&insn, 7, Rd);                                     \
    patch_reg((address)&insn, 15, Rs1);                                   \
    patch_reg((address)&insn, 20, Rs2);                                   \
    emit(insn);                                                           \
  }

  INSN(feq_s,    0b1010011, 0b010, 0b1010000);
  INSN(flt_s,    0b1010011, 0b001, 0b1010000);
  INSN(fle_s,    0b1010011, 0b000, 0b1010000);
  INSN(feq_d,    0b1010011, 0b010, 0b1010001);
  INSN(fle_d,    0b1010011, 0b000, 0b1010001);
  INSN(flt_d,    0b1010011, 0b001, 0b1010001);
#undef INSN

// Float and Double Arith Instruction
#define INSN(NAME, op, funct7)                                                                  \
  void NAME(FloatRegister Rd, FloatRegister Rs1, FloatRegister Rs2, RoundingMode rm = rne) {    \
    unsigned insn = 0;                                                                          \
    patch((address)&insn, 6, 0, op);                                                            \
    patch((address)&insn, 14, 12, rm);                                                          \
    patch((address)&insn, 31, 25, funct7);                                                      \
    patch_reg((address)&insn, 7, Rd);                                                           \
    patch_reg((address)&insn, 15, Rs1);                                                         \
    patch_reg((address)&insn, 20, Rs2);                                                         \
    emit(insn);                                                                                 \
  }

  INSN(fadd_s,   0b1010011, 0b0000000);
  INSN(fsub_s,   0b1010011, 0b0000100);
  INSN(fmul_s,   0b1010011, 0b0001000);
  INSN(fdiv_s,   0b1010011, 0b0001100);
  INSN(fadd_d,   0b1010011, 0b0000001);
  INSN(fsub_d,   0b1010011, 0b0000101);
  INSN(fmul_d,   0b1010011, 0b0001001);
  INSN(fdiv_d,   0b1010011, 0b0001101);

#undef INSN

// Whole Float and Double Conversion Instruction
#define INSN(NAME, op, funct5, funct7)                                  \
  void NAME(FloatRegister Rd, Register Rs1, RoundingMode rm = rne) {    \
    unsigned insn = 0;                                                  \
    patch((address)&insn, 6, 0, op);                                    \
    patch((address)&insn, 14, 12, rm);                                  \
    patch((address)&insn, 24, 20, funct5);                              \
    patch((address)&insn, 31, 25, funct7);                              \
    patch_reg((address)&insn, 7, Rd);                                   \
    patch_reg((address)&insn, 15, Rs1);                                 \
    emit(insn);                                                         \
  }

  INSN(fcvt_s_w,   0b1010011, 0b00000, 0b1101000);
  INSN(fcvt_s_wu,  0b1010011, 0b00001, 0b1101000);
  INSN(fcvt_s_l,   0b1010011, 0b00010, 0b1101000);
  INSN(fcvt_s_lu,  0b1010011, 0b00011, 0b1101000);
  INSN(fcvt_d_w,   0b1010011, 0b00000, 0b1101001);
  INSN(fcvt_d_wu,  0b1010011, 0b00001, 0b1101001);
  INSN(fcvt_d_l,   0b1010011, 0b00010, 0b1101001);
  INSN(fcvt_d_lu,  0b1010011, 0b00011, 0b1101001);

#undef INSN

// Float and Double Conversion Instruction
#define INSN(NAME, op, funct5, funct7)                                  \
  void NAME(Register Rd, FloatRegister Rs1, RoundingMode rm = rtz) {    \
    unsigned insn = 0;                                                  \
    patch((address)&insn, 6, 0, op);                                    \
    patch((address)&insn, 14, 12, rm);                                  \
    patch((address)&insn, 24, 20, funct5);                              \
    patch((address)&insn, 31, 25, funct7);                              \
    patch_reg((address)&insn, 7, Rd);                                   \
    patch_reg((address)&insn, 15, Rs1);                                 \
    emit(insn);                                                         \
  }

  INSN(fcvt_w_s,   0b1010011, 0b00000, 0b1100000);
  INSN(fcvt_l_s,   0b1010011, 0b00010, 0b1100000);
  INSN(fcvt_wu_s,  0b1010011, 0b00001, 0b1100000);
  INSN(fcvt_lu_s,  0b1010011, 0b00011, 0b1100000);
  INSN(fcvt_w_d,   0b1010011, 0b00000, 0b1100001);
  INSN(fcvt_wu_d,  0b1010011, 0b00001, 0b1100001);
  INSN(fcvt_l_d,   0b1010011, 0b00010, 0b1100001);
  INSN(fcvt_lu_d,  0b1010011, 0b00011, 0b1100001);


#undef INSN


// Float and Double Move Instruction
#define INSN(NAME, op, funct3, funct5, funct7)       \
  void NAME(FloatRegister Rd, Register Rs1) {        \
    unsigned insn = 0;                               \
    patch((address)&insn, 6, 0, op);                 \
    patch((address)&insn, 14, 12, funct3);           \
    patch((address)&insn, 20, funct5);               \
    patch((address)&insn, 31, 25, funct7);           \
    patch_reg((address)&insn, 7, Rd);                \
    patch_reg((address)&insn, 15, Rs1);              \
    emit(insn);                                      \
  }

  INSN(fmv_w_x,  0b1010011, 0b000, 0b00000, 0b1111000);
  INSN(fmv_d_x,  0b1010011, 0b000, 0b00000, 0b1111001);

#undef INSN

// Float and Double Conversion Instruction
#define INSN(NAME, op, funct3, funct5, funct7)            \
  void NAME(Register Rd, FloatRegister Rs1) {             \
    unsigned insn = 0;                                    \
    patch((address)&insn, 6, 0, op);                      \
    patch((address)&insn, 14, 12, funct3);                \
    patch((address)&insn, 20, funct5);                    \
    patch((address)&insn, 31, 25, funct7);                \
    patch_reg((address)&insn, 7, Rd);                     \
    patch_reg((address)&insn, 15, Rs1);                   \
    emit(insn);                                           \
  }

  INSN(fclass_s, 0b1010011, 0b001, 0b00000, 0b1110000);
  INSN(fclass_d, 0b1010011, 0b001, 0b00000, 0b1110001);
  INSN(fmv_x_w,  0b1010011, 0b000, 0b00000, 0b1110000);
  INSN(fmv_x_d,  0b1010011, 0b000, 0b00000, 0b1110001);

#undef INSN

  void bgt(Register Rs, Register Rt, const address &dest);
  void ble(Register Rs, Register Rt, const address &dest);
  void bgtu(Register Rs, Register Rt, const address &dest);
  void bleu(Register Rs, Register Rt, const address &dest);
  void bgt(Register Rs, Register Rt, Label &l, bool is_far = false);
  void ble(Register Rs, Register Rt, Label &l, bool is_far = false);
  void bgtu(Register Rs, Register Rt, Label &l, bool is_far = false);
  void bleu(Register Rs, Register Rt, Label &l, bool is_far = false);

  typedef void (Assembler::* jal_jalr_insn)(Register Rt, address dest);
  typedef void (Assembler::* load_insn_by_temp)(Register Rt, address dest, Register temp);
  typedef void (Assembler::* compare_and_branch_insn)(Register Rs1, Register Rs2, const address dest);
  typedef void (Assembler::* compare_and_branch_label_insn)(Register Rs1, Register Rs2, Label &L, bool is_far);

  void wrap_label(Register r1, Register r2, Label &L, compare_and_branch_insn insn,
                  compare_and_branch_label_insn neg_insn, bool is_far);
  void wrap_label(Register r, Label &L, Register t, load_insn_by_temp insn);
  void wrap_label(Register r, Label &L, jal_jalr_insn insn);

  // calculate pseudoinstruction
  void add(Register Rd, Register Rn, int64_t increment, Register temp = t0);
  void addw(Register Rd, Register Rn, int64_t increment, Register temp = t0);
  void sub(Register Rd, Register Rn, int64_t decrement, Register temp = t0);
  void subw(Register Rd, Register Rn, int64_t decrement, Register temp = t0);

  Assembler(CodeBuffer* code) : AbstractAssembler(code) {
  }

  virtual RegisterOrConstant delayed_value_impl(intptr_t* delayed_value_addr,
                                                Register tmp,
                                                int offset) {
    ShouldNotCallThis();
    return RegisterOrConstant();
  }

  // Stack overflow checking
  virtual void bang_stack_with_offset(int offset) { Unimplemented(); }

  static bool operand_valid_for_add_immediate(long imm) {
    return is_imm_in_range(imm, 12, 0);
  }

  // The maximum range of a branch is fixed for the riscv64
  // architecture.
  static const unsigned long branch_range = 1 * M;

  static bool reachable_from_branch_at(address branch, address target) {
    return uabs(target - branch) < branch_range;
  }

  virtual ~Assembler() {}

};

class BiasedLockingCounters;

#endif // CPU_RISCV64_VM_ASSEMBLER_RISCV64_HPP
