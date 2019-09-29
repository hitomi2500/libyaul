ifneq (1,$(words [$(shell pwd)]))
  $(error Current directory ($(shell pwd)) contains spaces)
endif

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif
ifneq (1,$(words [$(strip $(YAUL_INSTALL_ROOT))]))
	$(error YAUL_INSTALL_ROOT (install root directory) contains spaces)
endif
ifneq ($(shell test -e $(YAUL_INSTALL_ROOT) && echo -n $${?}),0)
  $(error Path YAUL_INSTALL_ROOT (install root directory) does not exist)
endif
ifneq ($(shell test -d $(YAUL_INSTALL_ROOT) && echo -n $${?}),0)
  $(error Path YAUL_INSTALL_ROOT (install root directory) is not a directory)
endif

ifeq ($(strip $(YAUL_ARCH_SH_PREFIX)),)
  $(error Undefined YAUL_ARCH_SH_PREFIX (tool-chain prefix))
endif
ifneq (1,$(words [$(strip $(YAUL_ARCH_SH_PREFIX))]))
  $(error YAUL_ARCH_SH_PREFIX (tool-chain prefix) contains spaces)
endif

ifneq (1,$(words [$(strip $(YAUL_PROG_SH_PREFIX))]))
  $(error YAUL_PROG_SH_PREFIX (tool-chain program prefix) contains spaces)
endif

ifeq ($(strip $(YAUL_BUILD_ROOT)),)
  $(error Undefined YAUL_BUILD_ROOT (build root directory))
endif
ifneq (1,$(words [$(strip $(YAUL_BUILD_ROOT))]))
  $(error YAUL_BUILD_ROOT (build root directory) contains spaces)
endif
ifneq ($(shell test -e $(YAUL_BUILD_ROOT) && echo -n $${?}),0)
  $(error Path YAUL_BUILD_ROOT (build root directory) does not exist)
endif
ifneq ($(shell test -d $(YAUL_BUILD_ROOT) && echo -n $${?}),0)
  $(error Path YAUL_BUILD_ROOT (build root directory) is not a directory)
endif

ifeq ($(strip $(YAUL_BUILD)),)
  $(error Undefined YAUL_BUILD (build directory))
endif
ifneq (1,$(words [$(strip $(YAUL_BUILD))]))
  $(error YAUL_BUILD (build directory) contains spaces)
endif

ifeq ($(strip $(YAUL_CDB)),)
  $(error Undefined YAUL_CDB (update JSON compile command database))
endif
ifneq ($(YAUL_CDB),$(filter $(YAUL_CDB),0 1))
  $(error Invalid value for YAUL_CDB (update JSON compile command database))
endif

# Check options
ifeq ($(strip $(YAUL_OPTION_DEV_CARTRIDGE)),)
  $(error Undefined YAUL_OPTION_DEV_CARTRIDGE (development cartridge))
endif
ifneq ($(YAUL_OPTION_DEV_CARTRIDGE),$(filter $(YAUL_OPTION_DEV_CARTRIDGE),0 1 2))
  $(error Invalid value for YAUL_OPTION_DEV_CARTRIDGE (development cartridge))
endif

ifeq ($(strip $(YAUL_OPTION_MALLOC_IMPL)),)
  $(error Undefined YAUL_OPTION_MALLOC_IMPL (malloc implementation))
endif
ifneq (1,$(words [$(strip $(YAUL_OPTION_MALLOC_IMPL))]))
  $(error YAUL_OPTION_MALLOC_IMPL (malloc implementation) contains spaces)
endif
ifneq ($(YAUL_OPTION_MALLOC_IMPL),$(filter $(YAUL_OPTION_MALLOC_IMPL),tlsf slob))
  $(error Invalid value for YAUL_OPTION_MALLOC_IMPL (malloc implementation))
endif

ifeq ($(strip $(YAUL_OPTION_SPIN_ON_ABORT)),)
  $(error Undefined YAUL_OPTION_SPIN_ON_ABORT (spin on calling abort()))
endif
ifneq ($(YAUL_OPTION_SPIN_ON_ABORT),$(filter $(YAUL_OPTION_SPIN_ON_ABORT),0 1))
  $(error Invalid value for YAUL_OPTION_SPIN_ON_ABORT (spin on calling abort()))
endif

ifeq ($(OS),Windows_NT)
EXE_EXT:= .exe
endif

