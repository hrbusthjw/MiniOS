SECTIONS
{
	PROVIDE(kernel_base = 0x0);

    . = 0x00010000;

	PROVIDE(start_text = .);
	.text : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .text)
	}

	.rodata : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .rodata)
	}
	PROVIDE(end_text = .);

	. = ALIGN(4096);
	PROVIDE(start_data = .);
	.data : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .data)
	}

	.bss : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .bss)
	}

	e_data = .;

	. = 0x80000000;

	PROVIDE(s_first_task = LOADADDR(.first_task));
	.first_task : AT(e_data){
		*first_task_entry*(.text .data. rodata .bss)
		*first_task*(.text .data. rodata .bss)
		*lib_syscall*(.text .data. rodata .bss)
	}
	PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));

	PROVIDE(mem_free_start = e_first_task);
}
