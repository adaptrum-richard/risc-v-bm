RISCVGNU ?= riscv64-linux-gnu

COPS += -g -Wall -nostdlib  -Iinclude 
ASMOPS = -g -Iinclude 

BUILD_DIR = build
SRC_DIR = src

all : clean kernel.img

clean :
	rm -rf $(BUILD_DIR) *.img kernel.map

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(RISCVGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(RISCVGNU)-gcc $(ASMOPS) -MMD -c -D__ASSEMBLY__ $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(RISCVGNU)-ld -T $(SRC_DIR)/linker.ld -Map kernel.map -o $(BUILD_DIR)/kernel.elf  $(OBJ_FILES)
	$(RISCVGNU)-objcopy $(BUILD_DIR)/kernel.elf -O binary kernel.img

QEMU_FLAGS  += -nographic

run:
	qemu-system-riscv64 -machine virt -bios none -kernel build/kernel.elf  $(QEMU_FLAGS)
debug:
	qemu-system-riscv64 -M virt -bios none $(QEMU_FLAGS) -kernel build/kernel.elf  -S -s