# Installation location; by default, this is the same path that contains the
# tool-chain, but it can be set to another path instead.
YAUL_PREFIX?= $(YAUL_INSTALL_ROOT)
ifeq ($(strip $(YAUL_PREFIX)),)
YAUL_PREFIX:= $(YAUL_INSTALL_ROOT)
endif

YAUL_PROG_SH_PREFIX?= $(YAUL_ARCH_SH_PREFIX)
ifeq ($(strip $(YAUL_PROG_SH_PREFIX)),)
YAUL_PROG_SH_PREFIX:= $(YAUL_ARCH_SH_PREFIX)
endif

SH_AS:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-as$(EXE_EXT)
SH_AR:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-ar$(EXE_EXT)
SH_CC:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-gcc$(EXE_EXT)
SH_CXX:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-g++$(EXE_EXT)
SH_LD:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-gcc$(EXE_EXT)
SH_NM:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-nm$(EXE_EXT)
SH_OBJCOPY:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-objcopy$(EXE_EXT)
SH_OBJDUMP:= $(YAUL_INSTALL_ROOT)/bin/$(YAUL_PROG_SH_PREFIX)-objdump$(EXE_EXT)

SH_CFLAGS_shared:= \
	-pedantic \
	-s \
	-ffreestanding \
	-ffast-math \
	-fstrict-aliasing \
	-fdelete-null-pointer-checks \
	-fmerge-all-constants \
	-Wmissing-include-dirs \
	-Wfatal-errors \
	-Wall \
	-Wextra \
	-Wduplicated-branches \
	-Wduplicated-cond \
	-Wnull-dereference \
	-Winit-self \
	-Wbad-function-cast \
	-Wshadow \
	-Wunused \
	-DHAVE_DEV_CARTRIDGE=$(YAUL_OPTION_DEV_CARTRIDGE) \
	-DFIXMATH_NO_OVERFLOW=1 \
	-DFIXMATH_NO_ROUNDING=1

ifeq ($(strip $(YAUL_OPTION_MALLOC_IMPL)),tlsf)
SH_CFLAGS_shared += \
	-DMALLOC_IMPL_TLSF
endif
ifeq ($(strip $(YAUL_OPTION_MALLOC_IMPL)),slob)
SH_CFLAGS_shared += \
	-DMALLOC_IMPL_SLOB
endif

ifeq ($(strip $(YAUL_OPTION_SPIN_ON_ABORT)),1)
SH_CFLAGS_shared += \
	-DSPIN_ON_ABORT=1
endif

SH_CFLAGS:= \
	-std=c11 \
	$(SH_CFLAGS_shared)

SH_CXXFLAGS_shared:=

SH_CXXFLAGS:= \
	-fno-exceptions \
	-fno-rtti \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-threadsafe-statics \
	-fno-use-cxa-atexit \
	$(SH_CXXFLAGS_shared)

SH_CFLAGS_shared_release:= -Os -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables
SH_CFLAGS_shared_debug:= -Og -ggdb3 -DDEBUG

SH_CFLAGS_release:= $(SH_CFLAGS_shared_release) $(SH_CFLAGS)
SH_CFLAGS_debug:= $(SH_CFLAGS_shared_debug) $(SH_CFLAGS)

SH_CXXFLAGS_release:= $(SH_CFLAGS_shared_release) $(SH_CXXFLAGS)
SH_CXXFLAGS_debug:= $(SH_CFLAGS_shared_debug) $(SH_CXXFLAGS)

CDB_FILE:= $(join $(YAUL_BUILD_ROOT)/,compile_commands.json)

ifeq ($(strip $(YAUL_OPTION_SPIN_ON_ABORT)),1)
define update-build-commands
	@$(THIS_ROOT)/libyaul/common/update-cdb -i $1 -d $2 -o $3 -- $4
endef
else
define update-build-commands
endef
endif

define macro-sh-build-object
	@printf -- "$(V_BEGIN_YELLOW)$(shell v="$@"; printf -- "$${v#$(YAUL_BUILD_ROOT)/}")$(V_END)\n"
	$(ECHO)mkdir -p $(@D)
	$(ECHO)$(SH_CC) -MF $(YAUL_BUILD_ROOT)/$(SUB_BUILD)/$1/$*.d -MD $(SH_CFLAGS_$1) \
		$(foreach dir,$(INCLUDE_DIRS),-I$(abspath $(dir))) \
		-c $(abspath $(<)) -o $@
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		$(CDB_FILE),\
		$(SH_CFLAGS_$1) $(foreach dir,$(INCLUDE_DIRS),-I$(abspath $(dir))))
