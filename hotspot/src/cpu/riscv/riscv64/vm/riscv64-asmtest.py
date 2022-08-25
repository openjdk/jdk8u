#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random

RISCV64_AS = "<PATH-TO-AS>"
RISCV64_OBJDUMP = "<PATH-TO-OBJDUMP>"
RISCV64_OBJCOPY = "<PATH-TO-OBJCOPY>"

class Operand(object):

     def generate(self):
        return self

class Register(Operand):

    def generate(self):
        self.number = random.randint(1, 31)
        return self

    def astr(self, prefix):
        return prefix + str(self.number)

class FloatRegister(Register):

    def __str__(self):
        return self.astr("f")

    def nextReg(self):
        next = FloatRegister()
        next.number = (self.number + 1) % 32
        return next

class GeneralRegister(Register):

    def __str__(self):
        return self.astr("x")

class GeneralRegisterOrZr(Register):

    def generate(self):
        self.number = random.randint(0, 31)
        return self

    def astr(self, prefix = ""):
        if (self.number == 0):
            return "zr"
        else:
            return prefix + str(self.number)

    def __str__(self):
        if (self.number == 0):
            return self.astr()
        else:
            return self.astr("x")

class GeneralRegisterOrSp(Register):
    def generate(self):
        self.number = random.randint(0, 31)
        return self

    def astr(self, prefix = ""):
        if (self.number == 2):
            return "sp"
        else:
            return prefix + str(self.number)

    def __str__(self):
        if (self.number == 2):
            return self.astr()
        else:
            return self.astr("x")

class OperandFactory:

    _modes = {'x' : GeneralRegister,
              'f' : FloatRegister}

    @classmethod
    def create(cls, mode):
        return OperandFactory._modes[mode]()

class Instruction(object):

    def __init__(self, name):
        self._name = name

    def aname(self):
        return self._name.replace('_', '.')

    def emit(self) :
        pass

    def compare(self) :
        pass

    def generate(self) :
        return self

    def cstr(self):
        return '__ %s(' % self.name()

    def astr(self):
        return '%s\t' % self.aname()

    def name(self):
        name = self._name
        if name == "and":
            name = "andr"
        elif name == "or":
            name = "orr"
        elif name == "not":
            name = "notr"
        elif name == "xor":
            name = "xorr"
        return name

    def multipleForms(self):
         return 0

class ThreeRegInstruction(Instruction):

    def generate(self):
        self.reg = [GeneralRegister().generate(), GeneralRegister().generate(),
                    GeneralRegister().generate()]
        return self


    def cstr(self):
        return (super(ThreeRegInstruction, self).cstr()
                + ('%s, %s, %s'
                   % (self.reg[0],
                      self.reg[1], self.reg[2])))

    def astr(self):
        return (super(ThreeRegInstruction, self).astr()
                + ('%s, %s, %s'
                   % (self.reg[0],
                      self.reg[1], self.reg[2])))

class TwoRegInstruction(Instruction):

    def generate(self):
        self.reg = [GeneralRegister().generate(), GeneralRegister().generate()]
        return self

    def cstr(self):
        return (super(TwoRegInstruction, self).cstr()
                + '%s, %s' % (self.reg[0], self.reg[1]))

    def astr(self):
        return (super(TwoRegInstruction, self).astr()
                + '%s, %s' % (self.reg[0], self.reg[1]))

class TwoRegImmedInstruction(TwoRegInstruction):

    def generate(self):
        super(TwoRegImmedInstruction, self).generate()
        self.immed = random.randint(0, 1<<12 -1)
        return self

    def cstr(self):
        return (super(TwoRegImmedInstruction, self).cstr()
                + ', %su' % self.immed)

    def astr(self):
        return (super(TwoRegImmedInstruction, self).astr()
                + ', %s' % hex(self.immed))

class LoadImmedOp(Instruction):

    def generate(self):
        super(LoadImmedOp, self).generate()
        self.immed = hex(random.randint(0, 1<<20 -1))
        self.reg = GeneralRegister().generate()
        return self

    def cstr(self):
        return (super(LoadImmedOp, self).cstr()
                + '%s, %s000);' % (self.reg, self.immed))

    def astr(self):
        return (super(LoadImmedOp, self).astr()
                + '%s, %s' % (self.reg, self.immed))

class TwoRegImmedOp(TwoRegInstruction):

    def generate(self):
        super(TwoRegImmedOp, self).generate()
        self.immed = random.randint(0, 1<<12 -1)
        return self

    def cstr(self):
        return (super(TwoRegImmedOp, self).cstr()
                + ', %su);' % self.immed)

    def astr(self):
        return (super(TwoRegImmedOp, self).astr()
                + ', %s' % self.immed)

class OneRegOp(Instruction):

    def generate(self):
        self.reg = GeneralRegister().generate()
        return self

    def cstr(self):
        return (super(OneRegOp, self).cstr()
                + '%s);' % self.reg)

    def astr(self):
        return (super(OneRegOp, self).astr()
                + '%s' % self.reg)

class ArithOp(ThreeRegInstruction):

    def generate(self):
        super(ArithOp, self).generate()
        return self

    def cstr(self):
        return ('%s);'
                % ThreeRegInstruction.cstr(self))

    def astr(self):
        return ThreeRegInstruction.astr(self)

class AddSubImmOp(TwoRegImmedInstruction):

    def cstr(self):
         return super(AddSubImmOp, self).cstr() + ");"

class LogicalImmOp(TwoRegImmedInstruction):

     def cstr(self):
         return super(LogicalImmOp, self).cstr() + ");"

class MultiOp():

    def multipleForms(self):
         return 3

    def forms(self):
         return ["__ pc()", "back", "forth"]

    def aforms(self):
         return [".", "back", "forth"]

class AbsOp(MultiOp, Instruction):

    def cstr(self):
        return super(AbsOp, self).cstr() + "%s);"

    def astr(self):
        return Instruction.astr(self) + "%s"

class TwoRegAbsOp(TwoRegImmedOp):

    def generate(self):
        super(TwoRegAbsOp, self).generate()
        return self

class RegAndAbsOp(MultiOp, Instruction):

    def generate(self):
        Instruction.generate(self)
        self.reg = GeneralRegister().generate()
        return self

    def cstr(self):
        return (super(RegAndAbsOp, self).cstr()
                + "%s, %s);" % (self.reg, "%s"))

    def astr(self):
        return "%s  %s, %s" % (self._name, self.reg, "%s")

class TwoRegAndAbsOp(MultiOp, TwoRegInstruction):

    def generate(self):
        super(TwoRegAndAbsOp, self).generate()
        return self

    def cstr(self):
        return (super(TwoRegAndAbsOp, self).cstr()
                + ', %s);' % "%s")

    def astr(self):
        return (super(TwoRegAndAbsOp, self).astr()
                + ', %s' % "%s")

class ShiftRegOp(ThreeRegInstruction):

    def generate(self):
        super(ShiftRegOp, self).generate()
        return self

    def cstr(self):
        return ('%s);'
                % ThreeRegInstruction.cstr(self))

    def astr(self):
        return ThreeRegInstruction.astr(self)

class ShiftImmOp(TwoRegImmedInstruction):

    def generate(self):
        super(ShiftImmOp, self).generate()
        self.immed = random.randint(0, 1<<5 -1)
        return self

    def cstr(self):
        return ('%s);'
                % TwoRegImmedInstruction.cstr(self))

    def astr(self):
        return TwoRegImmedInstruction.astr(self)

class Op(Instruction):

    def cstr(self):
        return Instruction.cstr(self) + ");"

