target remote 127.0.0.1:1234
display/i $pc
layout split
break init
set confirm off
symbol-file ../../linux-4.19.133/vmlinux
break vring_interrupt
