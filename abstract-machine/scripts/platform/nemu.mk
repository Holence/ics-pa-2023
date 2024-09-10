AM_SRCS := platform/nemu/trm.c \
		   platform/nemu/ioe/ioe.c \
		   platform/nemu/ioe/timer.c \
		   platform/nemu/ioe/input.c \
		   platform/nemu/ioe/gpu.c \
		   platform/nemu/ioe/audio.c \
		   platform/nemu/ioe/disk.c \
		   platform/nemu/mpe.c

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -T $(AM_HOME)/scripts/linker.ld \
			 --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0
LDFLAGS   += --gc-sections -e _start
NEMUFLAGS += -l $(shell dirname $(IMAGE).elf)/nemu-log.txt

CFLAGS += -DMAINARGS=\"$(mainargs)\"
CFLAGS += -I$(AM_HOME)/am/src/platform/nemu/include
.PHONY: $(AM_HOME)/am/src/platform/nemu/trm.c

image: $(IMAGE).elf
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin

# -S (--strip-all)
#    Do not copy relocation and symbol information from the source file.  Also deletes debug sections.
# -S之后只是少了些附加信息，依旧可以被linux运行、被readelf、被objdump
# -O binary 是保留raw binary file，不能被linux运行、被readelf、被objdump

	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin

run: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin

# use function info in IMAGE.elf
# make ARCH=riscv32-nemu run_ftrace 
# use function info in IMAGE.elf and a navy elf
# make ARCH=riscv32-nemu run_ftrace NAVY_ELF=$NAVY_HOME/apps/bird/build/bird-riscv32
ELF_ARGS = -e $(IMAGE).elf
ifneq ($(NAVY_ELF), )
	ELF_ARGS := $(ELF_ARGS) -e $(NAVY_ELF)
endif
run_ftrace: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(ELF_ARGS) $(NEMUFLAGS)" IMG=$(IMAGE).bin

run_batch: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="-b $(NEMUFLAGS)" IMG=$(IMAGE).bin

gdb: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin
