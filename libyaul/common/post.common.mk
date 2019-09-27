.PHONY: $(SH_PROGRAM)

ifeq ($(strip $(SH_PROGRAM)),)
  $(error Empty SH_PROGRAM (SH program name))
endif

# Check that SH_OBJECTS doesn't include duplicates
# Be mindful that sort remove duplicates
SH_OBJECTS_UNIQ= $(sort $(SH_OBJECTS))
SH_OBJECTS_NO_LINK_UNIQ= $(sort $(SH_OBJECTS_NO_LINK))

ifeq ($(strip $(SH_OBJECTS_UNIQ)),)
  # If both SH_OBJECTS_UNIQ and SH_OBJECTS_NO_LINK_UNIQ is empty
  ifeq ($(strip $(SH_OBJECTS_NO_LINK_UNIQ)),)
    $(error Empty SH_OBJECTS (SH object list))
  endif
endif

# Check that M68K_OBJECTS doesn't include duplicates
M68K_OBJECTS_UNIQ= $(sort $(M68K_OBJECTS))

ifneq ($(strip $(M68K_PROGRAM)),)
  ifneq ($(strip $(M68K_PROGRAM)),undefined-program)
    ifeq ($(strip $(M68K_OBJECTS_UNIQ)),)
      $(error Empty M68K_OBJECTS (M68K object list))
    endif
  endif
endif

SH_DEFSYMS=

ifneq ($(strip $(IP_MASTER_STACK_ADDR)),)
  SH_DEFSYMS+= -Wl,--defsym=__master_stack=$(IP_MASTER_STACK_ADDR)
endif

ifneq ($(strip $(IP_SLAVE_STACK_ADDR)),)
  SH_DEFSYMS+= -Wl,--defsym=__slave_stack=$(IP_SLAVE_STACK_ADDR)
endif

SH_LDFLAGS+= $(SH_DEFSYMS)
SH_LXXFLAGS+= $(SH_DEFSYMS)

SH_SPECS= yaul.specs

ifeq ($(strip $(SH_CUSTOM_SPECS)),)
  SH_SPECS+= yaul-main.specs
else
  SH_SPECS+= $(SH_CUSTOM_SPECS)
endif

ROMDISK_DEPS:= $(shell find ./romdisk -type f 2> /dev/null) $(ROMDISK_DEPS)

ifneq ($(strip $(M68K_PROGRAM)),)
  ifneq ($(strip $(M68K_PROGRAM)),undefined-program)
    # Check if there is a romdisk.o. If not, append to list of SH
    # objects
    ifeq ($(strip $(filter %.romdisk.o,$(SH_OBJECTS_UNIQ))),)
      SH_OBJECTS_UNIQ+= root.romdisk.o
    endif
    ROMDISK_DEPS+= ./romdisk/$(M68K_PROGRAM).m68k
  endif
endif

SH_DEPS:= $(SH_OBJECTS_UNIQ:.o=.d)
SH_TEMPS:= $(SH_OBJECTS_UNIQ:.o=.i) $(SH_OBJECTS_UNIQ:.o=.ii) $(SH_OBJECTS_UNIQ:.o=.s)
SH_DEPS_NO_LINK:= $(SH_OBJECTS_NO_LINK_UNIQ:.o=.d)

# Parse out included paths from GCC when the specs files are used. This is used
# to explictly populate each command database entry with include paths
SH_INCLUDE_DIRS:=$(shell echo | $(SH_CC) -E -Wp,-v $(foreach specs,$(SH_SPECS),-specs=$(specs)) - 2>&1 | \
	awk '/^\s/ { sub(/^\s+/,"-I"); print }')

define update-build-commands
	@$(YAUL_INSTALL_ROOT)/share/update-cdb -i $1 -d $2 -o $3 -- $4
endef

$(SH_PROGRAM): $(SH_PROGRAM).iso

all: $(SH_PROGRAM).iso

example: all

$(SH_PROGRAM).bin: $(SH_PROGRAM).elf
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_OBJCOPY) -O binary $< $@
	@[ -z "${SILENT}" ] && du -hs $@ | awk '{ print $$1 }' || true

$(SH_PROGRAM).elf: $(SH_OBJECTS_UNIQ) $(SH_OBJECTS_NO_LINK_UNIQ)
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_LD) $(foreach specs,$(SH_SPECS),-specs=$(specs)) $(SH_OBJECTS_UNIQ) $(SH_LDFLAGS) $(foreach lib,$(SH_LIBRARIES),-l$(lib)) -o $@
	$(ECHO)$(SH_NM) $(SH_PROGRAM).elf > $(SH_PROGRAM).sym
	$(ECHO)$(SH_OBJDUMP) -S $(SH_PROGRAM).elf > $(SH_PROGRAM).asm

./romdisk/$(M68K_PROGRAM).m68k: $(M68K_PROGRAM).m68k.elf
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(M68K_OBJCOPY) -O binary $< $@
	$(ECHO)chmod -x $@
	@du -hs $@ | awk '{ print $$1 " ""'"($@)"'" }'

$(M68K_PROGRAM).m68k.elf: $(M68K_OBJECTS_UNIQ)
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(M68K_LD) $(M68K_OBJECTS_UNIQ) $(M68K_LDFLAGS) -o $@
	$(ECHO)$(M68K_NM) $(M68K_PROGRAM).m68k.elf > $(M68K_PROGRAM).m68k.sym

