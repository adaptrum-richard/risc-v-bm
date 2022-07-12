RISCVGNU ?= riscv64-linux-gnu

COPS += -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -MD 
COPS += -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. 
COPS += -fno-stack-protector -fno-pie -no-pie
ASMOPS = -g
LDFLAGS = -z max-page-size=4096

USER_DIR = user
USER_BUILD_DIR = user_build
KERNEL_BUILD_DIR = kernel_build
KERNEL_DIR = kernel

all : clean mkdir_kernel_build kernel.img fs.img

mkdir_kernel_build:
	mkdir -p $(KERNEL_BUILD_DIR)

$(KERNEL_BUILD_DIR)/%_c.o: $(KERNEL_DIR)/%.c
	$(RISCVGNU)-gcc $(COPS) -c $< -o $@

$(KERNEL_BUILD_DIR)/%_s.o: $(KERNEL_DIR)/%.S
	$(RISCVGNU)-gcc $(ASMOPS) -c -D__ASSEMBLY__ $< -o $@

C_FILES = $(wildcard $(KERNEL_DIR)/*.c)
ASM_FILES = $(wildcard $(KERNEL_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(KERNEL_DIR)/%.c=$(KERNEL_BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(KERNEL_DIR)/%.S=$(KERNEL_BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)


kernel.img: $(KERNEL_DIR)/linker.ld $(OBJ_FILES)
	$(RISCVGNU)-ld -T $(KERNEL_DIR)/linker.ld -Map kernel.map -o $(KERNEL_BUILD_DIR)/kernel.elf  $(OBJ_FILES)
	$(RISCVGNU)-objcopy $(KERNEL_BUILD_DIR)/kernel.elf -O binary kernel.img
	$(RISCVGNU)-objdump -S $(KERNEL_BUILD_DIR)/kernel.elf > $(KERNEL_BUILD_DIR)/kernel.asm
	$(RISCVGNU)-objdump -t $(KERNEL_BUILD_DIR)/kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(KERNEL_BUILD_DIR)/kernel.sym

$(USER_DIR)/initcode.out: $(USER_DIR)/initcode.S 
	$(RISCVGNU)-gcc $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $(USER_DIR)/initcode.S -o $(USER_DIR)/initcode.o
	$(RISCVGNU)-ld $(LDFLAGS) -N -e start -Ttext 0x1000000000  -o $(USER_DIR)/initcode.out $(USER_DIR)/initcode.o
	$(RISCVGNU)-objcopy -S -O binary $(USER_DIR)/initcode.out $(USER_DIR)/initcode
	$(RISCVGNU)-objdump -S $(USER_DIR)/initcode.o > $(USER_DIR)/initcode.asm


USER_CFLAGS=-Wall -Werror -O -fno-omit-frame-pointer \
	-ggdb -MD -mcmodel=medany -ffreestanding -fno-common \
		-nostdlib -mno-relax -fno-stack-protector -fno-pie -no-pie 

$(USER_DIR)/sys.o : $(USER_DIR)/sys.S
	$(RISCVGNU)-gcc $(CFLAGS) -I. -I$(KERNEL_DIR) -c -o $(USER_DIR)/sys.o $(USER_DIR)/sys.S

$(USER_DIR)/string.o : $(USER_DIR)/string.c
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/string.o $(USER_DIR)/string.c

$(USER_DIR)/printf.o : $(USER_DIR)/printf.c
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/printf.o $(USER_DIR)/printf.c

$(USER_DIR)/umalloc.o : $(USER_DIR)/umalloc.c
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/umalloc.o $(USER_DIR)/umalloc.c

$(USER_DIR)/ulib.o : $(USER_DIR)/ulib.c $(USER_DIR)/umalloc.o $(USER_DIR)/printf.o  $(USER_DIR)/string.o $(USER_DIR)/sys.o 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/ulib.o $(USER_DIR)/ulib.c


ULIB = $(USER_DIR)/string.o $(USER_DIR)/sys.o  $(USER_DIR)/printf.o $(USER_DIR)/umalloc.o 
ULIB += $(USER_DIR)/ulib.o 

$(USER_DIR)/_init: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/init.o $(USER_DIR)/init.c
	$(RISCVGNU)-ld $(LDFLAGS)  -N -e main -Ttext 0x1000000000 -o $@  $(USER_DIR)/init.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_sh: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/sh.o $(USER_DIR)/sh.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/sh.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

mkfs/mkfs: mkfs/mkfs.c
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

USER_PROGS= $(USER_DIR)/_init $(USER_DIR)/initcode.out $(USER_DIR)/_sh

fs.img: mkfs/mkfs README $(USER_PROGS)
	mkfs/mkfs fs.img README $(USER_PROGS)

clean :
	rm -rf $(KERNEL_BUILD_DIR) *.img kernel.map null.d $(USER_BUILD_DIR)  \
		$(USER_DIR)/*.o $(USER_DIR)/*.asm \
		$(USER_DIR)/_* $(USER_DIR)/*.d \
		$(USER_DIR)/*.sym \
		$(USER_DIR)/initcode  $(USER_DIR)/initcode.out

QEMU_FLAGS  += -nographic

QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

run:
	qemu-system-riscv64 -machine virt -m 128M  -bios none -kernel $(KERNEL_BUILD_DIR)/kernel.elf  $(QEMU_FLAGS) $(QEMUOPTS)
debug:
	qemu-system-riscv64 -machine virt -m 128M  -bios none $(QEMU_FLAGS) -kernel $(KERNEL_BUILD_DIR)/kernel.elf $(QEMUOPTS) -S -s
gdb:
	gdb-multiarch --tui $(KERNEL_BUILD_DIR)/kernel.elf -ex 'target remote localhost:1234'