class SystemOp(Instruction):

     def __init__(self, op):
          Instruction.__init__(self, op[0])
          self.barriers = op[1]

     def generate(self):
          Instruction.generate(self)
          self.barrier \
              = [self.barriers[random.randint(0, len(self.barriers)-1)],
                 self.barriers[random.randint(0, len(self.barriers)-1)]]
          return self

     def cstr(self):
          return super(SystemOp, self).cstr() + "%su, %su);" \
                 % (self.barrier[0][0], self.barrier[1][0])

     def astr(self):
          return super(SystemOp, self).astr() + "%s, %s" % (self.barrier[0][1], self.barrier[1][1])

class CsrxixOp(Instruction):

    def generate(self):
        super(CsrxixOp, self).generate()
        self.reg = [GeneralRegister().generate(), GeneralRegister().generate()]
        self.immed = random.randint(0, 1<<12 - 1)
        return self

    def cstr(self):
        return super(CsrxixOp, self).cstr() + "%s, %s, %s);" % (self.reg[0], self.immed, self.reg[1])

    def astr(self):
        return super(CsrxixOp, self).astr() + "%s, %s, %s" % (self.reg[0], hex(self.immed), self.reg[1])

class CsrxiiOp(Instruction):

    def generate(self):
        super(CsrxiiOp, self).generate()
        self.reg = GeneralRegister().generate()
        self.immed = [random.randint(0, 1<<12 - 1), random.randint(0, 1<<5 - 1)]
        return self

    def cstr(self):
        return super(CsrxiiOp, self).cstr() + "%s, %s, %s);" % (self.reg, self.immed[0], self.immed[1])

    def astr(self):
        return super(CsrxiiOp, self).astr() + "%s, %s, %s" % (self.reg, hex(self.immed[0]), self.immed[1])

class CsrxiOp(Instruction):

    def generate(self):
        super(CsrxiOp, self).generate()
        self.reg = GeneralRegister().generate()
        self.immed = hex(random.randint(0, 1<<12 - 1))
        return self

    def cstr(self):
        return super(CsrxiOp, self).cstr() + "%s, %s);" % (self.reg, self.immed)

    def astr(self):
        return super(CsrxiOp, self).astr() + "%s, %s" % (self.reg, self.immed)

class CsriiOp(Instruction):

    def generate(self):
        super(CsriiOp, self).generate()
        self.immed = [hex(random.randint(0, 1<<12 - 1)), random.randint(0, 1<<5 - 1)]
        return self

    def cstr(self):
        return super(CsriiOp, self).cstr() + "%s, %s);" % (self.immed[0], self.immed[1])

    def astr(self):
        return super(CsriiOp, self).astr() + "%s, %s" % (self.immed[0], self.immed[1])

class CsrixOp(Instruction):

    def generate(self):
        super(CsrixOp, self).generate()
        self.reg = GeneralRegister().generate()
        self.immed = hex(random.randint(0, 1<<12 - 1))
        return self

    def cstr(self):
        return super(CsrixOp, self).cstr() + "%s, %s);" % (self.immed, self.reg)

    def astr(self):
        return super(CsrixOp, self).astr() + "%s, %s" % (self.immed, self.reg)

class AtomOp(Instruction):

    def __init__(self, args):
        self._name, self.size, self.suffix = args

    def generate(self):
        self.asmname = "%s.%s.%s" % (self._name, self.size, self.suffix)
        self._name = "%s_%s" % (self._name, self.size)
        self.reg = [GeneralRegister().generate(), GeneralRegister().generate(),
                    GeneralRegister().generate()]
        return self

    def aname(self):
        return self.asmname

    def cstr(self):
        if self._name.startswith("lr"):
            return super(AtomOp, self).cstr() + "%s, %s, Assembler::%s);" \
                   % (self.reg[0], self.reg[1], self.suffix)
        return super(AtomOp, self).cstr() + "%s, %s, %s, Assembler::%s);" \
                   % (self.reg[0], self.reg[1], self.reg[2], self.suffix)

    def astr(self):
        if self._name.startswith("lr"):
            return super(AtomOp, self).astr() + "%s, (%s)" \
                   % (self.reg[0], self.reg[1])
        elif self._name.startswith("sc"):
            return super(AtomOp, self).astr() + "%s, %s, (%s)" \
               % (self.reg[0], self.reg[1], self.reg[2])
        return super(AtomOp, self).astr() + "%s, %s, (%s)" \
               % (self.reg[0], self.reg[2], self.reg[1])

