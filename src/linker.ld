OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
	. = 0x80000000,
	_text_boot = .;
	.text.boot : { *(.text.boot) }
	_etext_boot = .;
	
	. = ALIGN(4096);
	_text = .;
	.text :
	{ 
		*(.text)
	}
	_etext = .;

	. = ALIGN(4096);
	_rodata = .;
	.rodata :
	{ 
		*(.rodata) 
	}
	_erodata = .;
    
    . = ALIGN(4096);
	_data = .;
	.data : { *(.data) }
	_edata = .;

	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;

	. += 0x8000;
	kernel_sp = .;
	
	_end = .;
}
