CROSS_COMPILE=aarch64-elf-
IMAGE=Image
INITRD=initrd.gz

ifeq ($(and $(IMAGE), $(DTB), $(TARGET)),)
ifneq ($(MAKECMDGOALS), clean)
 $(error Linux image (IMAGE) and/or device tree (DTB) and/or target name \
 	(TARGET) not specified)
endif
endif

all: $(TARGET).bin

clean:
	-rm *.elf *.bin *.dtb

.PHONY: all clean

$(TARGET).bin: $(TARGET).elf
	$(CROSS_COMPILE)objcopy -S -O binary $(TARGET).elf $(TARGET).bin

%.dtb: %.dts
	dtc $< -o $@

$(TARGET).elf: lloader.S lloader.ld $(IMAGE) $(DTB)
	$(CROSS_COMPILE)gcc -Wl,-build-id=none -nostdlib -T lloader.ld\
		-o $(TARGET).elf -mcmodel=large  lloader.S -I. -D IMAGE=$(IMAGE) -D DTB=$(DTB) -D INITRD=$(INITRD)