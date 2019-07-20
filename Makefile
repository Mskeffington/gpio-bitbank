obj-m += gpio-bitbank.o

SRC := $(shell pwd)
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean
	
modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
	
