OUTPUT_ARCH( "riscv" )
SECTIONS
{
	. = 0x1000000000,
	_text = .;
	.text :
	{ 
		*(.text)
	}
	_etext = .;


	_rodata = .;
	.rodata :
	{ 
		*(.rodata) 
	}
	_erodata = .;
    

	_data = .;
	.data : { *(.data) }
	_edata = .;


	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;

	_end = .;
}