class TwoRegOp(TwoRegInstruction):

    def cstr(self):
        return TwoRegInstruction.cstr(self) + ");"

class ThreeRegOp(ThreeRegInstruction):

    def cstr(self):
        return ThreeRegInstruction.cstr(self) + ");"

class Address(object):

    def generate(self):
        self.base = GeneralRegister().generate()
        self.offset = random.randint(-1<<11, 1<<11-1)
        return self

    def cstr(self):
        result = "Address(%s, %s)" % (self.base.astr("x"), self.offset)
        return result

    def astr(self):
        result = "%s(%s)" % (self.offset, self.base.astr("x"))
        return result

class LoadStoreOp(Instruction):

    def __init__(self, name):
        Instruction.__init__(self, name)

    def generate(self):
        self.adr = Address().generate()
        if (self._name.startswith("f")):
            self.reg = FloatRegister().generate()
        else:
            self.reg = GeneralRegister().generate()
        return self

    def cstr(self):
        return "%s%s, %s);" % (Instruction.cstr(self), self.reg, self.adr.cstr())

    def astr(self):
        return "%s %s, %s" % (Instruction.astr(self), self.reg, self.adr.astr())

class FloatInstruction(Instruction):

    def __init__(self, args):
        if (len(args) == 3):
            name, self.modes, self.roundingModes = args
        else:
            name, self.modes = args
            self.roundingModes = ""
        Instruction.__init__(self, name)

    def generate(self):
        self.reg = [OperandFactory.create(self.modes[i]).generate()
                    for i in range(self.numRegs)]
        return self

    def cstr(self):
        if (self.roundingModes == ""):
            formatStr = "%s%s" + ''.join([", %s" for i in range(1, self.numRegs)] + [");"])
        else:
            formatStr = "%s%s" + ''.join([", %s" for i in range(1, self.numRegs)] + [", Assembler::" + self.roundingModes + ");"])
        return (formatStr
                % tuple([Instruction.cstr(self)] +
                        [str(self.reg[i]) for i in range(self.numRegs)])) # Yowza

    def astr(self):
        name = self._name
        if (self.roundingModes == "") | (name == "fcvt_d_w") | (name == "fcvt_d_wu") | (name == "fcvt_d_s"):
            formatStr = "%s%s" + ''.join([", %s" for i in range(1, self.numRegs)])
        else:
            formatStr = "%s%s" + ''.join([", %s" for i in range(1, self.numRegs)]) + ", " + self.roundingModes
        return (formatStr
                % tuple([Instruction.astr(self)] +
                        [(self.reg[i].astr(self.modes[i])) for i in range(self.numRegs)]))

class TwoRegFloatOp(FloatInstruction):
    numRegs = 2

class ThreeRegFloatOp(TwoRegFloatOp):
    numRegs = 3

class Float2ArithOp(FloatInstruction):
    numRegs = 2

class Float3ArithOp(TwoRegFloatOp):
    numRegs = 3

class Float4ArithOp(TwoRegFloatOp):
    numRegs = 4

class FloatConvertOp(TwoRegFloatOp):
    numRegs = 2

