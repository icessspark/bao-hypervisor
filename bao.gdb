target remote 127.0.0.1:1234
display/i $pc
layout split
break init
set confirm off
break *0x40000000
symbol-file bin/qemu/aarch64-virt/bao.elf