./romdisk:
	$(ECHO)mkdir -p $@

%.romdisk: ./romdisk $(ROMDISK_DEPS)
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/genromfs $(ROMDISK_FLAGS) -d ./romdisk/ -f $@

%.romdisk.o: %.romdisk
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/fsck.genromfs ./romdisk/
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/bin2o $< `echo "$<" | sed -E 's/[\. ]/_/g'` $@
	$(ECHO)$(RM) $<

%.o: %.c
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_CC) -MF $(abspath $*.d) -MD $(SH_CFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c -o $@ $<
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		"compile_commands.json",\
		$(SH_CFLAGS) $(SH_INCLUDE_DIRS))

%.o: %.cc
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_CXX) -MF $(abspath $*.d) -MD $(SH_CXXFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c -o $@ $<
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		"compile_commands.json",\
		$(SH_CXXFLAGS) $(SH_INCLUDE_DIRS))

%.o: %.C
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_CXX) -MF $(abspath $*.d) -MD $(SH_CXXFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c -o $@ $<
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		"compile_commands.json",\
		$(SH_CXXFLAGS) $(SH_INCLUDE_DIRS))

%.o: %.cpp
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_CXX) -MF $(abspath $*.d) -MD $(SH_CXXFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c -o $@ $<
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		"compile_commands.json",\
		$(SH_CXXFLAGS) $(SH_INCLUDE_DIRS))

%.o: %.cxx
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_CXX) -MF $(abspath $*.d) -MD $(SH_CXXFLAGS) $(foreach specs,$(SH_SPECS),-specs=$(specs)) -c -o $@ $<
	$(call update-build-commands,\
		$(abspath $(<)),\
		$(abspath $(<D)),\
		"compile_commands.json",\
		$(SH_CXXFLAGS) $(SH_INCLUDE_DIRS))

%.o: %.sx
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(SH_AS) $(SH_AFLAGS) -o $@ $<

%.m68k.o: %.m68k.sx
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)$(M68K_AS) $(M68K_AFLAGS) -o $@ $<

$(SH_PROGRAM).iso: $(SH_PROGRAM).bin IP.BIN $(shell find $(IMAGE_DIRECTORY)/ -type f 2>/dev/null)
	@printf -- "$(V_BEGIN_YELLOW)$@$(V_END)\n"
	$(ECHO)mkdir -p $(IMAGE_DIRECTORY)
	$(ECHO)cp $(SH_PROGRAM).bin $(IMAGE_DIRECTORY)/$(IMAGE_1ST_READ_BIN)
	$(ECHO)for txt in "ABS.TXT" "BIB.TXT" "CPY.TXT"; do \
	    if ! [ -s $(IMAGE_DIRECTORY)/$$txt ]; then \
		printf -- "empty\n" > $(IMAGE_DIRECTORY)/$$txt; \
	    fi \
	done
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/make-iso $(IMAGE_DIRECTORY) $(SH_PROGRAM) $(MAKE_ISO_REDIRECT)

IP.BIN: $(YAUL_INSTALL_ROOT)/share/yaul/bootstrap/ip.sx
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/make-ip	\
		"$(IP_VERSION)" \
		$(IP_RELEASE_DATE) \
		"$(IP_AREAS)" \
		"$(IP_PERIPHERALS)" \
		"$(IP_TITLE)" \
		$(IP_MASTER_STACK_ADDR) \
		$(IP_SLAVE_STACK_ADDR) \
		$(IP_1ST_READ_ADDR)

clean:
	$(ECHO)printf -- "$(V_BEGIN_CYAN)$(SH_PROGRAM)$(V_END) $(V_BEGIN_GREEN)clean$(V_END)\n"
	$(ECHO)-rm -f \
	    $(SH_PROGRAM).bin \
	    $(SH_PROGRAM).iso \
	    $(SH_OBJECTS_UNIQ) \
	    $(SH_DEPS) \
	    $(SH_DEPS_NO_LINK) \
	    $(SH_TEMPS) \
	    $(SH_PROGRAM).asm \
	    $(SH_PROGRAM).bin \
	    $(SH_PROGRAM).elf \
	    $(SH_PROGRAM).map \
	    $(SH_PROGRAM).sym \
	    $(SH_OBJECTS_NO_LINK_UNIQ) \
	    $(SH_DEPS_NO_LINK) \
	    root.romdisk \
	    IP.BIN \
	    IP.BIN.map
ifneq ($(strip $(M68K_PROGRAM)),)
	$(ECHO)printf -- "$(V_BEGIN_CYAN)$(M68K_PROGRAM)$(V_END) $(V_BEGIN_GREEN)clean$(V_END)\n"
	$(ECHO)-rm -f \
	    romdisk/$(M68K_PROGRAM).m68k \
	    $(M68K_PROGRAM).m68k.elf \
	    $(M68K_PROGRAM).m68k.sym \
	    $(M68K_OBJECTS)
endif

list-targets:
	@$(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | \
	awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | \
	sort | \
	grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'

-include $(SH_DEPS)
-include $(SH_DEPS_NO_LINK)
