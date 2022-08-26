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

all : clean mkdir_kernel_build kernel.img fs.img kallsyms

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
	$(RISCVGNU)-ld -T $(KERNEL_DIR)/linker.ld -Map kernel.map -o $(KERNEL_BUILD_DIR)/kernel_tmp.elf  $(OBJ_FILES)

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

$(USER_DIR)/_cat: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/cat.o $(USER_DIR)/cat.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/cat.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_ls: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/ls.o $(USER_DIR)/ls.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/ls.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_echo: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/echo.o $(USER_DIR)/echo.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/echo.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_mkdir: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/mkdir.o $(USER_DIR)/mkdir.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/mkdir.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_rm: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/rm.o $(USER_DIR)/rm.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/rm.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_pipetest: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/pipetest.o $(USER_DIR)/pipetest.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/pipetest.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

USER_CFLAGS += -DNET_TESTS_PORT=$(SERVERPORT)
$(USER_DIR)/_udptest: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/udptest.o $(USER_DIR)/udptest.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/udptest.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym

$(USER_DIR)/_dhcpc: $(ULIB) 
	$(RISCVGNU)-gcc $(USER_CFLAGS) -I. -I$(USER_DIR) -c -o $(USER_DIR)/dhcpc.o $(USER_DIR)/dhcpc.c
	$(RISCVGNU)-ld $(LDFLAGS)  -T $(USER_DIR)/linker.ld -o $@  $(USER_DIR)/dhcpc.o $^
	$(RISCVGNU)-objdump -S $@ > $@.asm
	$(RISCVGNU)-objdump -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@.sym


mkfs/mkfs: mkfs/mkfs.c
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

USER_PROGS= $(USER_DIR)/_init $(USER_DIR)/initcode.out $(USER_DIR)/_sh $(USER_DIR)/_cat \
	$(USER_DIR)/_ls $(USER_DIR)/_echo $(USER_DIR)/_mkdir $(USER_DIR)/_rm \
	$(USER_DIR)/_pipetest $(USER_DIR)/_udptest  $(USER_DIR)/_dhcpc

fs.img: mkfs/mkfs readme.md $(USER_PROGS)
	mkfs/mkfs fs.img readme.md $(USER_PROGS)

scripts/kallsyms: scripts/kallsyms.c
	gcc -Werror -Wall -I. -o scripts/kallsyms scripts/kallsyms.c

kallsyms: scripts/kallsyms
	$(RISCVGNU)-nm -n kernel_build/kernel_tmp.elf | scripts/kallsyms --all-symbols > $(KERNEL_BUILD_DIR)/tmp_kallsyms1.S
	$(RISCVGNU)-gcc -c $(ASMOPS) $(KERNEL_BUILD_DIR)/tmp_kallsyms1.S -o $(KERNEL_BUILD_DIR)/tmp_kallsyms1.o
	$(RISCVGNU)-ld -T $(KERNEL_DIR)/linker.ld -Map kernel.map -o $(KERNEL_BUILD_DIR)/kernel_tmp1.elf \
		 $(OBJ_FILES) $(KERNEL_BUILD_DIR)/tmp_kallsyms1.o
	$(RISCVGNU)-nm -n $(KERNEL_BUILD_DIR)/kernel_tmp1.elf | scripts/kallsyms --all-symbols > $(KERNEL_BUILD_DIR)/tmp_kallsyms2.S
	$(RISCVGNU)-gcc -c $(ASMOPS) $(KERNEL_BUILD_DIR)/tmp_kallsyms2.S -o $(KERNEL_BUILD_DIR)/tmp_kallsyms2.o
	$(RISCVGNU)-ld -T $(KERNEL_DIR)/linker.ld -Map kernel.map -o $(KERNEL_BUILD_DIR)/kernel.elf \
		 $(OBJ_FILES) $(KERNEL_BUILD_DIR)/tmp_kallsyms2.o
	$(RISCVGNU)-objcopy $(KERNEL_BUILD_DIR)/kernel.elf -O binary kernel.img
	$(RISCVGNU)-objdump -S $(KERNEL_BUILD_DIR)/kernel.elf > $(KERNEL_BUILD_DIR)/kernel.asm
	$(RISCVGNU)-objdump -t $(KERNEL_BUILD_DIR)/kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(KERNEL_BUILD_DIR)/kernel.sym


clean :
	rm -rf $(KERNEL_BUILD_DIR) *.img kernel.map null.d $(USER_BUILD_DIR)  \
		$(USER_DIR)/*.o $(USER_DIR)/*.asm \
		$(USER_DIR)/_* $(USER_DIR)/*.d \
		$(USER_DIR)/*.sym \
		$(USER_DIR)/initcode  $(USER_DIR)/initcode.out

FWDPORT = $(shell expr `id -u` % 5000 + 25999)
SERVERPORT = $(shell expr `id -u` % 5000 + 25099)
QEMU_FLAGS  += -nographic

##
##https://blog.csdn.net/ren_star/article/details/125060518
##
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
QEMUOPTS += -netdev user,id=net0,hostfwd=udp::$(FWDPORT)-:2000,net=192.168.100.0/24,dhcpstart=192.168.100.100 \
	-object filter-dump,id=net0,netdev=net0,file=packets.pcap
QEMUOPTS += -device e1000,netdev=net0,bus=pcie.0
run:
	qemu-system-riscv64 -machine virt -m 128M  -bios none -kernel $(KERNEL_BUILD_DIR)/kernel.elf  $(QEMU_FLAGS) $(QEMUOPTS)
debug:
	qemu-system-riscv64 -machine virt -m 128M  -bios none $(QEMU_FLAGS) -kernel $(KERNEL_BUILD_DIR)/kernel.elf $(QEMUOPTS) -S -s
gdb:
	gdb-multiarch --tui $(KERNEL_BUILD_DIR)/kernel.elf -ex 'target remote localhost:1234'