def generate(kind, names):
    outfile.write("# " + kind.__name__ + "\n");
    print "\n// " + kind.__name__
    for name in names:
        for i in range(1):
             op = kind(name).generate()
             if op.multipleForms():
                  forms = op.forms()
                  aforms = op.aforms()
                  for i in range(op.multipleForms()):
                       cstr = op.cstr() % forms[i]
                       astr = op.astr() % aforms[i]
                       print "    %-50s //\t%s" % (cstr, astr)
                       outfile.write("\t%s\n" %(astr))
             else:
                  print "    %-50s //\t%s" %(op.cstr(), op.astr())
                  outfile.write("\t%s\n" %(op.astr()))

with open("riscv64ops.s", "w") as outfile:

    print "// BEGIN  Generated code -- do not edit"
    print "// Generated by riscv64-asmtest.py"

    print "    Label back, forth;"
    print "    __ bind(back);"

    outfile.write("back:\n")

    generate (ArithOp,
              [ "add", "sub", "addw", "subw",
                "or", "xor", "mul", "mulh",
                "mulhsu", "mulhu", "div", "divu",
                "rem", "remu", "mulw", "divw",
                "divuw", "remw", "remuw", "and"])

    generate (AddSubImmOp,
              [ "addi", "addiw"])

    generate (LogicalImmOp,
              [ "ori", "xori", "andi"])

    generate (AbsOp, [ "j", "jal" ])

    generate (TwoRegAbsOp, ["jalr"])

    generate (LoadImmedOp, ["lui", "auipc"])

    generate (RegAndAbsOp, ["bnez", "beqz"])

    generate (TwoRegAndAbsOp, ["bne", "beq", "bge", "bgeu",
                               "blt", "bltu"])

    generate (TwoRegImmedOp, ["slti", "sltiu"])

    generate (ShiftRegOp, ["sll", "srl", "sra", "sraw",
                           "sllw", "srlw"])

    generate (ShiftImmOp, ["slli", "srli", "srai", "slliw",
                           "srliw", "sraiw"])

    generate (Op, ["nop", "ecall", "ebreak", "fence_i"])

    barriers = [["8", "i"], ["4", "o"], ["2", "r"], ["1", "w"],
                ["10", "ir"], ["5", "ow"], ["15", "iorw"]]

    generate (SystemOp, [["fence", barriers]])

    for size in ("w", "d"):
        for suffix in ("aq", "rl"):
            generate (AtomOp, [["sc", size, suffix], ["amoswap", size, suffix],
                               ["amoadd", size, suffix], ["amoxor", size, suffix],
                               ["amoand", size, suffix], ["amoor", size, suffix],
                               ["amomin", size, suffix], ["amomax", size, suffix],
                               ["amominu", size, suffix], ["amomaxu", size, suffix],
                               ["lr", size, suffix]])

    generate (OneRegOp, ["frflags", "frrm",
                         "frcsr", "rdtime", "rdcycle",
                         "rdinstret"])

    generate(TwoRegOp,
             ["mv", "not", "neg", "negw",
              "sext_w", "seqz", "snez", "sltz",
              "sgtz", "fscsr", "fsrm", "fsflags"])

    generate(ThreeRegOp,
             ["slt", "sltu"])

    generate(CsrxixOp, ["csrrw", "csrrs", "csrrc"])

    generate(CsrxiiOp, ["csrrwi", "csrrsi", "csrrci"])

    generate(CsrxiOp, ["csrr"])

    generate(CsrixOp, ["csrw", "csrs", "csrc"])

    generate(CsriiOp, ["csrwi", "csrsi", "csrci"])

    generate(LoadStoreOp, ["ld", "lw", "lwu", "lh",
                           "lhu", "lb", "lbu", "sd",
                           "sw", "sh", "sb", "fld",
                           "flw", "fsd", "fsw"])

    generate (Float2ArithOp, [["fsqrt_s", "ff", "rdn"], ["fsqrt_d", "ff", "rdn"]])

    generate (Float3ArithOp, [["fadd_s", "fff", "rup"], ["fsub_s", "fff", "rup"],
                              ["fadd_d", "fff", "rup"], ["fsub_d", "fff", "rup"],
                              ["fmul_s", "fff", "rup"], ["fdiv_s", "fff", "rup"],
                              ["fmul_d", "fff", "rup"], ["fdiv_d", "fff", "rup"]])

    generate (Float4ArithOp, [["fmadd_s", "ffff", "rup"], ["fmsub_s", "ffff", "rtz"],
                              ["fmadd_d", "ffff", "rup"], ["fmsub_d", "ffff", "rtz"],
                              ["fnmsub_s", "ffff", "rmm"], ["fnmadd_s", "ffff", "rtz"],
                              ["fnmsub_d", "ffff", "rmm"], ["fnmadd_d", "ffff", "rtz"]])

    generate(TwoRegFloatOp, [["fclass_s", "xf"], ["fmv_s", "ff"],
                             ["fclass_d", "xf"], ["fmv_d", "ff"],
                             ["fabs_s", "ff"], ["fneg_s", "ff"],
                             ["fabs_d", "ff"], ["fneg_d", "ff"],
                             ["fmv_x_w", "xf"], ["fmv_x_d", "xf"]])

    generate(ThreeRegFloatOp, [["fsgnj_s", "fff"], ["fsgnjn_s","fff"],
                               ["fsgnj_d", "fff"], ["fsgnjn_d","fff"],
                               ["fsgnjx_s", "fff"], ["fmin_s", "fff"],
                               ["fsgnjx_d", "fff"], ["fmin_d", "fff"],
                               ["fmax_s", "fff"], ["feq_s", "xff"],
                               ["fmax_d", "fff"], ["feq_d", "xff"],
                               ["flt_s", "xff"], ["fle_s", "xff"],
                               ["flt_d", "xff"], ["fle_d", "xff"]])

    generate(FloatConvertOp, [["fcvt_w_s", "xf", "rup"], ["fcvt_wu_s", "xf", "rne"],
                              ["fcvt_s_w", "fx", "rdn"], ["fcvt_s_wu", "fx", "rtz"],
                              ["fcvt_l_s", "xf", "rne"], ["fcvt_lu_s", "xf", "rmm"],
                              ["fcvt_s_l", "fx", "rup"], ["fcvt_s_lu", "fx", "rtz"],
                              ["fcvt_s_d", "ff", "rdn"], ["fcvt_d_s", "ff", "rne"],
                              ["fcvt_w_d", "xf", "rdn"], ["fcvt_wu_d", "xf", "rdn"],
                              ["fcvt_d_w", "fx", "rne"], ["fcvt_d_wu", "fx", "rne"],
                              ["fcvt_l_d", "xf", "rdn"], ["fcvt_lu_d", "xf", "rdn"],
                              ["fcvt_d_l", "fx", "rdn"], ["fcvt_d_lu", "fx", "rdn"]])

    print "\n    __ bind(forth);"
    outfile.write("forth:\n")

import subprocess
import sys

subprocess.check_call([RISCV64_AS, "riscv64ops.s", "-o", "riscv64ops.o"])

print
print "/*",
sys.stdout.flush()
subprocess.check_call([RISCV64_OBJDUMP, "-d", "riscv64ops.o"])
print "*/"

subprocess.check_call([RISCV64_OBJCOPY, "-O", "binary", "-j", ".text", "riscv64ops.o", "riscv64ops.bin"])

with open("riscv64ops.bin", "r") as infile:
    bytes = bytearray(infile.read())

    print
    print "  static const unsigned int insns[] ="
    print "  {"

    i = 0
    while i < len(bytes):
         print "    0x%02x%02x%02x%02x," % (bytes[i+3], bytes[i+2], bytes[i+1], bytes[i]),
         i += 4
         if i%16 == 0:
              print
    print "\n  };"
    print "// END  Generated code -- do not edit"
