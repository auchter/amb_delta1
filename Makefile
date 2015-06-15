ifneq ($(KERNELRELEASE),)

obj-m += amb-delta1.o

else

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules amb-delta1-overlay.dtb

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

%.dtb: %.dts
	$(KDIR)/scripts/dtc/dtc -I dts -O dtb -@ -o $@ $<

clean:
	rm -f *.o *.ko modules.order Module.symvers *.mod.c *.dtb

endif
