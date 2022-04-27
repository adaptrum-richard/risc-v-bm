RISCVGNU ?= riscv64-linux-gnu

COPS += -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -MD 
COPS += -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. 
COPS += -fno-stack-protector -fno-pie -no-pie
ASMOPS = -g

BUILD_DIR = build
SRC_DIR = src

all : clean kernel.img fs.img

clean :
	rm -rf $(BUILD_DIR) *.img kernel.map

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(RISCVGNU)-gcc $(COPS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(RISCVGNU)-gcc $(ASMOPS) -c -D__ASSEMBLY__ $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

mkfs/mkfs: mkfs/mkfs.c
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

fs.img: mkfs/mkfs README
	mkfs/mkfs fs.img README 

kernel.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(RISCVGNU)-ld -T $(SRC_DIR)/linker.ld -Map kernel.map -o $(BUILD_DIR)/kernel.elf  $(OBJ_FILES)
	$(RISCVGNU)-objcopy $(BUILD_DIR)/kernel.elf -O binary kernel.img

QEMU_FLAGS  += -nographic

QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

run:
	qemu-system-riscv64 -machine virt -m 128M  -bios none -kernel build/kernel.elf  $(QEMU_FLAGS) $(QEMUOPTS)
debug:
	qemu-system-riscv64 -machine virt -m 128M  -bios none $(QEMU_FLAGS) -kernel build/kernel.elf $(QEMUOPTS) -S -s
gdb:
	gdb-multiarch --tui build/kernel.elf -ex 'target remote localhost:1234'