endef

define macro-sh-build-c++-object
	@printf -- "$(V_BEGIN_YELLOW)$(shell v="$@"; printf -- "$${v#$(YAUL_BUILD_ROOT)/}")$(V_END)\n"
	$(ECHO)mkdir -p $(@D)
	$(ECHO)$(SH_CXX) -MF $(YAUL_BUILD_ROOT)/$(SUB_BUILD)/$1/$*.d -MD $(SH_CXXFLAGS_$1) \
		$(foreach dir,$(INCLUDE_DIRS),-I$(abspath $(dir))) \
		-c $(abspath $(<)) -o $@
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		$(CDB_FILE),\
		$(SH_CXXFLAGS_$1) $(foreach dir,$(INCLUDE_DIRS),-I$(abspath $(dir))))
endef

define macro-sh-build-library
	@printf -- "$(V_BEGIN_YELLOW)$(shell v="$@"; printf -- "$${v#$(YAUL_BUILD_ROOT)/}")$(V_END)\n"
	$(ECHO)$(SH_AR) rcs $@ $^
endef

define macro-sh-generate-install-header-rule
$(YAUL_PREFIX)/$(YAUL_ARCH_SH_PREFIX)/include/$3/$2: $1/$2
	$(ECHO)[ "$(SILENT)" != 1 ] && set -x; \
	mkdir -p "$$(@D)"; \
	path=$$$$(cd "$$(@D)"; pwd); \
	printf -- "$(V_BEGIN_BLUE)$$$${path#$$(YAUL_PREFIX)/$(YAUL_ARCH_SH_PREFIX)/}/$$(@F)$(V_END)\n";
	$(ECHO)$(INSTALL) -m 644 $$< $$@

install-$4: $4 $(YAUL_PREFIX)/$(YAUL_ARCH_SH_PREFIX)/include/$3/$2
endef

define macro-sh-generate-install-lib-rule
$(YAUL_PREFIX)/$(YAUL_ARCH_SH_PREFIX)/lib/$2: $1
	@printf -- "$(V_BEGIN_BLUE)lib/$2$(V_END)\n"
	$(ECHO)mkdir -p "$$(@D)"
	$(ECHO)$(INSTALL) -m 644 $$< $$@

install-$3: $3 $(YAUL_PREFIX)/$(YAUL_ARCH_SH_PREFIX)/lib/$2
endef

M68K_ARCH:= m68k-elf

M68K_AS:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-as$(EXE_EXT)
M68K_AR:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-ar$(EXE_EXT)
M68K_CC:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-gcc$(EXE_EXT)
M68K_CXX:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-g++$(EXE_EXT)
M68K_LD:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-gcc$(EXE_EXT)
M68K_NM:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-nm$(EXE_EXT)
M68K_OBJCOPY:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-objcopy$(EXE_EXT)
M68K_OBJDUMP:= $(YAUL_INSTALL_ROOT)/$(M68K_ARCH)/bin/$(M68K_ARCH)-objdump$(EXE_EXT)

FIND:= find$(EXE_EXT)
INSTALL:= install$(EXE_EXT)
SED:= sed$(EXE_EXT)
STRIP:= strip$(EXE_EXT)

V_BEGIN_BLACK= [1;30m
V_BEGIN_RED= [1;31m
V_BEGIN_GREEN= [1;32m
V_BEGIN_YELLOW= [1;33m
V_BEGIN_BLUE= [1;34m
V_BEGIN_MAGENTA= [1;35m
V_BEGIN_CYAN= [1;36m
V_BEGIN_WHITE= [1;37m
V_END= [m

ifeq ($(strip $(SILENT)),)
  ECHO=
else
  ECHO=@
endif
export ECHO

ifneq ($(strip $(NOCOLOR)),)
  V_BEGIN_BLACK=
  V_BEGIN_RED=
  V_BEGIN_GREEN=
  V_BEGIN_YELLOW=
  V_BEGIN_BLUE=
  V_BEGIN_MAGENTA=
  V_BEGIN_CYAN=
  V_BEGIN_WHITE=
  V_END=
endif

MAIN_TARGET= yaul
