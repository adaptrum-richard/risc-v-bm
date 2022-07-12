OUTPUT_ARCH( "riscv" )
ENTRY( main )

SECTIONS
{
	. = 0x1000000000,

	_text = .;
	.text :
	{ 
		*(.text)
	}
	. = ALIGN(4096);
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

	. = ALIGN(4096);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;

	_end = .;
}
