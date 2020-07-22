ENTRY(_start)

SECTIONS
{   . = 0x40000000;
    .nloader : {
        KEEP(*(.nloader))
    }

    .dtb : ALIGN(8) { 
        __dtb_start = .;
        KEEP(*(.dtb)) 
        __dtb_end = .;
    }  
    . = 0x40001000;
    .initrd : ALIGN(12) {
        __initrd_start = .;
        KEEP(*(.initrd))
        __initrd_end = .;
    }

    . = 0x40280000;
    .linux : { 
        __linux_start = .;
        KEEP(*(.linux)) 
        __linux_end = .;
    }

}
