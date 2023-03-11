#

ifneq ($(FDLIBM_CFLAGS),)
  OPT_CFLAGS/sharedRuntimeTrig.o = $(OPT_CFLAGS/SPEED) $(FDLIBM_CFLAGS)
  OPT_CFLAGS/sharedRuntimeTrans.o = $(OPT_CFLAGS/SPEED) $(FDLIBM_CFLAGS)
else
  OPT_CFLAGS/sharedRuntimeTrig.o = $(OPT_CFLAGS/NOOPT)
  OPT_CFLAGS/sharedRuntimeTrans.o = $(OPT_CFLAGS/NOOPT)
endif
# Must also specify if CPU is little endian
CFLAGS += -DVM_LITTLE_ENDIAN

# CFLAGS += -D_LP64=1

OPT_CFLAGS/compactingPermGenGen.o = -O1
